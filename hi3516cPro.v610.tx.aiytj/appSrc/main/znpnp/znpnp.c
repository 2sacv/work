/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    :znpnp.c
 * @Created Time : 2015-08-13
 * @Version      : 1.0
 * @Author       : cheby
 * @Description  :  pnp
 */

#if defined(CUST_PROT_ZNUPNP)

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/route.h>
#include <linux/sockios.h>
#include <linux/wireless.h>
#include <sys/sysinfo.h>
#include <netdb.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>

#include <pthread.h>
#include "cJSON.h"
#include "debug.h"
#include "utils.h"
#include "search_service.h"
#include "znpnp.h"
#include "confapi.h"
#include "passwdtrans.h"
#include "system_ctrl.h"
#include "js_scheduler.h"

#define ZN_PNP_PORT 6072
#define ZN_PNP_STR			"ZENO NVR UPNP PROTOCOL"
#define ZN_PNP_RECV_BUF 512
#define ZN_PNP_BROADCAST_ADDR "255.255.255.255"
#define ZN_PNP_ETH0_CHECK_TIME (3*60*1000)

JSRWHandle 	g_znpnp_handle = NULL;
JSScheduler g_znpnp_scher = NULL;
JSScheduler g_znpnp_engine = NULL;
JSTCHandle 	g_znpnp_engine_handle = NULL;


static int g_broadcast_fd = -1;
static int znpnp_broadcase_sendto(char *buf, int len, int sockfd);
static int znpnp_build_search_response(char *res_mac, char *res_buf, int res_len);
static int znpnp_build_add_response(char *res_mac, char *dst_mac, char *slave_ip, char *res_buf, int res_len);
static int znpnp_build_del_response(char *res_mac, char *dst_mac, char *slave_ip, char *res_buf, int res_len);
void znpnp_eth0_check(void *data);
static int znpnp_set_motion_alarm();


enum ZN_CMP_TYPE {
	ZN_CMP_TYPE_NULL = 0,
	ZN_CMP_TYPE_HELLO,
	ZN_CMP_TYPE_BYE,
	ZN_CMP_TYPE_SEARCH,
	ZN_CMP_TYPE_ADD,
	ZN_CMP_TYPE_DEL
};

int add_ghost_ip(int index, char *ip)
{
	char command[128] = {0};
	sprintf(command, "ifconfig eth0:%d %s", index, ip);
	UtilSystemCmd(command);
	return 0;
}

int del_ghost_ip(int index)
{
	char command[128] = {0};
	sprintf(command, "ifconfig eth0:%d down", index);
	UtilSystemCmd(command);
	return 0;
}

void encrypt_json(char *json, int json_len)
{
	int i ;
	int pwd_len = strlen(ZN_PNP_STR);
	const char *pwd = ZN_PNP_STR;
	for(i = 0; i < json_len; i++) {
		json[i] ^= pwd[i%pwd_len];
	}
}

int znpnp_parse_cmd(cJSON *json, char *res_mac, char *dst_mac, char *slave_ip)
{
	int i = 0, cmd = 0;
	int j = 0;
	char *cmd_string[] = {"NULL", "Hello", "Bye", "Search", "AddSlaveIP", "DelSlaveIP"};
	char *node_string[] = {"Name", "RequestMac", "DestMac", "SlaveIP"};
	cJSON *json_node = NULL;

	for (i = 0; i < ARRAY_SIZE(node_string); i++) {
		json_node = cJSON_GetObjectItem(json, node_string[i]);
		if (json_node == NULL) {
			break;
		}
		
		if (!strcmp(node_string[i], "Name")) {
			for (j = 0; j <= ZN_CMP_TYPE_DEL; j++) {
				if (!strcmp(json_node->valuestring, cmd_string[j])) {
					DBG("Get cmd %s\n", json_node->valuestring);
					cmd = j;
					break;
				}
			}
		} else if (!strcmp(node_string[i], "RequestMac")) {
			//DBG("RequestMac = %s\n", json_node->valuestring);
			strcpy(res_mac, json_node->valuestring);  
		} else if (!strcmp(node_string[i], "DestMac")) {
			//DBG("DestMac = %s\n", json_node->valuestring);
			strcpy(dst_mac, json_node->valuestring);  
		} else if (!strcmp(node_string[i], "SlaveIP")) {
			//DBG("SlaveIP = %s\n", json_node->valuestring);
			strcpy(slave_ip, json_node->valuestring);  
		}
	}

	return cmd;		
}

int znpnp_parse_data(char *data, int len)
{
	cJSON *cjson_root = NULL;
	int cmd = 0, ret = 0;
	char res_mac[32] = {0}, dst_mac[32] = {0}, slave_ip[64] = {0};
	char res_buf[2048] = {0};
	int res_len = 0;

	encrypt_json(data, len);

	//DBG("Recv json = %s\n", data);
	cjson_root = cJSON_Parse(data);
	if (cjson_root == NULL) {
		ERR("parse cjson error\n");
		return -1;
	}

	cJSON *json_request = cJSON_GetObjectItem(cjson_root, "Request");
	if (json_request == NULL) {
		//ERR("json request fail\n");
		cJSON_Delete(cjson_root);
		return -1;
	}

	cmd = znpnp_parse_cmd(json_request, res_mac, dst_mac, slave_ip);
	DBG("znpnp cmd = %d\n", cmd);
	switch(cmd) {
		case ZN_CMP_TYPE_SEARCH:
			res_len = znpnp_build_search_response(res_mac, res_buf, sizeof(res_buf));
			break;

		case ZN_CMP_TYPE_ADD:
			res_len = znpnp_build_add_response(res_mac, dst_mac, slave_ip, res_buf, sizeof(res_buf));
			break;

		case ZN_CMP_TYPE_DEL:
			res_len = znpnp_build_del_response(res_mac, dst_mac, slave_ip, res_buf, sizeof(res_buf));
			break;

		case ZN_CMP_TYPE_NULL:
		case ZN_CMP_TYPE_HELLO:
		case ZN_CMP_TYPE_BYE:
			DBG("ignore cmd = %d\n", cmd);

		default:
			break;
	}

	if (res_len > 0) {
		encrypt_json(res_buf, res_len);
		ret = znpnp_broadcase_sendto(res_buf, res_len, g_broadcast_fd);
	}
	
	cJSON_Delete(cjson_root);
	return ret;
}

static int znpnp_build_search_response(char *res_mac, char *res_buf, int res_len)
{
	cJSON *cjson_root = NULL;
	cJSON *cjson_node = NULL;
	unsigned int mac_data[8] = {0};
	char tmp[128] = {0};

	cjson_root = cJSON_CreateObject();	
	if (cjson_root == NULL) {
		return -1;
	}

	cjson_node = cJSON_CreateObject();	
	if (cjson_node == NULL) {
		return -1;
	}
	
	NetPortS netport = {0};
	get_portinfo(&netport);
	
	SysInfoS info = {{0,},};
	get_sysinfo(&info);
	
	NetEthS eth = {{0,},};
	get_ethinfo(&eth);

	cJSON_AddItemToObject(cjson_root, "Response", cjson_node);
	cJSON_AddStringToObject(cjson_node, "Name", "Search");
	cJSON_AddStringToObject(cjson_node, "RequestMac", res_mac);
	cJSON_AddStringToObject(cjson_node, "Protocol", "ONVIF");

	sscanf(eth.mac, "%02X:%02X:%02X:%02X:%02X:%02X", 
		&mac_data[0], &mac_data[1], &mac_data[2], &mac_data[3], &mac_data[4], &mac_data[5]);
	sprintf(tmp, "%02X:%02X:%02X:%02X:%02X:%02X", 
			mac_data[0], mac_data[1], mac_data[2], mac_data[3], mac_data[4], mac_data[5]);
	cJSON_AddStringToObject(cjson_node, "Mac", tmp);
	
	cJSON_AddStringToObject(cjson_node, "ProtocolUrl", eth.ip);

	cJSON_AddNumberToObject(cjson_node, "ProtocolPort", netport.httpport);
	cJSON_AddStringToObject(cjson_node, "DeviceType", "IPC");
	cJSON_AddStringToObject(cjson_node, "DeviceFactory", "JCO");
	cJSON_AddStringToObject(cjson_node, "DeviceName", info.devname);
	
	char password[36] = {0,};
	SysUserS userinfo;
	memset(&userinfo, 0, sizeof(SysUserS));
	conf_get_usercfg(&userinfo);
	passwd_trans_decode(password, userinfo.user[0].onvifpasswd, strlen(userinfo.user[0].onvifpasswd));

	cJSON_AddStringToObject(cjson_node, "DefaultUser", userinfo.user[0].username);
	cJSON_AddStringToObject(cjson_node, "DefaultPwd", password);
	
	char *s = cJSON_PrintUnformatted(cjson_root);
	if (s) {
		snprintf(res_buf, res_len-1, "%s", s);
		if (g_toggle.jcpinfo == 4) {
			DBG("znpnp_build_search_response = %s\n", res_buf);
		}
		free(s);
	}

	cJSON_Delete(cjson_root);
	return strlen(res_buf);
}

static int znpnp_build_add_response(char *res_mac, char *dst_mac, char *slave_ip, char *res_buf, int res_len)
{
	cJSON *cjson_root = NULL;
	cJSON *cjson_node = NULL;
	char *addr_result[] = {"Success", "NoSupport", "Full"};
	int ret = 0;
	unsigned int mac_data[8] = {0};
	char tmp[128] = {0};
	NetEthS eth = {{0,},};
	get_ethinfo(&eth);

	sscanf(eth.mac, "%02X:%02X:%02X:%02X:%02X:%02X", 
		&mac_data[0], &mac_data[1], &mac_data[2], &mac_data[3], &mac_data[4], &mac_data[5]);
	sprintf(tmp, "%02X:%02X:%02X:%02X:%02X:%02X", 
			mac_data[0], mac_data[1], mac_data[2], mac_data[3], mac_data[4], mac_data[5]);

	if (strcmp(tmp, dst_mac)) {
		ERR("Invalid mac %s!\n", dst_mac);
		return 0;
	}

	ret = add_ghost_ip(0, slave_ip);
	
	cjson_root = cJSON_CreateObject();	
	if (cjson_root == NULL) {
		return -1;
	}

	cjson_node = cJSON_CreateObject();	
	if (cjson_node == NULL) {
		return -1;
	}

	cJSON_AddItemToObject(cjson_root, "Response", cjson_node);
	cJSON_AddStringToObject(cjson_node, "Name", "AddSlaveIP");
	cJSON_AddStringToObject(cjson_node, "RequestMac", res_mac);
	cJSON_AddStringToObject(cjson_node, "DestMac", dst_mac);
	cJSON_AddStringToObject(cjson_node, "SlaveIP", slave_ip);
	cJSON_AddStringToObject(cjson_node, "Result", addr_result[ret]);
	char *s = cJSON_PrintUnformatted(cjson_root);
	if (s) {
		snprintf(res_buf, res_len-1, "%s", s);
		if (g_toggle.jcpinfo == 4) {
			DBG("znpnp_build_add_response = %s\n", res_buf);
		}
		free(s);
	}

	cJSON_Delete(cjson_root);
	return strlen(res_buf);
}

static int znpnp_build_del_response(char *res_mac, char *dst_mac, char *slave_ip, char *res_buf, int res_len)
{
	cJSON *cjson_root = NULL;
	cJSON *cjson_node = NULL;
	unsigned int mac_data[8] = {0};
	char tmp[128] = {0};
	NetEthS eth = {{0,},};
	get_ethinfo(&eth);

	sscanf(eth.mac, "%02X:%02X:%02X:%02X:%02X:%02X", 
		&mac_data[0], &mac_data[1], &mac_data[2], &mac_data[3], &mac_data[4], &mac_data[5]);
	sprintf(tmp, "%02X:%02X:%02X:%02X:%02X:%02X", 
			mac_data[0], mac_data[1], mac_data[2], mac_data[3], mac_data[4], mac_data[5]);

	if (strcmp(tmp, dst_mac)) {
		ERR("Invalid mac %s!\n", dst_mac);
		return 0;
	}

	del_ghost_ip(0);

	cjson_root = cJSON_CreateObject();	
	if (cjson_root == NULL) {
		return -1;
	}

	cjson_node = cJSON_CreateObject();	
	if (cjson_node == NULL) {
		return -1;
	}

	cJSON_AddItemToObject(cjson_root, "Response", cjson_node);
	cJSON_AddStringToObject(cjson_node, "Name", "DelSlaveIP");
	cJSON_AddStringToObject(cjson_node, "RequestMac", res_mac);
	cJSON_AddStringToObject(cjson_node, "DestMac", dst_mac);
	char *s = cJSON_PrintUnformatted(cjson_root);
	if (s) {
		snprintf(res_buf, res_len-1, "%s", s);
		if (g_toggle.jcpinfo == 4) {
			DBG("znpnp_build_del_response = %s\n", res_buf);
		}
		free(s);
	}

	cJSON_Delete(cjson_root);

	return strlen(res_buf);
}

static int znpnp_build_hello(char *res_buf, int res_len)
{
	cJSON *cjson_root = NULL;
	cJSON *cjson_node = NULL;
	unsigned int mac_data[8] = {0};
	char tmp[128] = {0};
	NetEthS eth = {{0,},};
	get_ethinfo(&eth);

	NetPortS netport = {0};
	get_portinfo(&netport);
	
	SysInfoS info = {{0,},};
	get_sysinfo(&info);
	
	sscanf(eth.mac, "%02X:%02X:%02X:%02X:%02X:%02X", 
		&mac_data[0], &mac_data[1], &mac_data[2], &mac_data[3], &mac_data[4], &mac_data[5]);
	sprintf(tmp, "%02X:%02X:%02X:%02X:%02X:%02X", 
			mac_data[0], mac_data[1], mac_data[2], mac_data[3], mac_data[4], mac_data[5]);

	cjson_root = cJSON_CreateObject();	
	if (cjson_root == NULL) {
		return -1;
	}

	cjson_node = cJSON_CreateObject();	
	if (cjson_node == NULL) {
		return -1;
	}

	cJSON_AddItemToObject(cjson_root, "Notify", cjson_node);
	cJSON_AddStringToObject(cjson_node, "Name", "Hello");
	cJSON_AddStringToObject(cjson_node, "Protocol", "ONVIF");
	cJSON_AddStringToObject(cjson_node, "Mac", tmp);
	cJSON_AddStringToObject(cjson_node, "ProtocolUrl", eth.ip);
	
	cJSON_AddNumberToObject(cjson_node, "ProtocolPort", netport.httpport);
	cJSON_AddStringToObject(cjson_node, "DeviceType", "IPC");
	cJSON_AddStringToObject(cjson_node, "DeviceFactory", "JCO");
	cJSON_AddStringToObject(cjson_node, "DeviceName", info.devname);
	char password[36] = {0,};
	SysUserS userinfo;
	memset(&userinfo, 0, sizeof(SysUserS));
	conf_get_usercfg(&userinfo);
	passwd_trans_decode(password, userinfo.user[0].onvifpasswd, strlen(userinfo.user[0].onvifpasswd));

	cJSON_AddStringToObject(cjson_node, "DefaultUser", userinfo.user[0].username);
	cJSON_AddStringToObject(cjson_node, "DefaultPwd", password);
	char *s = cJSON_PrintUnformatted(cjson_root);
	if (s) {
		snprintf(res_buf, res_len-1, "%s", s);
		if (g_toggle.jcpinfo == 4) {
			DBG("znpnp_build_hello = %s\n", res_buf);
		}
		free(s);
	}

	cJSON_Delete(cjson_root);
	return strlen(res_buf);

}

static int znpnp_build_bye(char *res_buf, int res_len)
{
	cJSON *cjson_root = NULL;
	cJSON *cjson_node = NULL;
	unsigned int mac_data[8];
	char tmp[128]={0};
	NetEthS eth = {{0,},};
	get_ethinfo(&eth);
	
	sscanf(eth.mac, "%02X:%02X:%02X:%02X:%02X:%02X", 
		&mac_data[0], &mac_data[1], &mac_data[2], &mac_data[3], &mac_data[4], &mac_data[5]);
	sprintf(tmp, "%02X:%02X:%02X:%02X:%02X:%02X", 
			mac_data[0], mac_data[1], mac_data[2], mac_data[3], mac_data[4], mac_data[5]);

	cjson_root = cJSON_CreateObject();	
	if (cjson_root == NULL) {
		return -1;
	}

	cjson_node = cJSON_CreateObject();	
	if (cjson_node == NULL) {
		return -1;
	}
	cJSON_AddItemToObject(cjson_root, "Notify", cjson_node);
	cJSON_AddStringToObject(cjson_node, "Name", "Bye");
	cJSON_AddStringToObject(cjson_node, "DeviceType", "IPC");
	char *s = cJSON_PrintUnformatted(cjson_root);
	if (s) {
		snprintf(res_buf, res_len-1, "%s", s);
		if (g_toggle.jcpinfo == 4) {
			DBG("znpnp_build_bye = %s\n", res_buf);
		}
		
		free(s);
	}

	cJSON_Delete(cjson_root);
	return strlen(res_buf);
}

static int znpnp_send_cmd(int cmd)
{
	char res_buf[1024] = {0};
	int length = 0;
	int ret = 0;
	
	switch(cmd) {
		case ZN_CMP_TYPE_HELLO:
			length = znpnp_build_hello(res_buf, sizeof(res_buf));
			break;

		case ZN_CMP_TYPE_BYE:
			length = znpnp_build_bye(res_buf, sizeof(res_buf));
			break;

		default:
			return -1;
	}

	if (length > 0) {
		encrypt_json(res_buf, length);
		ret = znpnp_broadcase_sendto(res_buf, length, g_broadcast_fd);
	}
	
	return ret;

}

int znpnp_create_broadcast(int broad_port)
{
	int sock_fd = -1;
	struct sockaddr_in addr;
	int ret = 0;
	int cur_flag = 0;
	
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd < 0) {
		ERR("socket fail\n");
		goto cleanup;
	}

	ret = 1;
	if (-1 == setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(ret))) {
		ERR("socket reuseraddr fail\n");
		goto cleanup;
	}

	ret = 1;
	if (setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &ret, sizeof(ret)))
		DBG("setsockopt fail, errno=%d %s\n", errno, strerror(errno));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(broad_port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		ERR("broadcase bind fail\n");
		goto cleanup;
	}

	cur_flag = fcntl(sock_fd, F_GETFL, 0);
	cur_flag = fcntl(sock_fd, F_SETFL, cur_flag | O_NONBLOCK);

	DBG("broadcase sockfd = %d\n", sock_fd);
	return sock_fd;
	
cleanup:
	if (sock_fd >=0) {
		close(sock_fd);
	}

	return FAILURE;
}

static int znpnp_broadcase_sendto(char *buf, int len, int sockfd)
{
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ZN_PNP_BROADCAST_ADDR);
	addr.sin_port = htons(ZN_PNP_PORT);

	int ret = sendto(sockfd, buf, len, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr));
	return ret;
}

static void znpnp_recv(int socffd, int event, void *client_data)
{
	char buf_recv[ZN_PNP_RECV_BUF] = {0};
	int readbyte;
	struct sockaddr_in addr;
	int addrlen = sizeof(addr);
    char *host;
    int port;
    char szhost[32] = {0};
	
	do {
		memset(buf_recv, 0, sizeof(buf_recv));
		readbyte = recvfrom(g_broadcast_fd, buf_recv, sizeof(buf_recv)-1, 0,
					(struct sockaddr *)&addr, (socklen_t *)&addrlen);

		if (readbyte < 0)
			break;
		
		host = j_inet_ntoa(addr.sin_addr);
		port = ntohs(addr.sin_port);
		strcpy(szhost, host);

		if (g_toggle.jcpinfo == 4) {
			printf("IPCtool.msg:%s:%d:\n", szhost, port);
		}

		znpnp_parse_data(buf_recv, readbyte);
		
	}while(1);
}

static void *start_znpnp_thread(void *data)
{
    struct sigaction sigAction;

    sigAction.sa_handler = SIG_IGN;
    sigAction.sa_flags = 0;
    sigemptyset(&sigAction.sa_mask);
    sigaddset(&sigAction.sa_mask, SIGTERM);
    sigaddset(&sigAction.sa_mask, SIGINT);
    sigaddset(&sigAction.sa_mask, SIGPIPE);
    sigaction(SIGPIPE, &sigAction, NULL);
	
    SYSLOG("thread: start_znpnp_thread,\ttid: %d, pid : %d\n", (int)syscall(SYS_gettid), (int)getpid());
	

	return NULL;
}

int init_znpnp_server(void *scher_data, void *engine_data)
{
	DBG("init_znpnp_server...\n");
	
	g_znpnp_scher = (JSScheduler)scher_data;
	g_znpnp_engine = (JSScheduler)engine_data;

	znpnp_set_motion_alarm();

	g_broadcast_fd = znpnp_create_broadcast(ZN_PNP_PORT);
	if (g_broadcast_fd <= 0) {
		ERR("znpn server fail, fd = %d\n", g_broadcast_fd);
	}

	znpnp_send_cmd(ZN_CMP_TYPE_HELLO);
	
    js_create_reader_r(g_znpnp_scher, g_broadcast_fd, JS_READABLE, znpnp_recv, NULL, &g_znpnp_handle);
	js_create_timer_r(g_znpnp_engine, ZN_PNP_ETH0_CHECK_TIME, ZN_PNP_ETH0_CHECK_TIME, znpnp_eth0_check, NULL, &g_znpnp_engine_handle);
	
	return 0;
}

int uninit_znpnp_server()
{
	znpnp_send_cmd(ZN_CMP_TYPE_BYE);

	js_delete_timer_r(&g_znpnp_engine_handle);

	js_delete_reader_r(&g_znpnp_handle);
	
	if (g_broadcast_fd > 0) {
		close(g_broadcast_fd);
		g_broadcast_fd = -1;
	}

    g_znpnp_scher = NULL;
    g_znpnp_engine = NULL;

	return 0;
}

int get_eth0_0_ip(char* ip)
{
    if (ip == NULL) 
		return -1;

    char result[512] = {0};
	char bcast[128] = {0};
	
    ReadCmdResult((char*)"ifconfig eth0:0", result,512);
    char * str = strstr(result, "inet addr:");
    if(str == NULL) {
		//ERR("eth0:0 get fail\n");
        return -1;
    }

    int iret = sscanf(str, "inet addr:%32[^ ]  Bcast:%32[^ ]  ", ip, bcast);
	if (iret != 2) {
		ERR("get_eth0_0_ip sscanf get fail \n");
		return -1;
	}
	//DBG("ip: %s, Bcast: %s\n", ip, bcast);
    return 0;
}

void znpnp_eth0_check(void *data)
{
	char eth0_0_ip[32] = {0};
	char cmdbuf[128] = {0};
	char result[128] = {0};
	
	int ret = get_eth0_0_ip(eth0_0_ip);
	if (ret < 0) {
		return;
	}
 
	sprintf(cmdbuf, "netstat -atn | grep %s -c", eth0_0_ip);
	ReadCmdResult(cmdbuf, result,128);
	//DBG("result : %s\n", result);

	int num = atoi(result);
	if (num > 0) {
		return;
	}

	del_ghost_ip(0);
}

static int znpnp_set_motion_alarm()
{
	int i = 0;
	MotionDetectS outer = {0};
	MotionDetectS motionxmlcfg = {0};
	conf_get_motiondetectcfg(&motionxmlcfg);
	outer.enable = 1;
	outer.thresh = motionxmlcfg.thresh;
	sprintf(outer.mbdesc, "%s", motionxmlcfg.mbdesc);
	for (i = 0; i < 7; i++) {
		outer.times[i] = motionxmlcfg.times[i];
	}

	conf_set_motiondetectcfg(outer);
	return 0;
}

#endif

