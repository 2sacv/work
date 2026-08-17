$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
        showVideo();
        showRoiInfo();
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

var roiMaskArr = [
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
];

//ROI启动
function RoiEnableSelect(){
     var v = $("#selRoi").val();
     var enable = $("#checkRoi").is(":checked");
     
     if(enable){
        document.IPCamera.IPCSetMDModeEx(0, true);
        document.IPCamera.IPCSetMDAreaRectEx(parseInt(0),
                                                parseInt(roiMaskArr[v][1] * wndWidth / veWidth),
                                                parseInt(roiMaskArr[v][2]  * wndHeight / veHeight),
                                                parseInt(roiMaskArr[v][3]  * wndWidth / veWidth),
                                                parseInt(roiMaskArr[v][4]  * wndHeight / veHeight));

        document.IPCamera.IPCShowMDAreaEx(0, true);
        document.IPCamera.IPCSetMDAreaTitleEx(0, parseInt(v)+1);
    }
    else {     
        document.IPCamera.IPCShowMDAreaEx(0, false);
    }
}

//初始化视频遮挡信息
function showRoiInfo(){
    GetJCPList({cmd: "roicfg -act list", ParseJCP: function(jcpstr){
        var extArr = jcpstr.split("#");
        for(var i=0; i<8;i++){
            var extObj = parse_jcp_content(extArr[i]);
            roiMaskArr[i][0] = parseInt(extObj.enable); 
            roiMaskArr[i][1] = parseInt(extObj.left);
            roiMaskArr[i][2] = parseInt(extObj.top);
            roiMaskArr[i][3] = parseInt(extObj.right);
            roiMaskArr[i][4] = parseInt(extObj.bottom);

            if(i==0){
                  if(extObj.enable==1){
                    $("#checkRoi").prop("checked",true);
                    document.IPCamera.IPCSetMDModeEx(0, true);
                    document.IPCamera.IPCSetMDAreaRectEx(parseInt(0),
                            parseInt(roiMaskArr[0][1] * wndWidth / veWidth),
                            parseInt(roiMaskArr[0][2]  * wndHeight / veHeight),
                            parseInt(roiMaskArr[0][3]  * wndWidth / veWidth),
                            parseInt(roiMaskArr[0][4]  * wndHeight / veHeight));
                    document.IPCamera.IPCShowMDAreaEx(0, true);
                    document.IPCamera.IPCSetMDAreaTitleEx(0, 1);

                 }
            }
        }

    }});
}

var roi_index = 0;
function changeRoiIndex(){
    roi_index = parseInt($("#roi_option").val());
}

//ROI选择区域
function RoiSelect(){
    var v = parseInt($("#selRoi").val());
   
    var masken = roiMaskArr[v][0];
    if(masken==1){
        $("#checkRoi").prop("checked",true);
        document.IPCamera.IPCSetMDModeEx(0, true);
        document.IPCamera.IPCSetMDAreaRectEx(parseInt(0),
                                                roiMaskArr[v][1] * wndWidth / veWidth,
                                                roiMaskArr[v][2]  * wndHeight / veHeight,
                                                roiMaskArr[v][3]  * wndWidth / veWidth,
                                                roiMaskArr[v][4]  * wndHeight / veHeight);

        document.IPCamera.IPCShowMDAreaEx(0, true);
        document.IPCamera.IPCSetMDAreaTitleEx(0, parseInt(v)+1);
    }
    else {     
        $("#checkRoi").prop("checked",false);
        document.IPCamera.IPCShowMDAreaEx(0, false);
    }
}

//保存视频遮挡
function SaveRoi(){
    var v =  parseInt($("#selRoi").val());
    var masken = $("#checkRoi").is(":checked")?1:0;

    var winPosStr = document.IPCamera.IPCGetVideoWndRect(0);

    var l = GetRtspKeyStr(winPosStr, "left");
    var t = GetRtspKeyStr(winPosStr, "top");
    var r = GetRtspKeyStr(winPosStr, "right");
    var b = GetRtspKeyStr(winPosStr, "bottom");

    l = parseInt(l*veWidth/wndWidth);
    t = parseInt(t*veHeight/wndHeight);
    r = parseInt(r*veWidth/wndWidth);
    b = parseInt(b*veHeight/wndHeight);

    roiMaskArr[v][0] = masken;
    roiMaskArr[v][1] = l;
    roiMaskArr[v][2] = t;
    roiMaskArr[v][3] = r;
    roiMaskArr[v][4] = b;

    var jcpstr = "roicfg -act set -id "+v+" -enable "+masken+" -left "+l+" -top "+t+" -right "+r+" -bottom "+b;

    GetJCPList({cmd: jcpstr});

    parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
    window.focus();
}

//删除
function DelRoi(){
    var v =  parseInt($("#selRoi").val());
    document.IPCamera.IPCSetMDModeEx(0, true);
    document.IPCamera.IPCShowMDAreaEx(0, false);
    roiMaskArr[v][0] = 0;
    roiMaskArr[v][1] = 0;
    roiMaskArr[v][2] = 0;
    roiMaskArr[v][3] = 0;
    roiMaskArr[v][4] = 0;
    var jcpstr = "roicfg -act set -id "+v+" -enable 0  -left 0 -top 0 -right 0 -bottom 0";
    GetJCP({cmd: jcpstr,ParseJCP: function(jcpstr){
        if(jcpstr=="Error"){
            parent.paramFailTip(IDC_MSGBOX_DELETEFAIL);
            window.focus();
        }else{
            $("#checkRoi").prop("checked",false);
            parent.paramSaveTip(IDC_MSGBOX_DELETEOK);
            window.focus();
        }
    }});
}