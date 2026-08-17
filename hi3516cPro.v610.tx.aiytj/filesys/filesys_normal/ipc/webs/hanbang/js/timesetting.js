$(function(){
        var lf = $.cookie("loginflag_"+g_hostname);
        if (null === lf)
        {
           parent.location.href = "/login.asp";
        }else{
            _init_load();
        }
    })

   function _init_load(){
      GetJCP({cmd: "timecfg -act list",ParseJCP: initTimeSetting});
      GetJCP({cmd: "ntpcfg -act list",ParseJCP: initNtpSetting});
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
            parent.paramSaveTip(IDC_MSGBOX_SAVEOK);      
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
        parent.paramFailTip(IDC_TIME_DATE_FAIL); 
        return false;
    }

    var ss = date.split("-");
    if(ss[0]<1970){
        parent.paramFailTip(IDC_TIME_DATE_YEAR_FAIL); 
        return false;
    }
    var reg=/^((20|21|22|23|[0-1]\d)\:[0-5][0-9])(\:[0-5][0-9])?$/;    
    if(!reg.test(time)){   
        parent.paramFailTip(IDC_TIME_TIME_FAIL);
        return false;
    }    
    
    var st = time.split(":");
    if (st[0] > 24)
    {
        parent.paramFailTip(IDC_TIME_TIME_FAIL);
        return false;
    }
    
    if (st[1] > 59)
    {
        parent.paramFailTip(IDC_TIME_TIME_FAIL);
        return false;
    }
    
    if (st[2] > 59)
    {
        parent.paramFailTip(IDC_TIME_TIME_FAIL);
        return false;
    }
    
    var sdate = new Date(ss[0], ss[1] - 1, ss[2], st[0], st[1], st[2]);
    var jcpstr = "timecfg -act set -time " + (sdate / 1000);
    GetJCP({cmd: jcpstr, ParseJCP: function(result){
            if(result == 'Error'){
                parent.paramFailTip(IDC_MSGBOX_SAVEFAIL);
            }else{
                parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
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
          parent.paramFailTip(IDC_SERVERADDR+IDC_NOT_CONTAIN+"\"'<>%;)(&+“”[]!！？?\\#￥$^；【】。");
          window.focus();
          return;
    }
    
    var cArr = address.match(/[^\x00-\xff]/ig);   
    var len =  address.length + (cArr == null ? 0 : cArr.length*2);   

    if(len==0 || len>63){
        parent.paramFailTip(IDC_NTP_ADDRESS_ERROR);
        window.focus();
        return;
    }
    if(port.length==0 || isNaN(port) || port==0 || port>65535){
        parent.paramFailTip(IDC_NTP_PORT_ERROR);
        window.focus();
        return;
    }

    if(time.length==0 || isNaN(time) || time==0 || time>65535){
        parent.paramFailTip(IDC_NTP_SYNTIME_ERROR);
        window.focus();
        return;
    }

    var ntpen = $("#ntpserviceen").is(":checked")?1:0;
    var jcpstr = "ntpcfg -act set -ntpen "+parseInt(ntpen)+" -ntphost "+address+" -ntpport ";
    jcpstr+= port+" -interval "+time;
    GetJCP({cmd: jcpstr});
    parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
    window.focus();
    return;
}