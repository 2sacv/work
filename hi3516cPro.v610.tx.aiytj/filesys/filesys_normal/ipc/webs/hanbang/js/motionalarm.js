$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
        resize();
        showVideo();
        $(".sTime").selectTime();
        _init_click()
        _init_slider()
    }
})


var sliderSpeed = null;
var wndWidth = 350; //视频宽度
var wndHeight = 250; //视频高度
var veWidth=1920, veHeight=1080; //用插件获取到的尺寸
function showVideo(){
    if(g_is_msie){
      $("#objects").html('<object id="IPCamera" name="IPCamera" CLASSID="CLSID:2319F6E6-ABD3-4b68-BADF-05D8796FA072" width="'+wndWidth+'" height="'+wndHeight+'"></object>');
    }else{
      $("#objects").html('<object id="IPCamera" name="IPCamera" type="application/npipcam" width="'+wndWidth+'" height="'+wndHeight+'"></object>');
    }
    document.IPCamera.IPCSetWindowMode(1);
    var type = GetCookieByKey("ljtypes");
    type = type== -1?1:type;
    document.IPCamera.IPCStartPreviewEx(0, GetCookieByKey("url"), 0, parseInt(type), GetCookieByKey("rtspport"),GetCookieByKey("user"), GetCookieByKey("passwd"), GetCookieByKey("stream"),"V2.00");
}


var g_desc_info = "";
IPCWndInit = function()
{
    GetJCP({cmd: "mdmbcfg -act list",ParseJCP: function(info){
        if(info!='Error'){
            $("#m_slider_span").html(info.thresh)
            sliderSpeed.wsetValue(info.thresh);
            $("#motion_time_protection").selectTime('setData',info.timestrategy)
            if(info.enable == '1'){
              $("#motion_enb").prop("checked",true)
            }
            g_desc_info = info.mbdesc;
            if(false == document.IPCamera.IPCSetMDMode(0, true)){ return; }
            window.setTimeout(function(){
                IPCamera.IPCSetMDInfo(0, 1, g_desc_info);
            },200);
        }
    }});
    
}

var empty = "0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,";

_init_click = function(){
  $("#m_select").click(function(){ 
      IPCamera.IPCSetMDInfo(0, 0,"")
  })

  $("#m_setting").click(function(){
    var mbdesc = IPCamera.IPCGetMDInfo(0);
    IPCamera.IPCSetMDInfo(0, 1, mbdesc); 
    var enb = $("#motion_enb").is(":checked") == true ? 1 : 0
    var str = "mdmbcfg -act set -timestrategy " + $("#motion_time_protection").selectTime('getData');
    str += " -mbdesc " + IPCamera.IPCGetMDInfo(0) + " -enable " + enb + " -thresh " + $("#m_slider_span").html();
 
    GetJCP({cmd:str , ParseJCP: function(result){
        if(result == "Error"){
          parent.paramFailTip(IDC_MSGBOX_SAVEFAIL);
        }else{
          parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
        }
        window.focus();
    }});
  })

  $("#m_delect").click(function(){
    document.IPCamera.IPCSetMDInfo(0, 0, empty)
    GetJCP({cmd:"mdmbcfg -act set -mbdesc " + empty, ParseJCP: function(result){
        if(result == "Error"){
          parent.paramFailTip(IDC_MSGBOX_DELETEFAIL);
        }else{
          parent.paramSaveTip(IDC_MSGBOX_DELETEOK);
        }
    }})
    window.focus();
  })
}

_init_slider = function(){
    sliderSpeed = new SliderModules({
      targetId: "m_slider",
      min: 1,
      max: 100
    }); 
    sliderSpeed.create();
    sliderSpeed.onchange = function () {
        $('#m_slider_span').text(sliderSpeed.getValue());
    };
}

$(window).resize(function(){
    resize();
});

function resize(){
    var $h = $(window).height();
    if($h < 700){
       $("body").css("height",700);
    }
}

function FireChannelEvent(msg,chn, str){
  if(msg == 11){
    IPCWndInit();
  }else if(msg == 12){
    handleMotionDetect(str);
  }
}

function handleMotionDetect(alarm_msg){
  if("MOTIONDETECT"==alarm_msg.split(";")[0].split("=")[1]){
    $("#mdTip").html(IDC_MOTIONDETECT_ALARM);
    window.setTimeout(function(){
      $("#mdTip").html('');
    },2000);
  }
}