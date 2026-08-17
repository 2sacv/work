$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
        showVideo();
        showPrivacyInfo();
    }
})

var wndWidth = 350; //视频宽度
var wndHeight = 250; //视频高度
var veWidth=1920, veHeight=1080; //用插件获取到的尺寸
function showVideo(){
    if(g_is_msie){
      $("#objects").html('<object id="IPCamera" name="IPCamera" CLASSID="CLSID:2319F6E6-ABD3-4b68-BADF-05D8796FA072" width="'+wndWidth+'" height="'+wndHeight+'">></object>');
    }else{
      $("#objects").html('<object id="IPCamera" name="IPCamera" type="application/npipcam" width="'+wndWidth+'" height="'+wndHeight+'">></object>');
    }
    document.IPCamera.IPCSetWindowMode(1);
    var type = GetCookieByKey("ljtypes");
    type = type== -1?1:type;
    document.IPCamera.IPCStartPreviewEx(0, GetCookieByKey("url"), 0, parseInt(type), GetCookieByKey("rtspport"),GetCookieByKey("user"), GetCookieByKey("passwd"), GetCookieByKey("stream"),"V2.00");
}

var videoMaskArr = [
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
];

var videoMaskColor;


//视频遮挡区域选择,启用隐藏触发事件
function AreaMaskSelect(){
     var v = parseInt($("#selAreaMask").val());
     var masken = videoMaskArr[v][0];
     if(masken==1){
        $("#checkMask").prop("checked",true);
        document.IPCamera.IPCSetMDModeEx(0, true);
        document.IPCamera.IPCSetMDAreaRectEx(parseInt(0),
                                                videoMaskArr[v][1] * wndWidth / veWidth,
                                                videoMaskArr[v][2]  * wndHeight / veHeight,
                                                videoMaskArr[v][3]  * wndWidth / veWidth,
                                                videoMaskArr[v][4]  * wndHeight / veHeight);

        document.IPCamera.IPCShowMDAreaEx(0, true);
        document.IPCamera.IPCSetMDAreaTitleEx(0, parseInt(v)+1);
    }
    else {     
        $("#checkMask").prop("checked",false);
        document.IPCamera.IPCShowMDAreaEx(0, false);
    }
}

//视频遮挡启动
function MaskEnableSelect(){
     var v = $("#selAreaMask").val();
     var enable = $("#checkMask").is(":checked");
     
     if(enable){
        document.IPCamera.IPCSetMDModeEx(0, true);
        document.IPCamera.IPCSetMDAreaRectEx(parseInt(0),
                                                parseInt(videoMaskArr[v][1] * wndWidth / veWidth),
                                                parseInt(videoMaskArr[v][2]  * wndHeight / veHeight),
                                                parseInt(videoMaskArr[v][3]  * wndWidth / veWidth),
                                                parseInt(videoMaskArr[v][4]  * wndHeight / veHeight));

        document.IPCamera.IPCShowMDAreaEx(0, true);
        document.IPCamera.IPCSetMDAreaTitleEx(0, parseInt(v)+1);
    }
    else {     
        document.IPCamera.IPCShowMDAreaEx(0, false);
    }
}

//初始化视频遮挡信息
function showPrivacyInfo(){
    GetJCPList({cmd: "videomaskcfg -act list", ParseJCP: function(jcpstr){
        var extArr = jcpstr.split("#");
        for(var i=0; i<8;i++){
            var extObj = parse_jcp_content(extArr[i]);
            videoMaskArr[i][0] = extObj.masken; 
            videoMaskArr[i][1] = extObj.left;
            videoMaskArr[i][2] = extObj.top;
            videoMaskArr[i][3] = extObj.right;
            videoMaskArr[i][4] = extObj.bottom;

            if(i==0){
                 $("#selMaskColor").val(extObj.color);
                 $("#selMaskColor").css("background", $("#selMaskColor option:eq(" + extObj.color + ")").attr("bgvalue"));
                
                 if(extObj.masken==1){
                    $("#checkMask").prop("checked",true);
                    document.IPCamera.IPCSetMDModeEx(0, true);
                    document.IPCamera.IPCSetMDAreaRectEx(0,
                            videoMaskArr[0][1] * wndWidth / veWidth,
                            videoMaskArr[0][2]  * wndHeight / veHeight,
                            videoMaskArr[0][3]  * wndWidth / veWidth,
                            videoMaskArr[0][4]  * wndHeight / veHeight);
                    document.IPCamera.IPCShowMDAreaEx(0, true);
                    document.IPCamera.IPCSetMDAreaTitleEx(0, 1);

                 }
            }
        }

    }});
}

//视频遮挡颜色变化
function SetColorChg(){
    $("#selMaskColor").css("background", $("#selMaskColor option:eq(" + $("#selMaskColor").val() + ")").attr("bgvalue"));
    window.focus();
}

//保存视频遮挡
function SaveVideoMask(){
    var v =  parseInt($("#selAreaMask").val());
    var masken = $("#checkMask").is(":checked")?1:0;
    var c = $("#selMaskColor").val();
    var winPosStr = document.IPCamera.IPCGetVideoWndRect(0);

    var l = GetRtspKeyStr(winPosStr, "left");
    var t = GetRtspKeyStr(winPosStr, "top");
    var r = GetRtspKeyStr(winPosStr, "right");
    var b = GetRtspKeyStr(winPosStr, "bottom");

    l = parseInt(l*veWidth/wndWidth);
    t = parseInt(t*veHeight/wndHeight);
    r = parseInt(r*veWidth/wndWidth);
    b = parseInt(b*veHeight/wndHeight);

    videoMaskArr[v][0] = masken;
    videoMaskArr[v][1] = l;
    videoMaskArr[v][2] = t;
    videoMaskArr[v][3] = r;
    videoMaskArr[v][4] = b;

    var jcpstr = "videomaskcfg -act set -maskid "+v+" -masken "+masken+" -color "+c+"  -left "+l+" -top "+t+" -right "+r+" -bottom "+b;

    GetJCP({cmd: jcpstr,ParseJCP: function(jcpstr){
        if(jcpstr=='Error'){
           parent.paramFailTip(IDC_MSGBOX_SAVEFAIL);
        }else{
           parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
        }
    }});
    window.focus();
}

//删除
function DelVideoMask(){
    var v =  parseInt($("#selAreaMask").val());
    document.IPCamera.IPCSetMDModeEx(0, true);
    document.IPCamera.IPCShowMDAreaEx(0, false);
    videoMaskArr[v][0] = 0;
    videoMaskArr[v][1] = 0;
    videoMaskArr[v][2] = 0;
    videoMaskArr[v][3] = 0;
    videoMaskArr[v][4] = 0;
    var jcpstr = "videomaskcfg -act set -maskid "+v+" -masken 0  -left 0 -top 0 -right 0 -bottom 0";
    GetJCP({cmd: jcpstr,ParseJCP: function(jcpstr){
        if(jcpstr=="Error"){
            parent.paramFailTip(IDC_MSGBOX_DELETEFAIL);
            window.focus();
        }else{
            $("#checkMask").prop("checked",false);
            parent.paramSaveTip(IDC_MSGBOX_DELETEOK);
            window.focus();
        }
    }});
}
