$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
        resize();
        showVideo();
        $(".sTime").selectTime();
        _init_slider();
    }
})


var slider_env = null;
var slider_body = null;

var empty = "0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,";

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


IPCWndInit = function()
{

    document.IPCamera.IPCSetHDMode(0, true);
    GetJCP({cmd: "humandetectcfg -act list",ParseJCP: function(info){
        if(info!='Error'){
            $("#m_slider_span_env_sensitivity").html(info.thresh);
            slider_env.wsetValue(info.thresh);

            $("#m_slider_span_body_distance").html(info.humandistance);
            slider_body.wsetValue(info.humandistance);

            $("#people_monitor_time_protection").selectTime('setData',info.timestrategy);

            if(info.enable == '1'){
			  $("#people_monitor_enb").prop("checked",true);
			} else {
			  $("#people_monitor_enb").prop("checked",false);
			}

			if(info.screenenable == '1'){
			  $("#people_blink_enb").prop("checked",true);
			} else {
			  $("#people_blink_enb").prop("checked",false);
			}

			$("#select_people_scene_mode").val(info.outdoor);
			
			IPCamera.IPCSetHDInfo(0, info.mbdesc);

			var str = "humandetectcfg -act set ";
			str += " -drag 1 ";
			str += " -humandistance " +  info.humandistance;

			GetJCP({cmd:str , ParseJCP: function(result){}});

        }
    }});
    
}



clickDrawPeople = function() {
	document.IPCamera.IPCSetHDMode(0, true);
	document.IPCamera.IPCSetHDInfo(0, "");
}

clickDelPeople = function() {
	document.IPCamera.IPCSetHDInfo(0, "");
    GetJCP({cmd:"humandetectcfg -act set -drag 0 -mbdesc " + empty, ParseJCP: function(result){
		if(result == "Error"){
			parent.paramFailTip(IDC_DEL + IDC_FAIL);
		}else{
			parent.paramSaveTip(IDC_DEL + IDC_SUCCESS);
		}
    }})
    window.focus();
}

function clickSavePeople() {
	var mbdesc = document.IPCamera.IPCGetHDInfo(0);
	var enb = $("#people_monitor_enb").is(":checked") == true ? 1 : 0;
	var screenenable = $("#people_blink_enb").is(":checked") == true ? 1 : 0;
	var outdoor = $("#select_people_scene_mode").val();

	var str = "humandetectcfg -act set -timestrategy " + $("#people_monitor_time_protection").selectTime('getData');
	str += " -mbdesc " + mbdesc + " -enable " + enb + " -thresh " + $("#m_slider_span_env_sensitivity").html();
	str += " -screenenable " + screenenable;
	str += " -humandistance " +  + $("#m_slider_span_body_distance").html();
	str += " -drag 0";
	str += " -outdoor " + outdoor;

	GetJCP({cmd:str , ParseJCP: function(result){
		if(result == "Error"){
		  parent.paramFailTip(IDC_MSGBOX_SAVEFAIL);
	    }else{
		  parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
	    }
		window.focus();
	}});
}

_init_slider = function(){
    slider_env = new SliderModules({
      targetId: "m_slider_env_sensitivity",
      min: 1,
      max: 100
    }); 
    slider_env.create();
    slider_env.onchange = function () {
        $('#m_slider_span_env_sensitivity').text(slider_env.getValue());
    };

	
    slider_body = new SliderModules({
      targetId: "m_slider_body_distance",
      min: 1,
      max: 100
    }); 
    slider_body.create();
    slider_body.onchange = function () {
        $('#m_slider_span_body_distance').text(slider_body.getValue());
			
		var str = "humandetectcfg -act set ";
		str += " -drag 1 ";
		str += " -humandistance " +  slider_body.getValue();

		GetJCP({cmd:str , ParseJCP: function(result){}});
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
  if("HUMANDETECT"==alarm_msg.split(";")[0].split("=")[1]){
    $("#people_tip").html(IDC_HUMAN_DETECTION_ALARM);
    window.setTimeout(function(){
      $("#people_tip").html('');
    },2000);
  }
}
