$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf || typeof(lf) =='undefined' || 0 > parseInt(lf))
    {
          parent.location.href = "/login.asp";
    }else{
        GetJCP({cmd: "version -act list", ParseJCP: function(jcpGet){
        if(jcpGet !== "Error"){
            platform = jcpGet.platform.toLowerCase();
            if(platform == 'hb'){
                $("#trAutoIp").hide();
            }
        }}});
        if($.cookie("graintype") == 0){
          $("#litabPppos").hide();
          $("#litabDdns").hide();
          $("#trPpppoeIp").hide();
        }

        if($.cookie("4g") == 1) {
            $("#litabWlan").hide();
            $("#litab4G").show();
        }

        if($.cookie("wifi") == 1) {
            $("#litabWlan").show();
            $("#litab4G").hide();
        }

        if($.cookie("4g") != 1 && $.cookie("wifi") != 1) {
            $("#litabWlan").hide();
            $("#litab4G").hide();
        }

        $("#lahttpen").hide();
        $("#httpen").hide();
        $("#webPort_text").hide();
        
        $("#tabs").tabs();
        GetJCP({cmd: "ethcfg -act list", ParseJCP: ParseEthcfg});
        _init_call();
        $("select").bind("change",function(){
             window.focus();
        });
    }
});

var ports = 80;
var oldip = "192.168.1.217";
function _init_call(){
    $("#tabEthnet").click(function(){
        $("#frmsubmit").attr("src","");
        GetJCP({cmd: "ethcfg -act list", ParseJCP: ParseEthcfg});
        window.focus();
    });
    $("#tabWlan").click(function(){
        $("#frmsubmit").attr("src","");
        GetJCP({cmd: "wificfg -act list", ParseJCP: ParseWlancfg});
        window.focus();
    });
    $("#tab4G").click(function(){
        $("#frmsubmit").attr("src","");
        GetJCP({cmd: "sim4g -act list", ParseJCP: Parse4Gcfg});
        window.focus();
    });
    $("#tabPppos").click(function(){
        $("#frmsubmit").attr("src","");
        GetJCP({cmd: "pppoecfg -act list", ParseJCP: ParsePPPOECfg});
        window.focus();
    });
    $("#tabDdns").click(function(){
        $("#frmsubmit").attr("src","");
        GetJCP({cmd: "ddns3322 -act list", ParseJCP: ParseDdns3322Cfg});
        window.focus();
    });
    $("#tabPort").click(function(){
        $("#frmsubmit").attr("src","");
        GetJCP({cmd: "portcfg -act list", ParseJCP: ParsePortCfg});
        //GetJCP({cmd: "upnpcfg -act list", ParseJCP: ParseUpnpCfg});
        window.focus();
    });
    $("#tabEmail").click(function(){
        $("#frmsubmit").attr("src","");
        GetJCP({cmd: "emailcfg -act list", ParseJCP: ParseEmailCfg});
        window.focus();
    });
        
    $("#tabFtp").click(function(){
        $("#frmsubmit").attr("src","");
        GetJCP({cmd: "ftpclicfg -act list", ParseJCP: ParseFtpcliCfg});
        window.focus();
    });   

    $("#tabNetCheck").click(function(){
        $("#notaddress").val("192.168.1.211");
        $("#notcount").val("4");
        $("#notsize").val("16");
        $("#notresult").val();
        $("#frmsubmit").attr("src","netchecksub.asp");
        window.focus();
    });   
}


var g_4G_mode = 0;
function Parse4Gcfg(jcpobj)
{
    try
    {

        g_4G_mode = jcpobj.online;
        $("input[name='4g_mode_switch'][value=" + parseInt(jcpobj.online) + "]").attr("checked",true);
        $("#sim_info").val(jcpobj.sim==1?IDC_SIM_EXIST:IDC_SIM_NO_EXIST);
        $("#is_4g").val(jcpobj.is4G==1?IDC_SIM_EXIST:IDC_SIM_NO_EXIST);
        
        $("#dbm").val(jcpobj.dbm);
        $("#txBpsec").val(jcpobj.txBpsec);
        $("#rxBpsec").val(jcpobj.rxBpsec);
        $("#4g_ip").val(jcpobj.ip);

        refreshWlanStatus();
    }
    catch(e){ return e; }
}

function Set4GMode(mode) {
    if (mode != g_4G_mode) {
        g_4G_mode = mode;
        GetJCP({cmd: "sim4g -act set -online " + mode, ParseJCP: function(jcpstr)
        {
            if(jcpstr != "Error")
            {
                alert(IDC_MSGBOX_SAVEOK);
                window.focus();
            }
            else
            {
                alert(IDC_MSGBOX_SAVEFAIL);
                window.focus();
            }
        }});
    }
}

function refresh4GStatus(){
    GetJCP({cmd: "sim4g -act list", ParseJCP: Parse4Gcfg});
    window.focus();
}



//无线配置
function showIPSetBlock(flag){
    if(flag){
        $("#tr1").show();
        $("#tr2").show();
        $("#tr3").show();
        //$("#tr4").show();
        $("#tr0").hide();
    }else{
        $("#tr1").hide();
        $("#tr2").hide();
        $("#tr3").hide();
        //$("#tr4").hide();
        $("#tr0").show();
    }
}

function SearchWLAN(){
    $("#wlsnTable div").remove();
    $("#btnSearchWlan").prop("disabled",true);
    GetJCPList({cmd: "wifilist -act list", timeout:4000, ParseJCP: function(jcpGet){
        if(jcpGet != 'Error'){
            var jcpArr = jcpGet.split(";");
            var dataArr = [];
            var len = jcpArr.length;
            if(len > 0){
                for(var i=0;i<len;i++){
                    if(jcpArr[i] != ''){
                        var subData = jcpArr[i].split("=");
                        var level = subData[1]>=85?3:subData[1]>=50?2:1;
                        dataArr.push('<div>');
                        dataArr.push('<label onclick="clickWlanRadio(\''+subData[0]+'\',\''+subData[2]+'\',\''+subData[3]+'\')">'+subData[0]+'</label>');
                        dataArr.push('<img src="/image/w'+level+'.png"');
                        dataArr.push('</div>');
                    }
                }
                if(dataArr.length > 0){
                    $("#wlsnTable").append(dataArr.join("")); 
                } 
            }else{
                $("#wlsnTable").append("<div>"+IDC_SYSLOG_NODATA+"</div>");
            }
            $("#btnSearchWlan").prop("disabled",false); 
        }else{
            $("#btnSearchWlan").prop("disabled",false); 
        }
    }});  
    
}

function clickWlanRadio(id,wepauthtype,encrypttype){
    $("#wlan").val(id);
    $("#wepauthtype").val(wepauthtype);
    $("#encrypttype").val(encrypttype);
    $("#wlanpassword").val("");
    $("#wlanmode").val(1);
}

function switchWlanPwdShow() {
    var flag = $('#showWlanPwd').prop('checked');
    if (flag) {
        $("#wlanpassword").get(0).setAttribute("type","text");
    } else {
        $("#wlanpassword").get(0).setAttribute("type","password");
    }
}

function ParseWlancfg(jcpobj)
{
    try
    {
        $("#wlanmode").val(jcpobj.mode);
        
        $("#wlanip").val(jcpobj.ip);
        $("#wlanip").ipaddress({cidr:false});
    
        $("#wlanmask").val(jcpobj.mask);
        $("#wlanmask").ipaddress({cidr:false});
        
        $("#wlangate").val(jcpobj.gw);
        $("#wlangate").ipaddress({cidr:false});
        
        $("#iptypesel").val(jcpobj.dhcp);
        $("#wlan").val(jcpobj.ssid);
        $("#wlanpassword").val(jcpobj.weppasswd);

        $("#wlanmacaddr").val(jcpobj.mac);
        
        showIPSetBlock(jcpobj.dhcp==0?true:false);

        refreshWlanStatus();
    }
    catch(e){ return e; }
}

function refreshWlanStatus(){
    GetJCP({cmd: "wifilist -act status", ParseJCP: function(jcpobj){
        if(jcpobj != 'Error'){
            var ss = parseInt(jcpobj.status)==5?IDC_PPPOE_STATUS_OK:IDC_PPPOE_STATUS_ERR;
            if (jcpobj.ip_address == '' || jcpobj.ip_address.length == 0) {
                ss = IDC_PPPOE_STATUS_ERR;
            }
            $("#wlanStatus").html(ss);
            $("#wlandhcpip").val(jcpobj.ip_address);
        }
    }});
}

function SaveWlan()
{
    var mode = $("#wlanmode").val();
    var wlan = $("#wlan").val();
    var wlanpassword = $("#wlanpassword").val();
    //var ip = $("#wlanip").getip();
    //var submask = $("#wlanmask").getip();
    //var gateway = $("#wlangate").getip();
    //var iptype = $("#iptypesel").val();

    if(wlan == ''){
        alert(IDC_WLAN_NOEMPTY);
        window.focus();
        return;
    }

    /*if(wlanpassword == ''){
        alert(IDC_WLAN_PWD_NOEMPTY);
        window.focus();
        return;
    }

    if(wlanpassword.length < 8){
        alert(IDC_WLAN_PASSWORD_LENGTH);
        window.focus();
        return;
    }*/

    //判断DNS , IP , SUBMASK, GATEWAY是否为0-255之间的数
    /*if($("#iptypesel").val() == 0){
        for (var i = 0; i < 4; i ++)
        {
            if (ip.split(".")[i]=='' || ip.split(".")[i] > 255 || ip.split(".")[i] < 0
                ||submask.split(".")[i]=='' || submask.split(".")[i] > 255 || submask.split(".")[i] < 0
                ||gateway.split(".")[i]=='' ||gateway.split(".")[i] > 255 || gateway.split(".")[i] < 0)
            {
                alert(IDC_MSGBOX_ADDRESS_NUM);
                window.focus();
                return;
            }
        }
        
        //判断IP地址与网关是否在同一网段
        var ret = Ip_Gateway_Chk(ip, gateway, submask);
        if (ret == false)
        {
            //不在同一网段，直接返回
            alert(IDC_MSGBOX_IPGATE_DIFFER);
            window.focus();
            return;
        }
    }
    var ipad = ip.split(".");
    if(ipad[3] == "")
    {
        alert(IDC_IP_ADD);
        window.focus();
        return;
    }*/

    try
    {
        var jcpstr = "wificfg -act set -mode " + mode;
        jcpstr += " -ssid " + "\"" + wlan + "\"";
        jcpstr += " -weppasswd " + "\"" + wlanpassword + "\"";
        //jcpstr += " -ip " + ip;
        //jcpstr += " -mask " + submask;
        //jcpstr += " -gw " + gateway;
        //jcpstr += " -dhcp " + iptype;
        //jcpstr += " -wepauthtype " +$("#wepauthtype").val();
        //jcpstr += " -encrypttype " + $("#encrypttype").val() ;
        

        GetJCP({cmd: jcpstr, ParseJCP: function(jcpstr)
        {
            if(jcpstr != "Error")
            {
                alert(IDC_MSGBOX_SAVEOK);
                window.focus();
            }
            else
            {
                alert(IDC_MASK_ERROR);
                window.focus();
            }
        }, async: false});
    }
    catch(E){ return E; }
}

/*====================================================================
    ETHNET
====================================================================*/
function ParseEthcfg(jcpobj)
{
    try
    {
        $("#dns").val(jcpobj.dns);
        $("#dns").ipaddress({cidr:false});
        
        $("#ip").val(jcpobj.ethip);
        $("#ip").ipaddress({cidr:false});
        oldip = jcpobj.ethip;
    
        $("#mask").val(jcpobj.ethmask);
        $("#mask").ipaddress({cidr:false});
        
        $("#gate").val(jcpobj.ethgw);
        $("#gate").ipaddress({cidr:false});
        
        $("#macaddr").val(jcpobj.ethmac);

        $("#ethmtu").val(jcpobj.ethmtu);
        
        $("#dhcpSwitch").prop("checked", jcpobj.ethdhcp==1?true:false);
        $("#autoipSwitch").prop("checked", jcpobj.ipadaen==1?true:false);
        $("#nstdReticle").prop("checked", jcpobj.nreticle==1?true:false);
        
        switchDhcp();
    }
    catch(e){ return e; }
}

function SaveEthnet()
{
    var dnsaddr = $("#dns").getip();
    var ip = $("#ip").getip();
    var submask = $("#mask").getip();
    var gateway = $("#gate").getip();
    var ethmtu  = $("#ethmtu").val();
    var dhcpen = $('#dhcpSwitch').prop('checked')?1:0;
    var autoipen = $('#autoipSwitch').prop('checked')?1:0;
    var nreticle = $('#nstdReticle').prop('checked')?1:0;
    //判断DNS , IP , SUBMASK, GATEWAY是否为0-255之间的数
    for (var i = 0; i < 4; i ++)
    {
        if (dnsaddr.split(".")[i]=='' || dnsaddr.split(".")[i] > 255 || dnsaddr.split(".")[i] < 0
            ||ip.split(".")[i]=='' || ip.split(".")[i] > 255 || ip.split(".")[i] < 0
            ||submask.split(".")[i]=='' || submask.split(".")[i] > 255 || submask.split(".")[i] < 0
            ||gateway.split(".")[i]=='' ||gateway.split(".")[i] > 255 || gateway.split(".")[i] < 0)
        {
            alert(IDC_MSGBOX_ADDRESS_NUM);
            window.focus();
            return;
        }
    }
    
    //判断IP地址与网关是否在同一网段
    var ret = Ip_Gateway_Chk(ip, gateway, submask);
    if (ret == false)
    {
        //不在同一网段，直接返回
        alert(IDC_MSGBOX_IPGATE_DIFFER);
        window.focus();
        return;
    }
    var ipad = ip.split(".");
    if(ipad[3] == "")
    {
        alert(IDC_IP_ADD);
        window.focus();
        return;
    }

    try
    {
       var jcpstr = "ethcfg -act set -ethip " + ip + " -ethmask " + submask + " -ethgw " + gateway + " -dns " + dnsaddr +" -ethdhcp " + dhcpen + " -ipadaen "+autoipen + " -nreticle "+nreticle + " -ethmtu "+ethmtu;
        GetJCP({cmd: jcpstr, ParseJCP: function(jcpstr)
        {
            if(jcpstr != "Error")
            {
                alert(IDC_MSGBOX_SAVEOK);
                window.focus();
                var ipaddr = (document.URL.split('//')[1]).split('/')[0].split(':')[0];
                if (ipaddr == oldip && oldip != ip)
                {
                    parent.location.href = "http://" + ip + ":" + ports;
                }
            }
            else
            {
                alert(IDC_MASK_ERROR);
                window.focus();
            }
        }, async: false});
    }
    catch(E){ return E; }
}

//判断页面与网关的地址是否在同一网段functions 方法：
//IP and 掩码  是否等于 网关 and 掩码
function Ip_Gateway_Chk(ip, gateway, submask)
{
    var ip = ip.split(".");
    var gateway = gateway.split(".");
    var submask = submask.split(".");
    var ip_sub = new Array(), gateway_sub = new Array();
    
    for (var i = 0; i < ip.length; i ++)
    {
        ip_sub[i] = ip[i] & submask[i];
        gateway_sub[i] = gateway[i] & submask[i];
    }
    
    for (var i = 0; i < ip_sub.length; i ++)
    {
        if (ip_sub[i] != gateway_sub[i]) return false;
    }
    return true;
}

function switchDhcp(){
    var flag = $('#dhcpSwitch').prop('checked');
    $("#ip").enableip({enable:!flag});
    $("#mask").enableip({enable:!flag});
    $("#gate").enableip({enable:!flag});
    if(flag){
        $('#autoipSwitch').prop('checked',false);
    }
}

function switchAutoIp(){
    var flag = $('#autoipSwitch').prop('checked');
    if(flag){
        $('#dhcpSwitch').prop('checked',false);
        //alert(IDC_AUTOIP_TIP);
        switchDhcp();
    }
}

/*====================================================================
    PPPOE 
====================================================================*/
function pppoeEnable(flag)
{
    $("#nicsel").attr("disabled", !flag);
    $("#pppoeuser").attr("disabled", !flag);
    $("#pppoepasswd").attr("disabled", !flag);
}

function ParsePPPOECfg(jcpobj){
    try
    {
        var pppoeen =  parseInt(jcpobj.pppoeen);
       
        $("input[name='pppoeswitch'][value=" + pppoeen + "]").attr("checked",true);
        if (pppoeen == 1)
        {
            pppoeEnable(true);
        }
        else
        {
            pppoeEnable(false);
        }
       
        $("#nicsel").val(jcpobj.nicsel);
        $("#pppoeuser").val(jcpobj.pppoeuser);
        $("#pppoepasswd").val(jcpobj.pppoepasswd);
        
        if(jcpobj.pppoe_ip){
            $("#pppoeip").html("&nbsp;&nbsp;"+jcpobj.pppoe_ip);
        }else{
             $("#pppoeip").html("");
        }
    
        var ss = parseInt(jcpobj.status)==1?IDC_PPPOE_STATUS_OK:IDC_PPPOE_STATUS_ERR;
        $("#pppoestatus").html("&nbsp;&nbsp;"+ss);
    }
    catch(e){ }
}

function FlushPppoe(){
     GetJCP({cmd: "pppoecfg -act list", ParseJCP: ParsePPPOECfg});
     window.focus();
}

function SavePppoe(){
    //网卡类型，0有线，1无线
    var nictype = $("#nicsel").val();
    var pppoeen = $('input:radio[name="pppoeswitch"]:checked').val();
    var pppoeuser = $("#pppoeuser").val();
    var pppoepasswd = $("#pppoepasswd").val();
    
    if (isBlank(pppoeuser))
    {
        alert(IDC_GEN_USER_NOEMPTY);
        window.focus();
        return 1;
    }
    
    if (isBlank(pppoepasswd))
    {
        alert(IDC_GEN_PASSWORD_NOEMPTY);
        window.focus();
        return 1;
    }
    
    try
    {
        var jcpstr = "pppoecfg -act set -nicsel " + nictype + " -pppoeen " + pppoeen + " -pppoeuser " + pppoeuser + " -pppoepasswd " + pppoepasswd;
        GetJCP({cmd: jcpstr});
        
        alert(IDC_MSGBOX_SAVEOK);
        window.focus();  
        return 0;
    }
    catch(e){ return e; }
}

/*====================================================================
    DDNS 
====================================================================*/
function ParseDdns3322Cfg(jcpobj){
    try{
        $("#ddnssupport").val("3322");
        $("#3322addr").show();
        $("input[name='ddnsswitch'][value=" + parseInt(jcpobj.ddnsen) + "]").attr("checked",true);
        $("#ddnsusername").val(jcpobj.ddnsuser);
        $("#ddnspassword").val(jcpobj.ddnspasswd);
        $("#ddnsaddrs").val(jcpobj.ddnsprovider);
        var ss = parseInt(jcpobj.status)==1?IDC_9299DDNS_OK:IDC_9299DDNS_ERR;
        $("#ddnsstatus").html("&nbsp;&nbsp;"+ss);
        if(parseInt(jcpobj.ddnsen)==0){
            ddnsEnable(false);
        }else{
            ddnsEnable(true);
        }
            
    }catch(e){}
}

function ParseDdns9299Cfg(jcpobj){
    try{
        $("#3322addr").hide();
        $("input[name='ddnsswitch'][value=" + parseInt(jcpobj.enable) + "]").attr("checked",true);
        $("#ddnsusername").val(jcpobj.user);
        $("#ddnspassword").val(jcpobj.password);
        var ss = parseInt(jcpobj.status)==1?IDC_9299DDNS_OK:IDC_9299DDNS_ERR;
        $("#ddnsstatus").html("&nbsp;&nbsp;"+ss);
        if(parseInt(jcpobj.enable)==0){
            ddnsEnable(false);
        }else{
            ddnsEnable(true);
        }
    }catch(e){}
}

function changeDdnsSupport(){
    var ddnssupport = $("#ddnssupport").val();
    if("3322" == ddnssupport){
        GetJCP({cmd: "ddns3322 -act list", ParseJCP: ParseDdns3322Cfg});
    }else if("9299" == ddnssupport){
        GetJCP({cmd: "ddns9299 -act list", ParseJCP: ParseDdns9299Cfg});
    }
}

function SaveDdns(){
    var ddnssupport = $("#ddnssupport").val();
    var user = $("#ddnsusername").val();
    var password = $("#ddnspassword").val();
    var on = parseInt($('input:radio[name="ddnsswitch"]:checked').val());
    var address = $("#ddnsaddrs").val();
    if("3322" == ddnssupport){
        var jcpstr = "ddns3322 -act set -ddnsen "+on+" -ddnsuser "+user;
        jcpstr += " -ddnspasswd "+password+" -ddnsprovider "+address;
        GetJCP({cmd: jcpstr});
        if(on == 1){
            GetJCP({cmd: "ddns9299 -act set -enable 0"});
        }
    }else if("9299" == ddnssupport){
        var jcpstr = "ddns9299 -act set -enable " + on + " -user " + user + " -password " + password;
        GetJCP({cmd: jcpstr});
        if(on == 1){
            GetJCP({cmd: "ddns3322 -act set -ddnsen 0"});
        }
    }
    alert(IDC_MSGBOX_SAVEOK);
    window.focus();  
}

function ddnsEnable(flag)
{
    $("#ddnsusername").attr("disabled", !flag);
    $("#ddnspassword").attr("disabled", !flag);
    if("3322" == $("#ddnssupport").val()){
        $("#ddnsaddrs").attr("disabled", !flag);
    }
}

/*====================================================================
    Port 
====================================================================*/
var oldrtspport = 554;
var oldwebport = 80;
function ParsePortCfg(jcpobj){
    $("#porthttp").val(jcpobj.web);
    oldwebport = parseInt(jcpobj.web);
    
    $("#portrtsp").val(jcpobj.rtsp);
    oldrtspport = parseInt(jcpobj.rtsp);
    
    $("#portvoice").val(jcpobj.audio);
    $("#portupdate").val(jcpobj.update);
    $("#portftp").val(jcpobj.ftp);

    
    $("#webPort_text").text(jcpobj.web_upnp);
    $("#ftpPort_text").text(jcpobj.ftp_upnp);
    $("#rtspPort_text").text(jcpobj.rtsp_upnp);
    $("#speakPort_text").text(jcpobj.audio_upnp);
    $("#updatePort_text").text(jcpobj.update_upnp);

    Set_cookie("webport",jcpobj.web);
    Set_cookie("ftport",jcpobj.ftp);
}

function SavePort(){
    var rtspport = $("#portrtsp").val();
    var webport = $("#porthttp").val();
    var ftpport = $("#portftp").val();
    var voiceport = $("#portvoice").val();
    var updateport = $("#portupdate").val();
    //判断参数是否符合要求
    if (isBlank(rtspport) || rtspport > 65535 || 1 > rtspport)
    {
        alert("Rtsp " + IDC_GEN_PORT_RANGE);
        window.focus();
        return false;
    }
    if (isBlank(webport) || webport > 65535 || 1 > webport)
    {
        alert("Web " + IDC_GEN_PORT_RANGE);
        window.focus();
        return false;
    }
    if (isBlank(ftpport) || ftpport > 65535 || 1 > ftpport)
    {
        alert("Ftp " + IDC_GEN_PORT_RANGE);
        window.focus();
        return false;
    }
    if (isBlank(voiceport) || voiceport > 65535 || 1 > voiceport)
    {
        alert("Speak " + IDC_GEN_PORT_RANGE);
        window.focus();
        return false;
    }
    if (isBlank(updateport) || updateport > 65535 || 1 > updateport)
    {
        alert("Update " + IDC_GEN_PORT_RANGE);
        window.focus();
        return false;
    }

     //判断任意的两个端口号是否相同
    var ArrPort = new Array(webport, ftpport, rtspport, voiceport, updateport);

    if (!PortCheck(ArrPort))
    {
        alert(IDC_SAME_PORT_MSG);
        window.focus();
        return false;
    }

    Set_cookie("webport",webport);
    Set_cookie("ftport",ftpport);
    
    try
    {
        var jcpstr = "portcfg -act set" + " -rtsp " + rtspport + " -web " + webport + " -ftp " + ftpport + " -audio " + voiceport + " -update " + updateport;
        GetJCP({cmd: jcpstr});
        
        /*var jcpset = "upnpcfg -act set" + " -rtspen " + (document.all.rtspen.checked == true ? 1 : 0) + " -httpen " + (document.all.httpen.checked == true ? 1 : 0)
                    + " -ftpen " + (document.all.ftpen.checked == true ? 1 : 0) + " -voiceen " + (document.all.voiceen.checked == true ? 1 : 0)
                    + " -updateen " + (document.all.updateen.checked == true ? 1 : 0);
        GetJCP({cmd: jcpset});*/
        alert(IDC_MSGBOX_SAVEOK);
        window.focus();
        var ipaddr = (document.URL.split('//')[1]).split('/')[0].split(':')[0];
        if (parseInt(oldrtspport) != parseInt(rtspport))
        {
            parent.location.href = "http://" + ipaddr + ":" + webport;;
        }
        if (parseInt(oldwebport) != parseInt(webport))
        {
            parent.location.href = "http://" + ipaddr + ":" + webport;
        }
        return true;
    }
    catch(e){ }    

}

//检查端口号是否相同
function PortCheck(arrayport)
{
    for (var i = 0; i < arrayport.length; i++)
    {
        for (var j = i + 1; j < arrayport.length; j++)
        {
            if (arrayport[i] == arrayport[j]) return false;
        }
    }
    return true;
}

//checkbox点击后说触发的事件
function webPort(){
    if($("#httpen").is(":checked"))
    {
            $("#webPort_text").show();
    }
    else
    {
          $("#webPort_text").hide();
    }
}

function frpPort(){
    if($("#ftpen").is(":checked"))
    {
            $("#ftpPort_text").show();
    }
    else
    {
            $("#ftpPort_text").hide();
    }
}


function rtspPort(){
    if($("#rtspen").is(":checked"))
    {
            $("#rtspPort_text").show();
    }
    else
    {
            $("#rtspPort_text").hide();
    }
}

function speakPort(){
    if($("#voiceen").is(":checked"))
    {
            $("#speakPort_text").show();
    }
    else
    {
            $("#speakPort_text").hide();
    }
}

function updatePort(){
    if($("#updateen").is(":checked"))
    {
            $("#updatePort_text").show();
    }
    else
    {
            $("#updatePort_text").hide();
    }
}

function  ParseUpnpCfg(jcpobj){
    if(parseInt(jcpobj.httpen)==1){
        $("#httpen").prop("checked",true);
        $("#webPort_text").show();
    }

    if(parseInt(jcpobj.rtspen)==1){
        $("#rtspen").prop("checked",true);
        $("#rtspPort_text").show();
    }

     if(parseInt(jcpobj.ftpen)==1){
        $("#ftpen").prop("checked",true);
        $("#ftpPort_text").show();
    }

     if(parseInt(jcpobj.voiceen)==1){
        $("#voiceen").prop("checked",true);
        $("#speakPort_text").show();
    }

     if(parseInt(jcpobj.updateen)==1){
        $("#updateen").prop("checked",true);
        $("#updatePort_text").show();
    }
    
}

//--------------------------------邮件设置
function ParseEmailCfg(jcpobj){
    try{
        $("#emailserver").val(jcpobj.smtpserver);
        $("#emailuser").val(jcpobj.smtpuser);
        $("#emailpasswd").val(jcpobj.smtppasswd);
        $("#emailtoaddr").val(jcpobj.toaddr);
    }catch(e){
    }
}

function SaveEmail(){
    var email = $("#emailtoaddr").val();
    var emailserver = $("#emailserver").val();
    var user = $("#emailuser").val()
    var pwd = $("#emailpasswd").val()
    if (isBlank(email))
    {
        alert(IDC_EMAIL_DEST_MSG_BLANK);//邮件参数目的
        window.focus();
        return false;
    }
    if (isBlank(emailserver))
    {
        alert(IDC_EMAIL_SERVER_MSG_BLANK);//邮件参数服务器
        window.focus();
        return false;
    }

    var myRegExp = /["'<>%;)(&+“”\[\]!！？?\\#￥$^；【】。]/;
    if(myRegExp.test(emailserver)){
          alert(IDC_EMAIL_SERVER+IDC_NOT_CONTAIN+"\"'<>%;)(&+“”[]!！？?\\#￥$^；【】。");
          window.focus();
          return;
    }
    if(myRegExp.test(user)){
          alert(IDC_EMAIL_USERNAME+IDC_NOT_CONTAIN+"\"'<>%;)(&+“”[]!！？?\\#￥$^；【】。");
          window.focus();
          return;
    }
    if(myRegExp.test(pwd)){
          alert(IDC_EMAIL_PASSWORD+IDC_NOT_CONTAIN+"\"'<>%;)(&+“”[]!！？?\\#￥$^；【】。");
          window.focus();
          return;
    }

    if(myRegExp.test(email)){
          alert(IDC_EMAIL_TOADDR+IDC_NOT_CONTAIN+"\"'<>%;)(&+“”[]!！？?\\#￥$^；【】。");
          window.focus();
          return;
    }

    if (email.getBytes() > 63)
    {
        alert(IDC_EMAIL_DEST_MSG_RANGE);//Email目的地址
        window.focus();
        return false;
    }
    if (emailserver.getBytes() > 63)
    {
        alert(IDC_EMAIL_SERVER_MSG_RANGE);//Email服务器
        window.focus();
        return false;
    }

    //验证EMAIL地址的合法性
    var EmailRegExp = new RegExp("^\\w+([-+.]\\w+)*@\\w+([-.]\\w+)*\\.\\w+([-.]\\w+)*$","g");
    if (EmailRegExp.test(email) == false)
    {
        alert(IDC_EMAIL_CHECK_FAIL);
        window.focus();
        return false;
    }
    
    //验证邮件服务器,形如:xxxx.xyz.xxx或Ip地址
    var UrlRegExp = new RegExp("([\\w-]+\\.)+[\\w-]+(/\\[\\w- ./?%&=\\]*)?","g");
    if (UrlRegExp.test(emailserver) == false)
    {
        alert(IDC_SERVER_CHECK_FAIL);
        window.focus();
        return false;
    }

    jcpstr = "emailcfg -act set -smtpserver " + emailserver + " -smtpuser " + $("#emailuser").val() + 
                " -smtppasswd " + $("#emailpasswd").val() + " -toaddr " + $("#emailtoaddr").val();
    //GetJCP({cmd: jcpstr});

    GetJCP({cmd: jcpstr, ParseJCP: function(jcpobj){
           if(jcpobj!='Error'){
                setTimeout(test_mail,700); 
                alert(IDC_MSGBOX_SAVEOK);
            } else {
                alert(IDC_MSGBOX_SAVEFAIL);
            }
    }});

    //alert(IDC_MSGBOX_SAVEOK);
    window.focus();

}

function test_mail() {
    GetJCP({cmd: "alarmtest -act set -alarmtype 13", timeout:6000, ParseJCP: function(jcpobj){
        if(jcpobj!='Error'){
            var res = parseInt(jcpobj.result);
            switch(res) {
                case 0:
                    alert(IDC_EMAIL_TEST_SUCC);
                    break;
                case 1:
                    alert(IDC_EMAIL_FAIL_UNKNOW);
                    break;
                case 2:
                    alert(IDC_EMAIL_FAIL_SVR);
                    break;
                case 3:
                    alert(IDC_EMAIL_FAIL_SSL);
                    break;
                case 4:
                    alert(IDC_EMAIL_FAIL_USER_PASSWD);
                    break;
                case 5:
                    alert(IDC_EMAIL_FAIL_ANTI_SPAM);
                    break;
            }

        } else {
            alert(IDC_MSGBOX_SAVEFAIL);
        }
    }});
}

//-----------------------Ftp Client
function ParseFtpcliCfg(jcpobj){
    try
    {
        $("#ftpserver").val(jcpobj.host);
        $("#ftpuser").val(jcpobj.user);
        $("#ftppasswd").val(jcpobj.passwd);
        $("#ftppath").val(jcpobj.path);
        //$("input[name='ftptype'][value=" + parseInt(jcpobj.type) + "]").attr("checked",true);
    }
    catch(e){}
}

function SaveFtp(){
    var ftpserver = $("#ftpserver").val();
    var ftppath = $("#ftppath").val();
    var user = $("#ftpuser").val();
    var pwd = $("#ftppasswd").val();

    //验证FTP服务器的合法性，形如:xxxx.xyz.xxx或Ip地址
    if (isBlank(ftpserver))
    {
        alert(IDC_FTP_SERVER_MSG_BLANK);//ftp服务器：
        window.focus();
        return false;
    }

    var myRegExp = /["'<>%;)(&+“”\[\]!！？?\\#￥$^；【】。]/;
    if(myRegExp.test(ftpserver)){
          alert(IDC_FTPCLI_SERVER+IDC_NOT_CONTAIN+"\"'<>%;)(&+“”[]!！？?\\#￥$^；【】。");
          window.focus();
          return;
    }
    if(myRegExp.test(user)){
          alert(IDC_EMAIL_USERNAME+IDC_NOT_CONTAIN+"\"'<>%;)(&+“”[]!！？?\\#￥$^；【】。");
          window.focus();
          return;
    }
    if(myRegExp.test(pwd)){
          alert(IDC_FTPCLI_PASSWORD+IDC_NOT_CONTAIN+"\"'<>%;)(&+“”[]!！？?\\#￥$^；【】。");
          window.focus();
          return;
    }

    if(myRegExp.test(ftppath)){
          alert(IDC_FTPCLI_PATH+IDC_NOT_CONTAIN+"\"'<>%;)(&+“”[]!！？?\\#￥$^；【】。");
          window.focus();
          return;
    }


     if (ftpserver.getBytes() > 63)
    {
        alert(IDC_FTP_SERVER_MSG_RANGE);//ftp服务器
        window.focus();
        return false;
    }
   
    if(ftppath.getBytes()>63){
        alert(IDC_FTP_ROUTE_MSG_RANGE);
        window.focus();
        return false;
    }
    
    var UrlRegExp = new RegExp("([\\w-]+\\.)+[\\w-]+(/\\[\\w- ./?%&=\\]*)?","g");
    var IpRegExp = new RegExp("^(\\d{1,2}|1\\d\\d|2[0-4]\\d|25[0-5])\\.(\\d{1,2}|1\\d\\d|2[0-4]\\d|25[0-5])\\.(\\d{1,2}|1\\d\\d|2[0-4]\\d|25[0-5])\\.(\\d{1,2}|1\\d\\d|2[0-4]\\d|25[0-5])$");
    if (UrlRegExp.test(ftpserver) == false && IpRegExp.test(ftpserver) == false)
    {
        alert(IDC_SERVER_CHECK_FAIL);
        window.focus();
        return false;   
    }

    jcpstr = "ftpclicfg -act set  -host " + $("#ftpserver").val() + " -user " + $("#ftpuser").val() +
                " -passwd " + $("#ftppasswd").val() + " -path " + ftppath;
    GetJCP({cmd: jcpstr});
       
    alert(IDC_MSGBOX_SAVEOK);
    window.focus();
    return true;
}

//---------------网络测试
var pingsetaddr ,pingsetsize , pingsetcnt;
var pingcnt = 0, pingdelay = 0;

function PingSubmit()
{
    window.clearTimeout(pingdelay);
    
    $("#frmsubmit").contents().find("#address").val(pingsetaddr);
    $("#frmsubmit").contents().find("#size").val(pingsetsize);
    pingcnt--;
    
    $("#frmsubmit").contents().find("#netcheck").submit();
}

function Ping()
{
    if (false == IPCheck())
    {
        window.focus();
        return false;
    }
    $("#notresult").val();
    
    pingmsg = "PING " + pingsetaddr + " " + pingsetsize + " bytes of data.;"
    pingactive = 1;
    
    PingSubmit();
}


function IPCheck()
{
    //检查传递参数的合法性
    var address = $("#notaddress").val();
    address = address.replace( /(^\s*)|(\s*$)/g, '');
    if (isBlank(address))
    {
        alert(IDC_NETCHECK_ADDR_FAIL);
        window.focus();
        return false;
    }
   /* var IpRegExp = new RegExp("^(\\d{1,2}|1\\d\\d|2[0-4]\\d|25[0-5])\\.(\\d{1,2}|1\\d\\d|2[0-4]\\d|25[0-5])\\.(\\d{1,2}|1\\d\\d|2[0-4]\\d|25[0-5])\\.(\\d{1,2}|1\\d\\d|2[0-4]\\d|25[0-5])$");
    if(!IpRegExp.test(address)){
        alert(IDC_IP_ADD);
        window.focus();
        return false;
    }*/

    var myRegExp = new RegExp("^\\d+$");        //匹配数字
    var count = $("#notcount").val();
    if ((myRegExp.test(count) == false) || parseInt(count) > 20 || parseInt(count) < 1)
    {
        alert(IDC_NETCHECK_COUNT + ":(1~20)");
        window.focus();
        return false;
    }
    
    var size = $("#notsize").val();
    if ((myRegExp.test(size) == false) || parseInt(size) > 1472 || parseInt(size) < 8)
    {
        alert(IDC_NETCHECK_SIZE + ":(8~1472)");
        window.focus();
        return false;
    }
        
    pingsetaddr = address;
    pingcnt = count;
    pingsetcnt = count;
    pingsetsize = size;
    window.focus();
    return true;
}

var $w = $(window).width();
if($w < 1200){
   $("body").css("width",1200);
}
$(window).resize(function(){
    var $w = $(window).width();
    if($w < 1200){
       $("body").css("width",1200);
    }else{
       $("body").css("width",'99%');
    }
});
