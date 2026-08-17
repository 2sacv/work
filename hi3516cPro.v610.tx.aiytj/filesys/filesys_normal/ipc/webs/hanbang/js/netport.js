$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
	   if($.cookie("graintype") == 0){
          $("#lahttpen").hide();
          $("#httpen").hide();
          $("#webPort_text").hide();
          
          $("#lartspen").hide();
          $("#rtspen").hide();
          $("#rtspPort_text").hide();
          $("#trFtp").hide();
       }
		
       GetJCP({cmd: "portcfg -act list", ParseJCP: ParsePortCfg});
       GetJCP({cmd: "upnpcfg -act list", ParseJCP: ParseUpnpCfg});
    }
})

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
        parent.paramFailTip("Rtsp " + IDC_GEN_PORT_RANGE);
        window.focus();
        return false;
    }
    if (isBlank(webport) || webport > 65535 || 1 > webport)
    {
        parent.paramFailTip("Web " + IDC_GEN_PORT_RANGE);
        window.focus();
        return false;
    }
    if (isBlank(ftpport) || ftpport > 65535 || 1 > ftpport)
    {
        parent.paramFailTip("Ftp " + IDC_GEN_PORT_RANGE);
        window.focus();
        return false;
    }
    if (isBlank(voiceport) || voiceport > 65535 || 1 > voiceport)
    {
        parent.paramFailTip("Speak " + IDC_GEN_PORT_RANGE);
        window.focus();
        return false;
    }
    if (isBlank(updateport) || updateport > 65535 || 1 > updateport)
    {
        parent.paramFailTip("Update " + IDC_GEN_PORT_RANGE);
        window.focus();
        return false;
    }

     //判断任意的两个端口号是否相同
    var ArrPort = new Array(webport, ftpport, rtspport, voiceport, updateport);

    if (!PortCheck(ArrPort))
    {
        parent.paramFailTip(IDC_SAME_PORT_MSG);
        window.focus();
        return false;
    }

    Set_cookie("webport",webport);
    Set_cookie("ftport",ftpport);
    
    try
    {
        var jcpstr = "portcfg -act set" + " -rtsp " + rtspport + " -web " + webport + " -ftp " + ftpport + " -audio " + voiceport + " -update " + updateport;
        GetJCP({cmd: jcpstr});
        
        var jcpset = "upnpcfg -act set" + " -rtspen " + (document.all.rtspen.checked == true ? 1 : 0) + " -httpen " + (document.all.httpen.checked == true ? 1 : 0)
                    + " -ftpen " + (document.all.ftpen.checked == true ? 1 : 0) + " -voiceen " + (document.all.voiceen.checked == true ? 1 : 0)
                    + " -updateen " + (document.all.updateen.checked == true ? 1 : 0);
        GetJCP({cmd: jcpset});
        parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
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