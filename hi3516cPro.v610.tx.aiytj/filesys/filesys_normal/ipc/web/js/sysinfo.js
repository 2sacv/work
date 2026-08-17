var ptz_alarm = 0; //是否有报警输入,1有,其他否
$(function(){
    ptz_alarm =  $.cookie("has_alarmin");
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf || typeof(lf) =='undefined' || 0 > parseInt(lf))
    {
          parent.location.href = "/login.asp";
    }else{
        initAlarmtype();//初始化报警类型
        showDMUpdate();//显示单片机升级tab
        showPortSetting();//显示串口设置tab
        _init_load();
        _init_progress();
        $("select").bind("change",function(){
             window.focus();
        });
    }
})

function initAlarmtype(){
    var _html = '<option value="0">'+JALARM_TYPE_BEGIN+'</option>';
    if(parseInt(ptz_alarm) > 0){
        for(var i=0;i<parseInt(ptz_alarm);i++){
            _html += '<option value="'+(80+i)+'">'+IDC_ALARM_TYPE_AI_CHN+(i+1)+'</option>';
        }
    }
    // _html += '<option value="1">'+JALARM_TYPE_MD+'</option>';
    _html += '<option value="2">'+IDC_MD_VGLINE+'</option>';
    _html += '<option value="3">'+IDC_MD_VGRECT+'</option>';
    _html += '<option value="13">'+IDC_HUMAN_DETECTION+'</option>';
    _html += '<option value="20">'+IDC_PEOPLE_CAR_DETECT+'</option>';
    _html += '<option value="4">'+JALARM_TYPE_VL+'</option>';
    // _html += '<option value="12">'+JALARM_TYPE_VM+'</option>';
    _html += '<option value="6">'+JALARM_TYPE_DISK_ERR+'</option>';
    _html += '<option value="7">'+IDC_ALARM_NET_DISCONNECT+'</option>';
    _html += '<option value="8">'+JALARM_TYPE_IP_CONFLICT+'</option>';
    $("#selAlarmType").html(_html);
}

_init_load = function(){
    window.parent.document.title = IDC_MENU_SYSTEM_SET;
    $("#tabs").tabs();
    clickTabBasicInfo();
}


//清除定时器
function cleanTimeout(){
    window.clearTimeout(CloseTimeoutsys);
    window.clearTimeout(flushTimeoutStatus);
}

//基本信息Tab
function clickTabBasicInfo(){
    cleanTimeout();
    GetJCP({cmd: "version -act list",ParseJCP: initDevInfo});
    window.focus();
}

//时钟设置Tab
function clickTabTimeSetting(){
    cleanTimeout();
    GetJCP({cmd: "timecfg -act list",ParseJCP: initTimeSetting});
    GetJCP({cmd: "ntpcfg -act list",ParseJCP: initNtpSetting});
    window.focus();
}

//系统维护Tab
function clickTabSysOpera(){
    cleanTimeout();
    GetJCPList({cmd:"sysctrl -act list",ParseJCP: initRebootInfoAndStatus});
    window.focus();
}

//系统升级
function clickTabSysUpdate(){
    cleanTimeout();
    GetJCP({cmd: "update -act list",ParseJCP: function(result){
        if(result.progressbar > 0)
        {
            get_progress()
            $("#update_confirm").prop("disabled",true)
        }
    }})
    window.focus();
}

//用户管理Tab
function clickTabUserManage(){
    cleanTimeout();
    initUserManageInfo();
    window.focus();
}

//系统状态Tab
function clickTabSysStatus(){
    cleanTimeout();
    $("#sysStatusEn").prop('checked',false);
    timeSysStatus();
    window.focus();
}

//系统日志
function clickTabSysLog(){
    cleanTimeout();
    _init_log();
    window.focus();
}

//报警日志Tab
function clickTabAlarmLog(){
    cleanTimeout();
    _init_alarm_log();
    window.focus();
}

//串口设置
function clickTabPortSetting(){
    cleanTimeout();
    _init_port_setting();
    window.focus();
}

_init_progress = function(){
    $("#progress_lab").text("2%")
    $("#progress").progressbar({
        value: 2,
        change: function(){
            $("#progress_lab").text( $("#progress").progressbar( "value" ) + "%" )  
        }
    })

    $( "#progressbar" ).progressbar({
      value: false,
      change: function() {
            $( ".progress-label" ).text( $( "#progressbar" ).progressbar( "value" ) + "%" );
      },
      complete: function() {
            $( "#progressbar" ).hide()
      }
    });
    //单片机升级
    $("#dmprogress").progressbar({
        value: 0,
        min:0,
        max:100,
        change: function(){
            $("#dmprogress_lab").text( $("#dmprogress").progressbar("value") + "%" )  
        }
    });
}

//初始化设备信息
function initDevInfo(result){
    try{
        $("#dev_name").val(result.devname);
        $("#dev_model").html(result.devtype);
        $("#dev_num").html(result.devid);
        $("#dev_bb").html(result.kernelver);
        $("#web_bb").html(result.webver);
        $("#server_bb").html(result.serverver);
        var cookie = GetCookieByKey("ocxversion");
        if(cookie < "6.0.0.2"){ alert(IDC_OCX_VERSIONFAIL); }
        $("#osd").html(cookie);
    }catch(e){}
}

//初始化自动重启信息和系统状态
function  initRebootInfoAndStatus(result){
    try{
        var strArr = result.split("#");
        if(parseInt(GetRtspKeyStr(strArr[0],"arben"))==1){
            $("#ckarb").prop("checked",true);
        }
        $("#time").val(parseInt(GetRtspKeyStr(strArr[0],"arbtm")));
        $("#weekend").val(parseInt(GetRtspKeyStr(strArr[0],"arbweek")));
        showSysStatus(result);
    }catch(e){}
}

function SetMsgToTable(s, kv)
{
    var insert = '<tr ><td align="center" valign="middle">' + s + '</td>';
    insert += '<td align="left" valign="middle" height="20">' + kv + '</td></tr>';
    $("#tbStatus").append(insert);
}

function FlushStatus()
{
    window.clearTimeout(flushTimeoutStatus);
    flushTimer = $("#sysStatusTime").val();
    timeSysStatus();
    if($("#sysStatusEn").is(":checked")){
        flushTimeoutStatus = setTimeout('FlushStatus()',1000*flushTimer);
    }
     window.focus(); 
}

function timeSysStatus(){
    GetJCPList({cmd: "sysctrl -act list", ParseJCP: showSysStatus});
}

function checkSysStatus(){
    if(!$("#sysStatusEn").is(":checked")){
        window.clearTimeout(flushTimeoutStatus);
    }else{
        FlushStatus();
    }
}

//系统状态更新时间
var flushTimer = 0;
var flushTimeoutStatus;

function showSysStatus(result){
    try{
        $("#tbStatus tr:not(:first)").remove();
        var content = result.split("status=")[1];
        var index = content.lastIndexOf(",;;");
        if(index>0){
            content = content.substring(0,index);
        }
        var arrRes = content.split(",");
        for (var i = 0, len = arrRes.length; i<len; i++){
          var kv = arrRes[i].replace("#","=");
          SetMsgToTable(i+1,kv);
        }

    }catch(e){}
}

//初始化时钟设置信息
function initTimeSetting(result){
     try{
        $("#TimeZone").val(result.timezone);
        var strtime = result.timestr;
        
        if(strtime.length>=10){
            $("#nvsdate").val(strtime.substring(0,10));
            $("#nvstime").val(strtime.substring(11,strtime.length));
        }
        $("#chksych").attr('checked',false);
    }catch(e){}
}

//初始化Ntp服务器设置
function initNtpSetting(result){
    try{
        if(parseInt(result.ntpen)==1){
            $("#ntpserviceen").prop("checked",true);
        }
        $("#ntpserveraddr").val(result.ntphost);
        $("#ntpserverport").val(result.ntpport);
        $("#ntpsyctime").val(result.interval);
        ntpEnable();
        
    }catch(e){}
}

function ntpEnable(){
    var flag = $("#ntpserviceen").is(":checked");
    $("#ntpserveraddr").attr("disabled",!flag);
    $("#ntpserverport").attr("disabled",!flag);
    $("#ntpsyctime").attr("disabled",!flag);
}

function SaveDevName(){
    var name = $.trim($("#dev_name").val());

    var myRegExp = /["'<>%;)(&+“”\[\]!！？?\\#￥$^；【】。]/;
    if(myRegExp.test(name)){
          alert(IDC_DEVNAME+IDC_NOT_CONTAIN+"\"'<>%;)(&+“”[]!！？?\\#￥$^；【】。");
          window.focus();
          return;
    }

    var cArr = name.match(/[^\x00-\xff]/ig);   
    var len =  name.length + (cArr == null ? 0 : cArr.length*2); 
    if(len==0){
        alert(IDC_DEVNAME_NULL);
        window.focus();
        return;
    }
    if(len>63){
        alert(IDC_DEVNAME_TOO_LONG);
        window.focus();
        return;
    }

    GetJCP({cmd: "version -act set -devname  \"" + name + "\"",ParseJCP: function(result){
        alert(IDC_MSGBOX_SAVEOK);
        window.focus();
        return;
    }});
}

//时间函数
var CloseTimeoutsys;
function sychtime()
{
    window.clearTimeout(CloseTimeoutsys);
    $("#nvsdate").val(new_date());
    $("#nvstime").val(new_time());
    
    if($("#chksych").is(':checked'))    
    {
        CloseTimeoutsys = setTimeout('sychtime()',1000);
    }
}


function SaveTimezoneSetting(){
    var jcpstr = "timecfg -act set -timezone "+$("#TimeZone").val();
    GetJCP({cmd: jcpstr,ParseJCP: function(result){
        if(result != "Error"){
            alert(IDC_MSGBOX_SAVEOK);        
        }
    }});
    window.focus();
    return;
}

//响应onkeyup事件, 限制用户输入以下特殊字符
function IsInputDateUp(){
    var arg0 = arguments[0];
    var myRegExp = /[\\\/]/;
    if(myRegExp.test(arg0.value)){
          arg0.value = arg0.value.replace(myRegExp, '-');
    }
}


function SaveTimeSetting(){
    var date = $("#nvsdate").val();
    var time = $("#nvstime").val();

    if(!RQcheck(date)){
        alert(IDC_TIME_DATE_FAIL);
        return false;
    }

    var ss = date.split("-");
    if(ss[0]<1970){
        alert(IDC_TIME_DATE_YEAR_FAIL);
        return false;
    }
    var reg=/^((20|21|22|23|[0-1]\d)\:[0-5][0-9])(\:[0-5][0-9])?$/;    
    if(!reg.test(time)){    
        alert(IDC_TIME_TIME_FAIL);
        return false;
    }    
    
    var st = time.split(":");
    if (st[0] > 24)
    {
        alert(IDC_TIME_TIME_FAIL);
        return false;
    }
    
    if (st[1] > 59)
    {
        alert(IDC_TIME_TIME_FAIL);
        return false;
    }
    
    if (st[2] > 59)
    {
        alert(IDC_TIME_TIME_FAIL);
        return false;
    }
    
    var sdate = new Date(ss[0], ss[1] - 1, ss[2], st[0], st[1], st[2]);
    var jcpstr = "timecfg -act set -time " + (sdate / 1000);
    GetJCP({cmd: jcpstr, ParseJCP: function(result){
            if(result == 'Error'){
                alert(IDC_MSGBOX_SAVEFAIL);
            }else{
                alert(IDC_MSGBOX_SAVEOK);
            }
    }});
    window.focus();
    return;
}

function SaveNtpSetting(){
    var address = $("#ntpserveraddr").val();
    var port = $("#ntpserverport").val();
    var time = $("#ntpsyctime").val();

    var myRegExp = /["'<>%;)(&+“”\[\]!！？?\\#￥$^；【】。]/;
    if(myRegExp.test(address)){
          alert(IDC_SERVERADDR+IDC_NOT_CONTAIN+"\"'<>%;)(&+“”[]!！？?\\#￥$^；【】。");
          window.focus();
          return;
    }
    
    var cArr = address.match(/[^\x00-\xff]/ig);   
    var len =  address.length + (cArr == null ? 0 : cArr.length*2);   

    if(len==0 || len>63){
        alert(IDC_NTP_ADDRESS_ERROR);
        window.focus();
        return;
    }
    if(port.length==0 || isNaN(port) || port==0 || port>65535){
        alert(IDC_NTP_PORT_ERROR);
        window.focus();
        return;
    }

    if(time.length==0 || isNaN(time) || time==0 || time>65535){
        alert(IDC_NTP_SYNTIME_ERROR);
        window.focus();
        return;
    }

    var ntpen = $("#ntpserviceen").is(":checked")?1:0;
    var jcpstr = "ntpcfg -act set -ntpen "+parseInt(ntpen)+" -ntphost "+address+" -ntpport ";
    jcpstr+= port+" -interval "+time;
    GetJCP({cmd: jcpstr});
    alert(IDC_MSGBOX_SAVEOK);
    window.focus();
    return;
}


function SaveAutoreboot(){
    var arbtime = $("#time").val();
    var arbweek = $("#weekend").val();
    var jcpstr = "sysctrl -act set -arben " + ($("#ckarb").is(":checked") ? 1 : 0) + " -arbtm " + arbtime + " -arbweek " + arbweek;
    GetJCP({cmd: jcpstr});
    
    alert(IDC_MSGBOX_SAVEOK);
    window.focus();
    return;
}

var restart_num = '0';
SysRet = function(num)
{
    restart_num = num;
    var a = num == 2 ? (IDC_MSGBOX_CONFIRM+IDC_LOADDEFAULT) : (IDC_MSGBOX_CONFIRM+IDC_RESETDEV);
    var tip = num == 2?IDC_DEFAULT_WAIT:IDC_REBOOT_WAIT;
    if(confirm(a)){
        GetJCP({cmd: "sysctrl -act set -cmd " + num,ParseJCP: function(result){
            if(result != 'Error'){
                $("#progressbar").show();
                $("#sysrettip").show();
                $("#sysrettip").html(tip);
                $("#defaultBtn").attr("disabled",true);
                $("#resetBtn").attr("disabled",true);
                $("#autoreBtn").attr("disabled",true);
                setTimeout( _progress, 1000 );
            }
        }})
    }
    window.focus();
}

var restart_progress = 0;
_progress = function(){
    var val = $("#progressbar").progressbar( "value" ) || 0;
    $("#progressbar").progressbar( "value", val+1);
 
    if(100 == parseInt(val)){
        window.clearTimeout(restart_progress)
        if(2 == parseInt(restart_num)){
            parent.location.href = 'http://192.168.1.217/login.asp'  
        }
        else
        {
            parent.location.href = '../login.asp'
        }
    }
    restart_progress = setTimeout( _progress, 600 );
}

function IframeUpdate(){
    var file = $("#filepath").val();
    var file_length = file.split(".");
    if(file == "" || file_length[file_length.length-1]!= 'tgz')
    {
        alert(IDC_UPDATE_TYPE_ERROR);
        return false;
    }
    else
    {
        if(confirm(IDC_MSGBOX_CONFIRM+IDC_UPDATE)){
            document.frmUpdate.action="/webs/updateCfg";
            frmUpdate.submit();
            $("#progress_div").show();
            $("#restart_prompt_div").show();
            $("#progress").progressbar("value", 1);
            window.setTimeout(get_progress,3000);
        }
        window.focus();
    }
}

var update_time = "";
get_progress = function(){
    $("#update_confirm").prop("disabled",true)
    $("#filepath").prop("disabled",true);
    $("#restart_prompt_div").show();
    GetJCP({cmd: "update -act list",ParseJCP: function(result){
        $("#progress_div").show()
        if('undefined' === typeof(result.progressbar) || 100 == parseInt(result.progressbar)){
            _to_be_continueted_pregress();
        }
        else if(100 < parseInt(result.progressbar) && 110 > parseInt(result.progressbar)){
            $('#progress_div').hide()
            clearTimeout(update_time)
            $("#restart_prompt").html(IDC_PACKAGE_ERROR+", " +IDC_SERVER_RESTART_PROMPT)
            _error_progress()
        }
        else if(parseInt(result.progressbar) == 111){
          $('#progress_div').hide()
          clearTimeout(update_time)
          $("#restart_prompt").html(IDC_SCRIPT_UPGRADE_ERROR+", " +IDC_SERVER_RESTART_PROMPT)
          _error_progress()
        }
        else
        {
            if (parseInt(result.progressbar) >= 0 && parseInt(result.progressbar) < 100) {
                var p = parseInt(result.progressbar/10,10)+1;
                $("#progress").progressbar("value", parseInt(p))
            }
            update_time = setTimeout(get_progress,3000)  
        }
}})
}

var error_p = 0, error_progress = 0, success_p = 0, success_progress = 0;
_error_progress = function(){
    $("#progress_div").show()
    $("#progress").progressbar("value", parseInt(error_p))
    if(error_p == 100){
        $("#progress_div").hide()
        clearTimeout(error_progress)
        $("#restart_prompt_div").hide()
        parent.location.href = "../login.asp";
    }
    else{
        error_p++;
        error_progress = setTimeout(_error_progress,350)    
    }
}

var be_continueted_pregress = 0, be_continueted = 10;
_to_be_continueted_pregress = function(){
    if(be_continueted == 100){
        $('#progress_div').hide()
        clearTimeout(be_continueted_pregress)
        $("#restart_prompt").html(IDC_UPDATE+IDC_SUCCESS+IDC_SERVER_RESTART_PROMPT)
        _success_progress()
    }else{
        be_continueted++;
        $("#progress").progressbar("value", parseInt(be_continueted));
        be_continueted_pregress = setTimeout(_to_be_continueted_pregress,1100)
    }
}

_success_progress = function(){
    $("#progress_div").show()
    $("#progress").progressbar("value", parseInt(success_p))
    if(success_p == 100){
        $("#progress_div").hide()
        clearTimeout(success_progress)
        $("#restart_prompt_div").hide()
        parent.location.href = "../login.asp";
    }
    else{
        success_p++;
        success_progress = setTimeout(_success_progress,810)
    }
}

//------------------------------------------------------------系统日志
function _init_log(){
    var language= GetCookieByKey("languages")==0?'zh-cn':'en';
    if(language == 'zh-cn' && g_lan_js_arr[0] == 'russian.js'){
        language = 'rus';
    }
    var webdeflang = GetCookieByKey("webdeflang");
    if (parseInt(webdeflang) == 1) {
        language = 'en';
    }
    $("#startTime").click(function(){
        WdatePicker({readOnly:true,lang:language,dateFmt:'yyyy-MM-dd HH:mm:ss'});
    });

    $("#endTime").click(function(){
        WdatePicker({readOnly:true,lang:language,dateFmt:'yyyy-MM-dd HH:mm:ss'});
    });

    $("#startTime").val(new_date()+" 00:00:00");
    $("#endTime").val(new_date()+" 23:59:59");
    

    cleanLogData();
}

var pageNum=20;
var totalPage=0;
var totalNum=0;
var currPage=1;

function searchLog(){
    var sTime = $("#startTime").val();
    var eTime = $("#endTime").val();

    if(sTime.length==0){
        alert(IDC_STARTTIME_SELECT);
        window.focus(); 
        return;
    }

    if(eTime.length==0){
        alert(IDC_ENDTIME_SELECT);
        window.focus(); 
        return;
    }

    if(sTime>=eTime){
        alert(IDC_STARTTIME_BIGGER_ENDTIME);
        window.focus(); 
        return;
    }

    cleanLogData();

    $("#searchLog").attr("disabled",true);
    $("#searchLog").after("<span style='color:red;' id='searchLogTip'>"+IDC_QUERYING+"</span>");

    var itemindex = (currPage-1)*pageNum+1;
    var jcpstr = "getlog -act list -starttime \""+sTime+"\" -endtime \""+eTime+"\" -itemindex  "+itemindex+" -itemnum "+pageNum;
    GetJCPList({cmd: jcpstr,ParseJCP: function(result){
        var arrRst = result.split(";");
        totalNum = parseInt(arrRst[0].split("=")[1]);
        if(totalNum>0){
            totalPage = parseInt(totalNum/pageNum)+(totalNum%pageNum==0?0:1);
      
            var itemlist = arrRst[2].split("itemlist=")[1];
            showLog(itemlist);
            if(totalPage>1){
                $("#selPage").attr('disabled',false);
                $("#selPage").empty();
                for(var j=1;j<=totalPage;j++){
                    $("#selPage").append("<option value="+j+">"+j+"</option>");
                }
                $("#pageNext").attr('disabled',false);
            }else{
                $("#selPage").append("<option value='1'>1</option>");
            }

        }else{
            $("#tbLog").append("<tr><td colspan='3' align='center' >"+IDC_SYSLOG_NODATA+"</td></tr>");
        }
        $("#searchLogTip").remove();
        $("#searchLog").attr("disabled",false);
    }});
    window.focus();  
}

function cleanLogData(){
    $("#selPage").empty();
    $("#tbLog tr:not(:first)").remove();
    $("#selPage").attr('disabled',true);
    $("#pagePrev").attr('disabled',true);
    $("#pageNext").attr('disabled',true);
    currPage=1;
}

function changePage(){
    currPage = $("#selPage").val();

    showPageButton();

    doLogParam();
}

function doLogParam(){
    var sTime = $("#startTime").val();
    var eTime = $("#endTime").val();

   if(sTime.length==0){
        alert(IDC_STARTTIME_SELECT);
        window.focus(); 
        return;
    }

    if(eTime.length==0){
        alert(IDC_ENDTIME_SELECT);
        window.focus(); 
        return;
    }

    if(sTime>=eTime){
        alert(IDC_STARTTIME_BIGGER_ENDTIME);
        window.focus(); 
        return;
    }
    var itemindex = (currPage-1)*pageNum+1;
    var jcpstr = "getlog -act list -starttime \""+sTime+"\" -endtime \""+eTime+"\" -itemindex  "+itemindex+" -itemnum "+pageNum;
    changePageData(jcpstr);
}

function changePageData(jcpstr){
    $("#tbLog tr:not(:first)").remove();
    GetJCPList({cmd: jcpstr,ParseJCP: function(result){
        var itemlist = result.split(";")[2].split("itemlist=")[1];
        showLog(itemlist);
    }});
}

function showLog(itemlist){
    var itemArr = itemlist.split("#");
    var html = "";
    for(var i=0,len=itemArr.length;i<len-1;i++){
        var subitem = itemArr[i].split("|");
        html += "<tr >"
        html += "<td align='center'>"+subitem[0]+"</td>"
        html += "<td align='center'>"+subitem[4]+"</td>"
        html += "<td align='center'>"+subitem[5]+"</td>"
        html += "</tr>"
    }
    $("#tbLog").append(html);
}

function pagePrev(){
    currPage--;
    showPageButton();
    setPageSel();
    doLogParam();
}

function pageNext(){
    currPage++;
    showPageButton();
    setPageSel();
    doLogParam();
}

function showPageButton(){
    if(currPage==1){
        $("#pagePrev").attr('disabled',true);
        $("#pageNext").attr('disabled',false);
    }else if(currPage==totalPage){
        $("#pagePrev").attr('disabled',false);
        $("#pageNext").attr('disabled',true);
    }else{
        $("#pagePrev").attr('disabled',false);
        $("#pageNext").attr('disabled',false);
    }
}

function setPageSel(){
    $("#selPage").val(currPage);
}
//------------------------------------------------------------------------------系统日志结束



//------------------------------------------------------------------------------告警日志
var totalPageAlarm=0;
var totalNumAlarm=0;
var currPageAlarm=1;
function _init_alarm_log(){
    var language= GetCookieByKey("languages")==0?'zh-cn':'en';
    if(language == 'zh-cn' && g_lan_js_arr[0] == 'russian.js'){
        language = 'rus';
    }
    var webdeflang = GetCookieByKey("webdeflang");
    if (parseInt(webdeflang) == 1) {
        language = 'en';
    }
    $("#startTimeAlarm").click(function(){
        WdatePicker({readOnly:true,lang:language,dateFmt:'yyyy-MM-dd HH:mm:ss'});
    });

    $("#endTimeAlarm").click(function(){
        WdatePicker({readOnly:true,lang:language,dateFmt:'yyyy-MM-dd HH:mm:ss'});
    });

    $("#startTimeAlarm").val(new_date()+" 00:00:00");
    $("#endTimeAlarm").val(new_date()+" 23:59:59");

    cleanAlarmLogData();
}
function searchAlarmLog(){
    var sTime = $("#startTimeAlarm").val();
    var eTime = $("#endTimeAlarm").val();
    var selType = $("#selAlarmType").val();

    if(sTime.length==0){
        alert(IDC_STARTTIME_SELECT);
        window.focus(); 
        return;
    }

    if(eTime.length==0){
        alert(IDC_ENDTIME_SELECT);
        window.focus(); 
        return;
    }

    if(sTime>=eTime){
        alert(IDC_STARTTIME_BIGGER_ENDTIME);
        window.focus(); 
        return;
    }

    cleanAlarmLogData();

    $("#searchAlarmLog").attr("disabled",true);
    $("#searchAlarmLog").after("<span style='color:red;' id='searchAlarmLogTip'>"+IDC_QUERYING+"</span>");

    var itemindex = (currPageAlarm-1)*pageNum+1;
    var jcpstr;
    if(selType >= 80){
        jcpstr = "getalarmevent -act list -starttime \""+sTime+"\" -endtime \""+eTime+"\" -type 8 -alarmchn "+(selType-80)+" -itemindex  "+itemindex+" -itemnum "+pageNum;
    }else{
        jcpstr = "getalarmevent -act list -starttime \""+sTime+"\" -endtime \""+eTime+"\" -type "+selType+" -itemindex  "+itemindex+" -itemnum "+pageNum;
    }

    GetJCPList({cmd: jcpstr,ParseJCP: function(result){
        var arrRst = result.split(";");
        totalNumAlarm = parseInt(arrRst[0].split("=")[1]);
        if(totalNumAlarm>0){
            totalPageAlarm = parseInt(totalNumAlarm/pageNum)+(totalNumAlarm%pageNum==0?0:1);
      
            var itemlist = arrRst[2].split("itemlist=")[1];
            showLogAlarm(itemlist);
            if(totalPageAlarm>1){
                $("#selPageAlarm").attr('disabled',false);
                $("#selPageAlarm").empty();
                for(var j=1;j<=totalPageAlarm;j++){
                    $("#selPageAlarm").append("<option value="+j+">"+j+"</option>");
                }
                $("#pageNextAlarm").attr('disabled',false);
            }else{
                $("#selPageAlarm").append("<option value='1'>1</option>");
            }

        }else{
            $("#tbLogAlarm").append("<tr><td colspan='3' align='center'>"+IDC_SYSLOG_NODATA+"</td></tr>");
        }
        $("#searchAlarmLogTip").remove();
        $("#searchAlarmLog").attr("disabled",false);
    }});
    window.focus(); 

}

function cleanAlarmLogData(){
    $("#selPageAlarm").empty();
    $("#tbLogAlarm tr:not(:first)").remove();
    $("#selPageAlarm").attr('disabled',true);
    $("#pagePrevAlarm").attr('disabled',true);
    $("#pageNextAlarm").attr('disabled',true);
    currPageAlarm=1;
}

function changePageAlarm(){
    currPageAlarm = $("#selPageAlarm").val();

    showPageButtonAlarm();

    doLogParamAlarm();
}

function doLogParamAlarm(){
    var sTime = $("#startTimeAlarm").val();
    var eTime = $("#endTimeAlarm").val();
    var selType = $("#selAlarmType").val();

    if(sTime.length==0){
        alert(IDC_STARTTIME_SELECT);
        window.focus(); 
        return;
    }

    if(eTime.length==0){
        alert(IDC_ENDTIME_SELECT);
        window.focus(); 
        return;
    }

    if(sTime>=eTime){
        alert(IDC_STARTTIME_BIGGER_ENDTIME);
        window.focus(); 
        return;
    }
    var itemindex = (currPageAlarm-1)*pageNum+1;
    var jcpstr;
    if(selType >= 80){
        jcpstr = "getalarmevent -act list -starttime \""+sTime+"\" -endtime \""+eTime+"\" -type 8 -alarmchn "+(selType-80)+" -itemindex  "+itemindex+" -itemnum "+pageNum;
    }else{
        jcpstr = "getalarmevent -act list -starttime \""+sTime+"\" -endtime \""+eTime+"\" -type "+selType+" -itemindex  "+itemindex+" -itemnum "+pageNum;
    }
    changePageDataAlarm(jcpstr);
}

function changePageDataAlarm(jcpstr){
    $("#tbLogAlarm tr:not(:first)").remove();
    GetJCPList({cmd: jcpstr,ParseJCP: function(result){
        var itemlist = result.split(";")[2].split("itemlist=")[1];
        showLogAlarm(itemlist);
    }});
}

function showLogAlarm(itemlist){
    var itemArr = itemlist.split("#");
    var html = "";
    for(var i=0,len=itemArr.length;i<len-1;i++){
        var subitem = itemArr[i].split("|");
        html += "<tr >"
        html += "<td align='center'>"+subitem[0]+"</td>"
        html += "<td align='center'>"+subitem[1]+"</td>"
        html += "<td align='center'>"+subitem[2]+"</td>"
        html += "</tr>"
    }
    $("#tbLogAlarm").append(html);
}

function pagePrevAlarm(){
    currPageAlarm--;
    showPageButtonAlarm();
    setPageSelAlarm();
    doLogParamAlarm();
}

function pageNextAlarm(){
    currPageAlarm++;
    showPageButtonAlarm();
    setPageSelAlarm();
    doLogParamAlarm();
}

function showPageButtonAlarm(){
    if(currPageAlarm==1){
        $("#pagePrevAlarm").attr('disabled',true);
        $("#pageNextAlarm").attr('disabled',false);
    }else if(currPageAlarm==totalPageAlarm){
        $("#pagePrevAlarm").attr('disabled',false);
        $("#pageNextAlarm").attr('disabled',true);
    }else{
        $("#pagePrevAlarm").attr('disabled',false);
        $("#pageNextAlarm").attr('disabled',false);
    }
}

function setPageSelAlarm(){
    $("#selPageAlarm").val(currPageAlarm);
}
//------------------------------------------------------------------------------报警日志结束



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

//单片机升级
function clickTabDmUpdate(){
    GetJCP({cmd: "update -act list", ParseJCP: function(jcpGet){
        $("#versionSpan").html(jcpGet.version);
        $("#versionCamera").html(jcpGet.camera_version);
        if(jcpGet.process > 0)
        {
            updateProgressDm();
        }
    }});
}


function updateProgressDm() {
    $("#progress_div_dm").show();
    $("#spanDiv").show();
    $("#dm_update_confirm").attr("disabled",true);
    $("#file").attr("disabled",true);
    $("#dmprogress").progressbar( "value", 0); 
    $(document).everyTime(3000,"upgadeTime",function(){
        GetJCP({cmd: "update -act list", ParseJCP: function(jcpGet){
            if(jcpGet !== 'Error'){
                var ret = jcpGet.process;
                if(ret == -1) 
                {
                    alert(IDC_UPGRADE_FAILED);
                    $(document).stopTime("upgadeTime");
                    $("#progress_div_dm").hide();
                    $("#spanDiv").hide();
                    $("#dm_update_confirm").attr("disabled",false);
                    $("#file").attr("disabled",false);
                }
                else if (-2 == ret)
                {
                    alert(IDC_UPGRADE_FAILED2);
                    $(document).stopTime("upgadeTime");
                    $("#progress_div_dm").hide();
                    $("#spanDiv").hide();
                    $("#dm_update_confirm").attr("disabled",false);
                    $("#file").attr("disabled",false);
                }
                else if (-3 == ret)
                {
                    alert(IDC_UPGRADE_FAILED3);
                    $(document).stopTime("upgadeTime");
                    $("#spanDiv").hide();
                    $("#progress_div_dm").hide();
                    $("#dm_update_confirm").attr("disabled",false);
                    $("#file").attr("disabled",false);
                }
                else if (-4 == ret)
                {
                    alert(IDC_UPGRADE_FAILED4);
                    $(document).stopTime("upgadeTime");
                    $("#progress_div_dm").hide();
                    $("#spanDiv").hide();
                    $("#dm_update_confirm").attr("disabled",false);
                    $("#file").attr("disabled",false);
                }
                else if (-5 == ret)
                {
                    alert(IDC_UPGRADE_FAILED5);
                    $(document).stopTime("upgadeTime");
                    $("#spanDiv").hide();
                    $("#progress_div_dm").hide();
                    $("#dm_update_confirm").attr("disabled",false);
                    $("#file").attr("disabled",false);
                    location.reload();
                }
                else if (-6 == ret)
                {
                    $(document).stopTime("upgadeTime");
                    alert(IDC_DMUPDATE_TITLE);
                    $("#progress_div_dm").hide();
                    $("#spanDiv").hide();
                    $("#dm_update_confirm").attr("disabled",false);
                    $("#file").attr("disabled",false);
                }
                else if (-7 == ret)
                {
                    $(document).stopTime("upgadeTime");
                    alert(IDC_DMUPGRADE_FAILED7);
                    $("#progress_div_dm").hide();
                    $("#spanDiv").hide();
                    $("#dm_update_confirm").attr("disabled",false);
                    $("#file").attr("disabled",false);
                }
                else if(ret == 100) 
                {
                    $(document).stopTime("upgadeTime");
                    $("#dmprogress").progressbar( "value", 100); 
                    $("#dmprogress_lab").html(100 + "%");
                    $("#spanDivDM").html(IDC_UPDATE+IDC_SUCCESS+IDC_SERVER_RESTART_PROMPT)
                    dmUpdateSuccess();
                }
                $(document).ready(function() { 
                    $("#dmprogress").progressbar({ value : parseInt(ret) });
                    $("#dmprogress_lab").html(ret + "%");
                });
            }

        }});
     });
}

var dm_success_p = 0;
var dm_success_progress = 0;
function dmUpdateSuccess(){
    $("#dmprogress").progressbar("value", dm_success_p);
    if(dm_success_p == 100){
        $("#progress_div_dm").hide()
        clearTimeout(dm_success_progress);
        $("#spanDiv").hide()
        window.top.location = "/login.asp";
    }else{
        dm_success_p++;
        dm_success_progress = setTimeout(dmUpdateSuccess,750);
    }
}

function checkFilePath(){
    var filepath = $("#file").val();
    if(filepath !== ''){
        $("#trFileName").show();
        var l = filepath.lastIndexOf("\\");
        $("#fileFullPath").html(filepath.substring(l+1));
    }else{
        $("#trFileName").hide();
    }
}

function DmIframeUpdate() {
    if($("#file").val() == ""){
        alert(IDC_UPDATE_TYPE_ERROR);
        return false;
    }

    if (window.confirm(IDC_UPDATE_CONFIRM)) 
    {
        var filepath = $("#file").val();
        var temp = filepath.split(".");
        if (temp[temp.length - 1].toLowerCase() == "tgz") {
            document.dmFrmUpdate.action="/webs/updateDMCfg";
            document.dmFrmUpdate.submit();

            $("#progress_div_dm").show();
            $("#spanDiv").show();
            $("#dmprogress").progressbar( "value", 0); 
            window.setTimeout(updateProgressDm,5000);
        }
        else
        {
           alert(IDC_UPDATE_TYPE_ERROR);
           var dmfile = $("#file");
           dmfile.after(dmfile.clone().val(''));
           dmfile.remove(); 
           $("#trFileName").hide();
        }
    } 
    window.focus();
}


function showDMUpdate(){
    if($.cookie("has_MCUupgrade") == 1){
        $("#liDMUpdate").show();
    }
}

function showPortSetting(){
    if($.cookie("has_dome") != 1){
        $("#liPortSetting").hide();
        //球机地址初始化
        for (i = 1; i < 256; i++){
            $("#selAddr").append('<option value="' + i + '">' + i + '</option>');
        }
        $("#selProtocol").append('<option value="PELCO_D">PELCO_D</option>');
    }
}

function _init_port_setting(){
    try{
        var boardmode = parseInt(GetCookieByKey("boardmode"));
        if (5 == parseInt(boardmode) || 18 == parseInt(boardmode)){
            $("#tabs-10 select").attr("disabled", true);
        }
  
        GetJCPList({cmd: "ptzcfg -act list", ParseJCP: function(jcpstr){
            if(jcpstr != 'Error'){         
                 //$("#comtype").val(GetRtspKeyStr(jcpstr, 'type'));  
                 $("#stopbits").val(GetRtspKeyStr(jcpstr, 'stop')); 
                 $("#databits").val(GetRtspKeyStr(jcpstr, 'data')); 
                 $("#checktype").val(GetRtspKeyStr(jcpstr, 'parity')); 
                 $("#baudrate").val(GetRtspKeyStr(jcpstr, 'baud')); 
                 $("#selAddr").val(GetRtspKeyStr(jcpstr, 'addr')); 
                 $("#selProtocol").val(GetRtspKeyStr(jcpstr, 'protocol'));
            }
               
        }});
        
    }
    catch(E){}
}

function SaveSerialPort(){
    var type = $("#comtype").val();
    var stop = $("#stopbits").val();
    var data = $("#databits").val();
    var parity = $("#checktype").val();
    var baud = $("#baudrate").val();
    var addr = $("#selAddr").val();
    var protocol = $("#selProtocol").val();
    
    var jcpstr = "ptzcfg -act set  -stop " + stop + " -data " + data + " -parity " + parity + " -baud " + baud + " -addr " + addr + " -protocol " + protocol;
    GetJCP({cmd: jcpstr});
    
    alert(IDC_MSGBOX_SAVEOK);
    window.focus();
    return 0;
}
