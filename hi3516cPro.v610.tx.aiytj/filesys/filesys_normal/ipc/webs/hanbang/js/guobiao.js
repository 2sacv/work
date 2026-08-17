$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
        initGB();
    }
})

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
            $("#SipServerId").val(jcpObj.srvid);
            $("#SipSystemName").val(jcpObj.devsysname);
            $("#SipDev").val(jcpObj.devid);
            $("#SipAlarms").val(jcpObj.alarmid);
            $("#SipTimeInter").val(jcpObj.reginterval);
            $("#SipTimeHbda").val(jcpObj.hbinterval);
            $("#SipPwd").val(jcpObj.password);
            $("#LocalPorts").val(jcpObj.localport);
            $("input[name='guobiao_switch'][value=" + parseInt(jcpObj.enable) + "]").prop("checked",true);
            $("#guobiao_status").html("&nbsp;&nbsp;&nbsp;" + (jcpObj.connect_status==0?IDC_HXHT_CONNECT_STATUS_OFFLINE:IDC_HXHT_CONNECT_STATUS_ONLINE));
        }
        catch (E){ return E; }
    }
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

function SaveNetGB(){
    var serverIp = $("#SipServerIp").val();
    var ports = $("#SipPorts").val();
    var serverId = $("#SipServerId").val();
    var systemName = $("#SipSystemName").val();
    var devs = $("#SipDev").val();
    var alarms = $("#SipAlarms").val();
    var timeInter = $("#SipTimeInter").val();
    var timeHbda = $("#SipTimeHbda").val();
    var pwd = $("#SipPwd").val();
    var localport = $("#LocalPorts").val();

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
	
    try
    {
        if(serverIp == "" || ports == "" || serverId == "" || systemName == "" || devs == "" || alarms == "" || timeInter == "" || timeHbda == "" || pwd == "" || location == "" || longitude == "" || latitude == "" || localport == ""){
            parent.paramFailTip(IDC_PROMPT_NULL);
            window.focus();
            return;
        }

        if(65535 < ports){
            parent.paramFailTip(IDC_GEN_PORT_RANGE);
            window.focus();
            return;
        }
        
        if(isNaN(timeInter) || isNaN(timeHbda))
        {
            parent.paramFailTip(IDC_TIS_TIME);
        }
        else
        {
            var jcpstr = "guobiaocfg -act set" + " -srvip " + serverIp + " -port " + ports + " -srvid " + serverId + " -devsysname " + systemName + " -devid " + devs + " -alarmid " + alarms + " -reginterval " + timeInter+ " -hbinterval " + timeHbda + " -password " + pwd;
            jcpstr += " -civilcode " + $('#dev_area').val();
            jcpstr += " -enable " + enable;
            jcpstr += " -videochannel " + $('#videochannel').val();
			jcpstr += " -localport " + localport;
            GetJCP({cmd: jcpstr,ParseJCP:function(jcpObj){
                
            }});

            var jcpstrs = "guobiaoaddr -act set" + " -address " + location + " -longitude " + longitude + " -latitude " + latitude;
            GetJCP({cmd: jcpstrs});
            parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
            window.focus();
            return 0;
        }
        
    }catch(e){ return e; }
    window.focus();
}
