$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf || typeof(lf) =='undefined' || 0 > parseInt(lf))
    {
       self.location.href = "/login.asp";
    }else{
        $("#tabGuobiao").click(function(){
            initGB();
            window.focus();
        });

        $("#tabHxht").click(function(){
            initHxht();
            window.focus();
        });

        $("#tabHngs").click(function(){
           initHngs();
           window.focus();
        });

        $("#tabTslive").click(function(){
            initTslive();
            window.focus();
        });

        $("#tabP2p").click(function(){
            initP2p();
            window.focus();
        });

        $("#tabJstar").click(function(){
            initJstar();
            window.focus();
        });

        $("#tabCloudplatform").click(function(){
            initCloudPlatform();
            window.focus();
        });
        
        $("#tabWstk").click(function(){
            initWstk();
            window.focus();
        });

        $("#tabs").tabs();

        var platform =  GetCookieByKey("platform");
        var arrPF = platform.split(",");
        for(var i=0,l=arrPF.length;i<l;i++){
            var str = transferStrFirstCharToUpper(arrPF[i]);
            $("#li"+str).show();
            if(i == 0){
                $("#tab"+str).click();
            }
        }

        $("select").bind("change",function(){
             window.focus();
        }); 
        
    }
});

function initP2p(){
    try
    {
       var ret = null;
       GetJCPList({cmd: "tutkcfg -act list", ParseJCP: function(result){
            ret = GetRtspKeyStr(result, 'enable');
            disabledPwdTutk(parseInt(ret));
            $("input[name='tutkswitch'][value='" + parseInt(ret) + "']").attr("checked",true);

            ret = GetRtspKeyStr(result, 'id');
            $("#usertutk").html(ret);

            ret = GetRtspKeyStr(result, 'password');
            $("#pwdtutk").val(ret);

            ret = GetRtspKeyStr(result, 'status');
            var status = null;
            switch(parseInt(ret))
            {
                case 0:
                    status = IDC_TUTK_STATUS0;
                    break;
                case 1:
                    status = IDC_TUTK_STATUS1;
                    break;
                case 2:
                    status = IDC_TUTK_STATUS2;
                    break;
                default:
                    break;
            }
            $("#statustutk").html(status);
        }});
    }
    catch (E){
    }
}

function tutkswitch(){
    disabledPwdTutk($("input[name='tutkswitch']:checked").val())
}

function disabledPwdTutk(type){
    $("#pwdtutk").attr("disabled",type==0);
}

function SaveNetTutk(){
    var enable = $("input[name='tutkswitch']:checked").val();
    var pwdtutk = $("#pwdtutk").val();
    var jcpstr = "tutkcfg -act set -enable " + enable + " -password " + pwdtutk;
    GetJCP({cmd: jcpstr});
    alert(IDC_MSGBOX_SAVEOK);
    window.focus();
    return true;
}


function initGB(){
    try
    {
        GetJCP({cmd: "guobiaocfg -act list",  ParseJCP: ParseSIP});
        GetJCP({cmd: "guobiaoaddr -act list", ParseJCP: function(jcpObj){
            if(jcpObj !== 'Error'){
                $("#GbLocation").val(jcpObj.address);
                $("#GbLongitude").val(jcpObj.longitude);
                $("#GbLatitude").val(jcpObj.latitude);
            }
        }});
    }
    catch (E){
    }
}

function ParseSIP(jcpObj)
{
    if(jcpObj !== 'Error'){
        try
        {
            $("#videochannel").val(jcpObj.videochannel);
            $("#dev_area").val(jcpObj.civilcode);
            $("#SipServerIp").val(jcpObj.srvip);
            $("#SipPorts").val(jcpObj.port);
            $("#LocalPorts").val(jcpObj.localport);
            $("#SipServerId").val(jcpObj.srvid);
            $("#SipSystemName").val(jcpObj.devsysname);
            $("#SipDev").val(jcpObj.devid);
            $("#SipAlarms").val(jcpObj.alarmid);
            $("#SipTimeInter").val(jcpObj.reginterval);
            $("#SipTimeHbda").val(jcpObj.hbinterval);
            $("#SipAuthenNames").val(jcpObj.authname);
            $("#SipUserName").val(jcpObj.username);
            $("#SipPwd").val(jcpObj.password);
            $("#SipTransport").val(jcpObj.protocoltype);
            $("input[name='guobiao_switch'][value=" + parseInt(jcpObj.enable) + "]").prop("checked",true);
            $("#guobiao_status").html("&nbsp;&nbsp;&nbsp;" + (jcpObj.connect_status==0?IDC_HXHT_CONNECT_STATUS_OFFLINE:IDC_HXHT_CONNECT_STATUS_ONLINE));
            $("#guobiao_version").html("&nbsp;&nbsp;&nbsp;" + jcpObj.gb_version);
        }
        catch (E){ return E; }
    }
}

var connstatus = new Array(IDC_HXHT_CONNECT_STATUS_OFFLINE, IDC_HXHT_CONNECT_STATUS_ONLINE);
function initHxht(){
    try
    {
        GetJCP({cmd: "hxhtcfg -act list", ParseJCP: function(jcpObj){
            if(jcpObj !== 'Error'){
                $("#devid").val(jcpObj.hdevid);
                $("#connect").val(jcpObj.hserveraddress);
                $("#connect").ipaddress({cidr:false});
                $("#form_connect_div_").css("margin-left","15px");
                $("#connect_port").val(jcpObj.hserverport);
                $("#connect_type").val(parseInt(jcpObj.hconnecttype));
                $("#video_port").val(jcpObj.hvideoport);
                $("#audio_port").val(jcpObj.haudioport);
                $("#instr_port").val(jcpObj.hinstrport);
                $("#record_port").val(jcpObj.hrecordport);
                $("#max_connect").val(jcpObj.hmaxconnect);
                $("input[name='natswitch'][value=" + parseInt(jcpObj.natenable) + "]").attr("checked",true);
                $("#connect_status").html(connstatus[parseInt(jcpObj.status)]);
            }
        }});
    }
    catch(E){}
}

function initGuoBiao(){
    try{
        GetJCP({cmd: "guobiaocfg -act list", ParseJCP: function(jcpObj){
            if(jcpObj !== 'Error'){
                $("#guobiao_status").html("&nbsp;&nbsp;&nbsp;" + (jcpObj.connect_status==0?IDC_HXHT_CONNECT_STATUS_OFFLINE:IDC_HXHT_CONNECT_STATUS_ONLINE));
            }
        }});
        window.focus();
    }catch(e){};
}

function initHngs(){
    try{
        GetJCP({cmd: "hngscfg -act list", ParseJCP: function(jcpObj){
            if(jcpObj !== 'Error'){
                $("#ip").val(jcpObj.servip);
                $("#ip").ipaddress({cidr:false});
                $("#master").val(jcpObj.main_sendip);
                $("#master").ipaddress({cidr:false});
                $("#masterPort").val(jcpObj.main_port);
                $("#masterCheck").prop("checked", jcpObj.main_enable==1?true:false);    
                $("#slave").val(jcpObj.sub_sendip);
                $("#slave").ipaddress({cidr:false});
                $("#slavePort").val(jcpObj.sub_port);
                $("#slaveCheck").prop("checked", jcpObj.sub_enable==1?true:false);   
            } 
        }});
    }catch(e){};
}

function initTslive(){
    try{
        GetJCP({cmd: "tslivecfg  -act list", ParseJCP: function(jcpObj){
            if(jcpObj !== 'Error'){
                $("input[name='bqstswitch'][value=" + parseInt(jcpObj.menable) + "]").attr("checked",true);
                $("#dqst_single").val(jcpObj.municastaddr);
                $("#dqst_single").ipaddress({cidr:false});
                $("#dqst_singlePort").val(jcpObj.municastport);
                $("#dqst_both").val(jcpObj.mmultiaddr);
                $("#dqst_both").ipaddress({cidr:false});
                $("#dqst_bothPort").val(jcpObj.mmultiport);
                SetDisabled(parseInt(jcpObj.menable) == 0);
                $("input[name='cbqstswitch2'][value=" + parseInt(jcpObj.senable) + "]").attr("checked",true);
                $("#cdqst_single").val(jcpObj.sunicastaddr);
                $("#cdqst_single").ipaddress({cidr:false});
                $("#cdqst_singlePort").val(jcpObj.sunicastport);
                $("#cdqst_both").val(jcpObj.smultiaddr);
                $("#cdqst_both").ipaddress({cidr:false});
                $("#cdqst_bothPort").val(jcpObj.smultiport);
                SetDisableds(parseInt(jcpObj.senable) == 0);
            }
        }});
    }catch(e){};
}

function SetDisabled(flag)
{
    for(var i=1;i<=4;i++){
        $("#form_dqst_single_octet_"+i).attr("disabled",flag);
        $("#form_dqst_both_octet_"+i).attr("disabled",flag);
    }
    $("#dqst_singlePort").attr("disabled",flag);
    $("#dqst_bothPort").attr("disabled",flag);
}


function SetDisableds(flag)
{
    for(var i=1;i<=4;i++){
        $("#form_cdqst_single_octet_"+i).attr("disabled",flag);
        $("#form_cdqst_both_octet_"+i).attr("disabled",flag);
    }
    $("#cdqst_singlePort").attr("disabled",flag);
    $("#cdqst_bothPort").attr("disabled",flag);
}

function SaveNetGB(){
    var serverIp = $("#SipServerIp").val();
    var ports = $("#SipPorts").val();
    var localport = $("#LocalPorts").val();
    var serverId = $("#SipServerId").val();
    var systemName = $("#SipSystemName").val();
    var devs = $("#SipDev").val();
    var alarms = $("#SipAlarms").val();
    var timeInter = $("#SipTimeInter").val();
    var timeHbda = $("#SipTimeHbda").val();
    var authenName = $("#SipAuthenNames").val();
    var userName = $("#SipUserName").val();
    var pwd = $("#SipPwd").val();
    var transport = $("#SipTransport").val();

    var location = $("#GbLocation").val();
    var longitude = $("#GbLongitude").val();
    var latitude = $("#GbLatitude").val();

    
    var enable = $('input:radio[name="guobiao_switch"]:checked').val();

    // for '\#'
    var index = pwd.indexOf('#');
    if (index !== -1) {
            pwd = pwd.substring(0, index) + '\\' + pwd.substring(index);
    }

    // for '&'
    index = pwd.indexOf('&');
    if (index !== -1) {
        pwd = pwd.replace(/&/g, '%26');
    }
    //alert(pwd);

    try
    {
        if(serverIp == "" || ports == "" || localport == "" || serverId == "" || systemName == "" || devs == "" || alarms == "" || timeInter == "" || timeHbda == "" || authenName == "" || userName == "" || pwd == "" || location == "" || longitude == "" || latitude == ""){
            alert(IDC_PROMPT_NULL);
            window.focus();
            return;
        }

        if(65535 < ports || 65536 < localport){
            alert(IDC_GEN_PORT_RANGE);
            window.focus();
            return;
        }
        
        if(isNaN(timeInter) || isNaN(timeHbda))
        {
            alert(IDC_TIS_TIME);
        }
        else
        {
            var jcpstr = "guobiaocfg -act set" + " -srvip " + serverIp + " -port " + ports + " -localport " + localport + " -srvid " + serverId + " -devsysname " + systemName + " -devid " + devs + " -alarmid " + alarms + " -reginterval " + timeInter+ " -hbinterval " + timeHbda + " -authname " + authenName + " -username " + userName + " -password " + pwd;
            jcpstr += " -protocoltype  " + transport;
            jcpstr += " -civilcode " + $('#dev_area').val();
            jcpstr += " -enable " + enable;
            jcpstr += " -videochannel " + $('#videochannel').val();
            GetJCP({cmd: jcpstr,ParseJCP:function(jcpObj){
                
            }});

            var jcpstrs = "guobiaoaddr -act set" + " -address " + location + " -longitude " + longitude + " -latitude " + latitude;
            GetJCP({cmd: jcpstrs});
            alert(IDC_MSGBOX_SAVEOK);
            window.focus();
            return 0;
        }
        
    }catch(e){ return e; }
    window.focus();
}

function SaveNetHxht(){
    var devid = $("#devid").val();
    var serviradd = $("#connect").getip();
    var serverport = parseInt($("#connect_port").val());
    var connecttype = parseInt($("#connect_type").val());
    var videoport = parseInt($("#video_port").val());
    var audioport = parseInt($("#audio_port").val());
    var instrport = parseInt($("#instr_port").val());
    var recordport = parseInt($("#record_port").val());
    var maxconnect = parseInt($("#max_connect").val());
    
    //判断端口是否符合要求
    if (isBlank(serverport) || serverport > 65535)
    {
        alert(IDC_GEN_PORT_MAX);
        window.focus();
        return 1;
    }
    if (isBlank(videoport) || videoport > 65535)
    {
        alert(IDC_GEN_PORT_MAX);
        window.focus();
        return 1;
    }    
    if (isBlank(audioport) || audioport > 65535)
    {
        alert(IDC_GEN_PORT_MAX);
        window.focus();
        return 1;
    }
    if (isBlank(instrport) || instrport > 65535)
    {
        alert(IDC_GEN_PORT_MAX);
        window.focus();
        return 1;
    }
    if (isBlank(recordport) || recordport > 65535)
    {
        alert(IDC_GEN_PORT_MAX);
        window.focus();
        return 1;
    }  
    if (isBlank(maxconnect) || maxconnect > 5 || 1 > maxconnect)
    {
        alert(IDC_HXHT_MAXCON_MSG + IDC_GEN_MAXCONNECT_RANGE);
        window.focus();
        return 1;
    }
    var natenable = parseInt($('input:radio[name="natswitch"]:checked').val());

    var jcpstr = "hxhtcfg -act set -hdevid " + devid + " -hserveraddress " + serviradd + " -hserverport " + serverport + " -hvideoport " + videoport;
    jcpstr += " -haudioport " + audioport + " -hinstrport " + instrport + " -hconnecttype " + connecttype + " -hrecordport " + recordport;
    jcpstr += " -hmaxconnect " + maxconnect + " -natenable " + natenable;
    GetJCP({cmd: jcpstr});

    alert(IDC_MSGBOX_SAVEOK);
    window.focus();
    return 0;
}

function FlushHxht()
{
    initHxht();
    window.focus();
}
         
function SaveNetHngs(){
    var ip = $("#ip").getip();
    var masterIp = $("#master").getip();
    var masterPort = $("#masterPort").val();
    var slaveIp = $("#slave").getip();
    var slavePort = $("#slavePort").val();
    var masterenb = 0,slavenb = 0;
    if($("#masterCheck").prop("checked"))
    {
        masterenb = 1;
    }

    if($("#slaveCheck").prop("checked"))
    {
        slavenb = 1;
    }

    if(ip.split(".").length > 3 && masterIp.split(".").length > 3 && slaveIp.split(".").length >3)
    {
        //判断DNS是否为0-255之间的数
        for (var i = 0; i < 4; i ++)
        {
            if (ip.split(".")[i] > 255 || ip.split(".")[i] < 0
                ||masterIp.split(".")[i] > 255 || masterIp.split(".")[i] < 0
                ||slaveIp.split(".")[i] > 255 || slaveIp.split(".")[i] < 0)
            {
                alert(IDC_MSGBOX_ADDRESS_NUM);
                window.focus();
                return;
            }
        }
    }
    else
    {
        alert(IDC_GET_REC_PROMPT);
        window.focus();
        return;
    }

    if(isNaN(masterPort) || isNaN(slavePort))
    {
        alert(IDC_GEN_IP_CK_MSG);
        window.focus();
        return;
    }
    else if(masterPort > 65535  || slavePort > 65535)
    {
        alert(IDC_GEN_PORT_MAX);
        window.focus();
        return;
    }
    else
    {
        var jcpstr = "hngscfg -act set -servip " + ip;
        jcpstr += " -main_sendip " + masterIp + " -main_port " + masterPort + " -main_enable " + masterenb;
        jcpstr += " -sub_sendip " + slaveIp + " -sub_port " + slavePort + " -sub_enable " + slavenb;
        GetJCP({cmd: jcpstr});

        alert(IDC_MSGBOX_SAVEOK);
        window.focus();
    }
}

function SaveNetTslive(){
    //主码流
    var menable = $('input:radio[name="bqstswitch"]:checked').val();
    var municastaddr1 = $('#dqst_single').getip();
    var municastport1 = $('#dqst_singlePort').val();
    var mmultiaddr = $('#dqst_both').getip();
    var mmultiport = $('#dqst_bothPort').val();
        
    //从码流
    var senable = $('input:radio[name="cbqstswitch2"]:checked').val();
    var sunicastaddr1 = $('#cdqst_single').getip();
    var sunicastport1 = $('#cdqst_singlePort').val();
    var smultiaddr = $('#cdqst_both').getip();
    var smultiport = $('#cdqst_bothPort').val();

    try
    {
        if(isNaN(municastport1) || isNaN(mmultiport) || isNaN(sunicastport1) || isNaN(smultiport))
        {
            alert(IDC_GET_REC_PROMPT);
            return;
        }

        if(menable == 1 && (65535 < municastport1 || 65535 < mmultiport) ){
            alert(IDC_GEN_PORT_MAX);
            return;
        }

        if(senable == 1 && ( 65535 < sunicastport1 || 65535 < smultiport) ){
            alert(IDC_GEN_PORT_MAX);
            return;
        }

        var jcpstr = "tslivecfg -act set -menable " + menable + " -mmultiaddr " + mmultiaddr + " -mmultiport " + mmultiport +
                    " -municastaddr " + municastaddr1 + " -municastport " + municastport1 + 
                    " -senable " + senable + " -smultiaddr " + smultiaddr + " -smultiport " + smultiport +
                    " -sunicastaddr "+ sunicastaddr1 + " -sunicastport " + sunicastport1;
        GetJCP({cmd: jcpstr});
        alert(IDC_MSGBOX_SAVEOK);
        
        window.focus();
    }
    catch(e){ return e; }
}

function initJstar(){
    try{
        GetJCP({cmd: "jstarcfg -act list", ParseJCP: function(jcpObj){
            if(jcpObj !== 'Error'){
                $("input[name='platform_enable'][value=" + parseInt(jcpObj.enable_flag) + "]").prop("checked",true);
                $("#dev_id").val(jcpObj.pu_id);
                $("#dev_name").val(jcpObj.pu_name);
                $("#dev_pwd").val(jcpObj.pu_passwd);
                $("#csgPorts").val(jcpObj.csg_port);
                $("#mduPorts").val(jcpObj.mdu_port);
                $("#csg_ip").val(jcpObj.csg_ip);
                $("#csg_ip").ipaddress({cidr:false});
                $("#mdu_ip").val(jcpObj.mdu_ip);
                $("#mdu_ip").ipaddress({cidr:false});
                $("#protocol_type").val(jcpObj.protocol_type);
                $("input[name='offline_to_record'][value=" + parseInt(jcpObj.offline_to_record) + "]").prop("checked",true);
                $("input[name='offline_to_upload'][value=" + parseInt(jcpObj.offline_to_upload) + "]").prop("checked",true);
            }
        }});
    }catch(e){};
}

function SaveJStar(){
    var enable_flag = $('input:radio[name="platform_enable"]:checked').val();
    var pu_id = $('#dev_id').val();
    var dev_name = $('#dev_name').val();
    var dev_pwd = $('#dev_pwd').val();
    var csgPorts = $('#csgPorts').val();
    var mduPorts = $('#mduPorts').val();
    var csg_ip = $('#csg_ip').getip();
    var mdu_ip = $('#mdu_ip').getip();
    var protocol_type = $('#protocol_type').val();
    var offline_to_record = $('input:radio[name="offline_to_record"]:checked').val();
    var offline_to_upload = $('input:radio[name="offline_to_upload"]:checked').val();
    
    try
    {
        if(isNaN(csgPorts) || isNaN(mduPorts))
        {
            alert(IDC_GET_REC_PROMPT);
            return;
        }

        if(65535 < csgPorts || 1 > csgPorts ){
            alert(IDC_GEN_PORT_RANGE);
            return;
        }

        if(65535 < mduPorts || 1 > mduPorts ){
            alert(IDC_GEN_PORT_RANGE);
            return;
        }

        var jcpstr = "jstarcfg -act set -enable_flag " + enable_flag + " -pu_id " + pu_id + " -pu_name " + dev_name +
            " -pu_passwd " + dev_pwd + " -csg_port " + csgPorts + 
            " -mdu_port " + mduPorts + " -csg_ip " + csg_ip + " -mdu_ip " + mdu_ip +
            " -protocol_type "+ protocol_type + " -offline_to_record " + offline_to_record + " -offline_to_upload "+offline_to_upload;
    
        GetJCP({cmd: jcpstr});
        alert(IDC_MSGBOX_SAVEOK);
        
        window.focus();
    }
    catch(e){ return e; }

   
}

function initCloudPlatform(){
    try{
        GetJCP({cmd: "burninfo -act list", ParseJCP: function(jcpObj) {
            if(jcpObj !== 'Error') {
                $("#platform_devid").html(jcpObj.cpuid);
            }
        }});

        GetJCP({cmd: "txconfcfg -act list", ParseJCP: function(jcpObj) {
            if(jcpObj !== 'Error') {
                $("#platform_status").html(jcpObj.connect_status==0?IDC_HXHT_CONNECT_STATUS_OFFLINE:IDC_HXHT_CONNECT_STATUS_ONLINE);
                $("#qrcode_path").attr("src","/"+jcpObj.bmp_path + "?"+Date.parse(new Date()));
            }
        }});
        window.focus();
    }catch(e){};
}

function switchWstk(){
    try{
        var enable = $('input:radio[name="wstk_switch"]:checked').val();
        var jcpstr = "wstkcfg -act set -enable " + enable ;
        GetJCP({cmd: jcpstr});
    }catch(e){};
}

function initWstk(){
    try{
        GetJCP({cmd: "wstkcfg -act list", ParseJCP: function(jcpObj){
            if(jcpObj !== 'Error'){
                $("#wstk_id").html(jcpObj.wstk_id);
                $("#wstk_status").html(jcpObj.online_status==0?IDC_HXHT_CONNECT_STATUS_OFFLINE:IDC_HXHT_CONNECT_STATUS_ONLINE);
                $("#wstk_path").attr("src","/"+jcpObj.bmp_path + "?"+Date.parse(new Date()));
                $("input[name='wstk_switch'][value=" + parseInt(jcpObj.enable) + "]").prop("checked",true);
                
            }
        }});
        window.focus();
    }catch(e){};
}

var $w = $(window).width();
if($w < 1000){
   $("body").css("width",1000);
}
$(window).resize(function(){
    var $w = $(window).width();
    if($w < 1000){
       $("body").css("width",1000);
    }else{
       $("body").css("width",'99%');
    }
});
