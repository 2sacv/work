  $(function(){
        var lf = $.cookie("loginflag_"+g_hostname);
        if (null === lf)
        {
           parent.location.href = "/login.asp";
        }else{
            timeSysStatus();
        }
    })

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