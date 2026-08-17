#ifndef __UPDATE_SERVICE__H__
#define __UPDATE_SERVICE__H__

#include "js_scheduler.h"

typedef struct {
    unsigned short cmd;
    unsigned short length;
} UPDATE_CMD_HEAD_S;

typedef struct {
    unsigned int size;
    unsigned int mtime;
} UPDATE_FILE_INFO_S;


class JUpdateService
{
public:
    JUpdateService(JSScheduler sch);
    ~JUpdateService();

    void RestartUpdateService();
    void StartService();

private:
    static void incomingUpatehandle(int fd, int events, void *instance);
    void doIncomingUpdatehandle();
    int initUpdateService();
    static void  DoinitUpdateService(void *instance);
    void uninitUpdateService();

    static void  incomingUpatePkghandle(int fd, int events, void *instance);
    void doIncomingUpdatePkghandle();

    int chk_breakpoint(char *buf, int len, UPDATE_FILE_INFO_S *p_info, struct stat *p_st);
    int chk_header(char *buf);

private:
    JSScheduler             sch_update;
    JSTCHandle              soc_update;
    JSTCHandle              hdl_unpack;
    JSTCHandle              hdl_reboot;
    int                     fSockFd;
    int                     fUpdateSockFd;
    int                     fUpdateConNum;
    int                     upgrade_type;

    int m_filefd;

    int m_iRecvDataLen;
    char m_databuf[64*1024];

    unsigned int m_fileRecvLen;
    bool m_checkDataHead;
    bool m_breakpointTrans;
    bool m_bStartUpdate;

    UPDATE_FILE_INFO_S m_fileInfo;
};

#endif

