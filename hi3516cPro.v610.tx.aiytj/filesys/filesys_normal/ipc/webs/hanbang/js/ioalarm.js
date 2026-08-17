$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
        GetJCP({cmd: "alarmoutcfg -act list ", ParseJCP: ParseAlarmoutCfg});
    }
})

function ParseAlarmoutCfg(jcpobj){
    try
    {
        $("input[name='ao0status'][value=" + parseInt(jcpobj.ao0) + "]").attr("checked",true);
        $("input[name='ao1status'][value=" + parseInt(jcpobj.ao1) + "]").attr("checked",true);
        $("input[name='ao0level'][value=" + parseInt(jcpobj.ao0alarm) + "]").attr("checked",true);
        $("input[name='ao1level'][value=" + parseInt(jcpobj.ao1alarm) + "]").attr("checked",true);
        $("#aoholdtime").val(jcpobj.holdtime);
    }catch(e){
    }
}

function SaveLinkSet(){
    var aoholdtime = $("#aoholdtime").val();

    if (isBlank(aoholdtime) || aoholdtime > 36000 || 1 > aoholdtime)
    {
        parent.paramFailTip(IDC_AO_HOLDTIME_MSG + "1~36000");//报警保持时间
        window.focus();
        return false;
    }

    var jcpstr = "alarmoutcfg -act set " +  " -ao0 " + parseInt($('input:radio[name="ao0status"]:checked').val())
                + " -ao1 " + parseInt($('input:radio[name="ao1status"]:checked').val()) + " -ao0alarm " + 
                parseInt($('input:radio[name="ao0level"]:checked').val()) + " -ao1alarm " + parseInt($('input:radio[name="ao1level"]:checked').val())
                + " -holdtime " + aoholdtime;
    GetJCP({cmd: jcpstr});

    parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
    window.focus();
}