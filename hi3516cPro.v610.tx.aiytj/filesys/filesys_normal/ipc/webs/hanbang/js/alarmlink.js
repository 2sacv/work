var  ptz_alarm_in =  $.cookie("has_alarmin");
var  ptz_alarm_out = $.cookie("has_alarmout");
var  has_dome = $.cookie("has_dome");
$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
        $("#tbAlarmLink tr").find('td:eq(0)').css("background-color","rgb(210,213,221)");
        initAlarmLink();
    }
})

function initAlarmLink(){
    /*if(ptz_alarm_in > 0){

        //获取报警输入配置信息
        GetJCPList({cmd: "ca2aicfg -act list", async: false , ParseJCP: function(jcpobj){
            if(jcpobj != 'Error'){
                var alarmArr = jcpobj.split("#");
                var num= alarmArr[0].split(";")[0].split("=")[1];
                for(var i=0;i<num;i++){
                    aiLinkage[i][0] = parseInt(GetRtspKeyStr(alarmArr[i],"interval"));
                    aiLinkage[i][1] = parseInt(GetRtspKeyStr(alarmArr[i],"acen"));
                    aiLinkage[i][2] = parseInt(GetRtspKeyStr(alarmArr[i],"emailen"));
                    aiLinkage[i][3] = parseInt(GetRtspKeyStr(alarmArr[i],"ao0en"));
                    aiLinkage[i][4] = parseInt(GetRtspKeyStr(alarmArr[i],"ao1en"));
                    aiLinkage[i][5] = parseInt(GetRtspKeyStr(alarmArr[i],"recorden"));
                    aiLinkage[i][6] = parseInt(GetRtspKeyStr(alarmArr[i],"ftpen"));
                    aiLinkage[i][7] = parseInt(GetRtspKeyStr(alarmArr[i],"sounden"));
                    aiLinkage[i][8] = parseInt(GetRtspKeyStr(alarmArr[i],"captureen"));
                    aiLinkage[i][9] = parseInt(GetRtspKeyStr(alarmArr[i],"linktype"));
                    aiLinkage[i][10] = parseInt(GetRtspKeyStr(alarmArr[i],"preset"));

                    SetAiCheck();//设置报警输入报警
                }
            }
                
        }});
        
    }*/

   
    //FtpClient
    //GetJCP({cmd: "ftpclicfg -act list", ParseJCP: function(jcpobj){
     //   $("input[name='cbFtp'][value="+parseInt(jcpobj.type)+"]").attr("checked",true);
    //}});

    //视频丢失联动
    GetJCP({cmd: "ca2vlcfg -act list", ParseJCP: function(jcpobj){
        $("#cbVLEmail").prop("checked",parseInt(jcpobj.emailen)==1);
        //$("#cbVLAO").prop("checked",parseInt(jcpobj.ao0en)==1);
        $("#cbVLInterval").val(jcpobj.interval);
    }});
    //移动侦测联动
    GetJCP({cmd: "ca2mdcfg -act list", ParseJCP: function(jcpobj){
        $("#cbMDEmail").prop("checked",parseInt(jcpobj.emailen)==1);
        //$("#cbMDRecord").prop("checked",parseInt(jcpobj.recorden)==1);
        //$("#cbMDFtp").prop("checked",parseInt(jcpobj.ftpen)==1);
        //$("#cbMDCapture").prop("checked",parseInt(jcpobj.captureen)==1);
        //$("#cbMDAO").prop("checked",parseInt(jcpobj.ao0en)==1);
        $("#cbMDInterval").val(jcpobj.interval);
    }});
    //视频遮挡联动                                      
    GetJCP({cmd: "ca2vmcfg -act list", ParseJCP: function(jcpobj){
        $("#cbVMEmail").prop("checked",parseInt(jcpobj.emailen)==1);
        //$("#cbVMRecord").prop("checked",parseInt(jcpobj.recorden)==1);
        //$("#cbVMFtp").prop("checked",parseInt(jcpobj.ftpup)==1);
        //$("#cbVMCapture").prop("checked",parseInt(jcpobj.captureen)==1);
        //$("#cbVMAO").prop("checked",parseInt(jcpobj.ao0en)==1);
        $("#cbVMInterval").val(jcpobj.interval);
    }});
    //网线断开联动
    /*GetJCP({cmd: "ca2linkbroken -act list", ParseJCP: function(jcpobj){
        $("#cbNETRecord").prop("checked",parseInt(jcpobj.recorden)==1);
        $("#cbNETCapture").prop("checked",parseInt(jcpobj.captureen)==1);
        $("#cbNETAO").prop("checked",parseInt(jcpobj.ao0en)==1);
        $("#cbNETInterval").val(jcpobj.interval);
    }});*/
    //磁盘错误
    /*GetJCP({cmd: "ca2diskerror -act list", ParseJCP: function(jcpobj){
        $("#cbDEEmail").prop("checked",parseInt(jcpobj.emailen)==1);
        //$("#cbDEAO").prop("checked",parseInt(jcpobj.ao0en)==1);
        $("#cbDEInterval").val(jcpobj.interval);
    }});*/
    //IP冲突联动
    /*GetJCP({cmd: "ca2ipconflict -act list", ParseJCP: function(jcpobj){
        $("#cbIPRecord").prop("checked",parseInt(jcpobj.recorden)==1);
        $("#cbIPCapture").prop("checked",parseInt(jcpobj.captureen)==1);
        $("#cbIPAO").prop("checked",parseInt(jcpobj.ao0en)==1);
        $("#cbIPInterval").val(jcpobj.interval);
    }});*/

    //越界侦测报警联动
    GetJCP({cmd: "ca2vglinecfg -act list", ParseJCP: function(jcpobj){
        $("#cbVGLineEmail").prop("checked",parseInt(jcpobj.emailen)==1);
        $("#cbVGLineInterval").val(jcpobj.interval);
    }});

    //区域侦测报警联动
    GetJCP({cmd: "ca2vgrectcfg -act list", ParseJCP: function(jcpobj){
        $("#cbVGRectEmail").prop("checked",parseInt(jcpobj.emailen)==1);
        $("#cbVGRectInterval").val(jcpobj.interval);
    }});

	//人形侦测报警联动
    GetJCP({cmd: "ca2humandetectcfg -act list", ParseJCP: function(jcpobj){
        $("#cbHumanEmail").prop("checked",parseInt(jcpobj.emailen)==1);
        $("#cbHumanInterval").val(jcpobj.interval);
    }});

	GetJCP({cmd: "alarmaudio -act list", ParseJCP: function(jcpobj){
        $("input[name='alarmsoundtype'][value=" + parseInt(jcpobj.alarm_type) + "]").attr("checked",true);//启用
    }});
}


function AutoSaveAiChk(obj, index)
{
    var selindex = parseInt($("#aisel").val());
    var temp = 0;
    var tempStr;
    if (obj == null)  return false;
    
    if (0 > index || index > 10)  return false;
   
    if(0 == index){
        var  v = $("#aiInterval").val();
        if(v<3) v=3;
        if(v>60) v=60;
        
        $("#aiInterval").val(v);
        aiLinkage[selindex][index] = v;
        aiLinkageChg[selindex] = 1;
    }else if (index == 9 || index == 10) {
        aiLinkageChg[selindex] = 1;
        aiLinkage[selindex][index] = $(obj).val();

        if(index == 9){
            if($(obj).val() == 0){
                $("#ptzParam").html('').attr("disabled",true);
            }else if($(obj).val() == 1){
                //$("#ptzParam").html(_prseset_html).attr("disabled",false);
            }else{
                //$("#ptzParam").html(_no_preset_html).attr("disabled",false);
            }
        }
    }else{
        if (obj.type.toLowerCase() == "checkbox")
        {
            //设置参数值是否变动的标志
            if (obj.checked != aiLinkage[selindex][index])
            {
                aiLinkageChg[selindex] = 1;
            }
            if (obj.checked == false)
            {
                aiLinkage[selindex][index] = 0;
            }
            else if (obj.checked == true)
            {
                aiLinkage[selindex][index] = 1;
            }
        }
        else if (obj.type.toLowerCase() == "text")
        {
            //设置参数值是否变动的标志
            if (obj.value != aiLinkage[selindex][index])
            {
                aiLinkageChg[selindex] = 1;
            }
            aiLinkage[selindex][index] = obj.value;
        }
    }
    window.focus();
}

/*====================================================================
    设置报警输入参数
====================================================================*/
function SetAiCheck()
{
    var selIndex = parseInt($("#aisel").val());
     
    var objId = new Array("chkAIAC", "chkAIEmail", "chkAIAO0", "chkAIAO1", "chkAIRcrd", "chkAIFtp", "chkAISound", "chkAICapture");
    for (var i = 0; i < objId.length; i ++)
    {
        $("#" + objId[i]).prop("checked", aiLinkage[selIndex][i + 1] == 1);
    }

    if(aiLinkage[selIndex][9] == 0){
        $("#ptzParam").html('').attr("disabled",true);
    }else if(aiLinkage[selIndex][9] == 1){
        //$("#ptzParam").html(_prseset_html).attr("disabled",false);
    }else{
        //$("#ptzParam").html(_no_preset_html).attr("disabled",false);
    }
    $("#ptzAction").val(aiLinkage[selIndex][9]);
    $("#ptzParam").val(aiLinkage[selIndex][10]);
    $("#aiInterval").val(aiLinkage[selIndex][0]);
    window.focus();
}

function changeInterval(oid){
    var v = $("#"+oid).val();
    if(v<3)v=3;
    if(v>60)v=60;
    $("#"+oid).val(v);
}

function mdFtpCheck(){
    if($("#cbMDFtp").is(":checked") && $("input[name='cbFtp']:checked").val()==1){
        $("#cbMDRecord").prop("checked","checked");
    }
}

function vmFtpCheck(){
    if($("#cbVMFtp").is(":checked") && $("input[name='cbFtp']:checked").val()==1){
        $("#cbVMRecord").prop("checked","checked");
    }
}

function SaveAlarmLink(){
    //if($("#cbMDFtp").is(":checked") && $("input[name='cbFtp']:checked").val()==1){
     //   $("#cbMDRecord").prop("checked","checked");
    //}

    //if($("#cbVMFtp").is(":checked") && $("input[name='cbFtp']:checked").val()==1){
    //    $("#cbVMRecord").prop("checked","checked");
    //}

    //var cbFtp = parseInt($("input[name='cbFtp']:checked").val());

    var VLEmail = $("#cbVLEmail").is(":checked")?1:0;
    //var VLAO = $("#cbVLAO").is(":checked")?1:0;
    var VLInterval = $("#cbVLInterval").val();


    //var DEEmail = $("#cbDEEmail").is(":checked")?1:0;
    //var DEAO = $("#cbDEAO").is(":checked")?1:0;
    //var DEInterval = $("#cbDEInterval").val();

    var VMEmail = $("#cbVMEmail").is(":checked")?1:0;
    //var VMRecord = $("#cbVMRecord").is(":checked")?1:0;
    //var VMFtp = $("#cbVMFtp").is(":checked")?1:0;
    //var VMCapture = $("#cbVMCapture").is(":checked")?1:0;
    //var VMAO = $("#cbVMAO").is(":checked")?1:0;
    var VMInterval = $("#cbVMInterval").val();
    

    var MDEmail = $("#cbMDEmail").is(":checked")?1:0;
    var MDAudio = $("#cbMDAudio").is(":checked")?1:0;
    //var MDRecord = $("#cbMDRecord").is(":checked")?1:0;
    //var MDFtp = $("#cbMDFtp").is(":checked")?1:0;
    //var MDCapture = $("#cbMDCapture").is(":checked")?1:0;
    //var MDAO = $("#cbMDAO").is(":checked")?1:0;
    var MDInterval = $("#cbMDInterval").val();
    
    //var NETRecord = $("#cbNETRecord").is(":checked")?1:0;
    //var NETCapture = $("#cbNETCapture").is(":checked")?1:0;
    //var NETAO = $("#cbNETAO").is(":checked")?1:0;
    //var NETInterval = $("#cbNETInterval").val();

    //var IPRecord = $("#cbIPRecord").is(":checked")?1:0;
    //var IPCapture = $("#cbIPCapture").is(":checked")?1:0;
    //var IPAO = $("#cbIPAO").is(":checked")?1:0;
    //var IPInterval = $("#cbIPInterval").val();

    var VGLineEmail = $("#cbVGLineEmail").is(":checked")?1:0;
    var VGLineAudio = $("#cbVGLineAudio").is(":checked")?1:0;
    var VGLineInterval = $("#cbVGLineInterval").val();

    
    var VGRectEmail = $("#cbVGRectEmail").is(":checked")?1:0;
    var VGRectAudio = $("#cbVGRectAudio").is(":checked")?1:0;
    var VGRectInterval = $("#cbVGRectInterval").val();

	var VGHumanEmail = $("#cbHumanEmail").is(":checked")?1:0;
    var VGHumanAudio = $("#cbHumanAudio").is(":checked")?1:0;
    var VGHumanInterval = $("#cbHumanInterval").val();

    var jcpstr = "ca2vlcfg -act set -interval "+parseInt(VLInterval)+" -emailen "+VLEmail;
    GetJCP({cmd: jcpstr});

    //var jcpstr = "ca2diskerror -act set -interval "+parseInt(DEInterval)+" -emailen "+DEEmail;
    //GetJCP({cmd: jcpstr});

    var jcpstr = "ca2vmcfg -act set -interval "+parseInt(VMInterval)+" -emailen "+VMEmail;
    GetJCP({cmd: jcpstr});

    var jcpstr = "ca2mdcfg -act set -interval "+parseInt(MDInterval)+" -emailen "+MDEmail;
    GetJCP({cmd: jcpstr});

    var jcpstr = "ca2vglinecfg -act set -interval "+parseInt(VGLineInterval)+" -emailen "+VGLineEmail;
    GetJCP({cmd: jcpstr});

    
    var jcpstr = "ca2vgrectcfg -act set -interval "+parseInt(VGRectInterval)+" -emailen "+VGRectEmail;
    GetJCP({cmd: jcpstr});
	
    var jcpstr = "ca2humandetectcfg -act set -interval "+parseInt(VGHumanInterval)+" -emailen "+VGHumanEmail;
    GetJCP({cmd: jcpstr});

    //同时项FtpClient设置发送命令
    //jcpstr = "ftpclicfg -act set  -type " + cbFtp;
    //GetJCP({cmd: jcpstr});

    //var jcpstr = "ca2linkbroken -act set -interval "+parseInt(NETInterval)+" -captureen "+NETCapture+" -recorden "+NETRecord + " -ao0en "+NETAO;
    //GetJCP({cmd: jcpstr});
   // var jcpstr = "ca2ipconflict -act set -interval "+parseInt(IPInterval)+" -captureen "+IPCapture+" -recorden "+IPRecord + " -ao0en "+IPAO;
    //GetJCP({cmd: jcpstr});

   /* if(ptz_alarm_in >0){
        for (var i = 0; i < ptz_alarm_in; i++)
        {
            if (aiLinkageChg[i] == 1) //判断各通道数据是否改变
            {
               jcpstr = "ca2aicfg -act set -aisel " + i + " -interval " + aiLinkage[i][0] + " -acen " + aiLinkage[i][1] + " -emailen " + aiLinkage[i][2]
                           + " -ao0en " + aiLinkage[i][3] + " -ao1en " + aiLinkage[i][4] + " -recorden " + aiLinkage[i][5] + " -ftpen " + aiLinkage[i][6]
                           + " -sounden " + aiLinkage[i][7] + " -captureen " + aiLinkage[i][8] + " -linktype " + aiLinkage[i][9] + " -preset " + aiLinkage[i][10];
               GetJCP({cmd: jcpstr});
            }
        }
    }*/

    parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
    window.focus();
}

