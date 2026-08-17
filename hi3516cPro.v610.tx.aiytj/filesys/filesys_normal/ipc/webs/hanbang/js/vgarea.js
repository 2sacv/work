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


var slider_area = null;
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

    document.IPCamera.IPCSetVGMode(0, 2);
    GetJCP({cmd: "vgrect -act list",ParseJCP: function(info){
        if(info!='Error'){
            $("#m_slider_span_area_monitor").html(info.thresh)
            slider_area.wsetValue(info.thresh);
            $("#area_monitor_time_protection").selectTime('setData',info.timestrategy)
            if(info.enable == '1'){
              $("#area_monitor_enb").prop("checked",true);
            } else {
              $("#area_monitor_enb").prop("checked",false);
            }
            if(info.blink == '1'){
              $("#area_blink_enb").prop("checked",true);
            } else {
              $("#area_blink_enb").prop("checked",false);
            }
            
            $("#select_area_scene_mode").val(info.indoor);
            $("#select_area_direction").val(info.dir);

            if (info.x0 == 0 && info.y0 == 0 && info.x1 == 0 && info.y1 == 0) {
              
            } else {
              var msg = "x0=" + setOCXShowXPosition(info.x0) + ",y0=" + setOCXShowYPosition(info.y0) + ",";
              msg += "x1=" + setOCXShowXPosition(info.x1) + ",y1=" + setOCXShowYPosition(info.y1) + ",";
              msg += "x2=" + setOCXShowXPosition(info.x2) + ",y2=" + setOCXShowYPosition(info.y2) + ",";
              msg += "x3=" + setOCXShowXPosition(info.x3) + ",y3=" + setOCXShowYPosition(info.y3) + ",";
              document.IPCamera.IPCSetVGMode(0, 2);
              document.IPCamera.IPCSetVGInfo(0, msg);
            }
        }
    }});
    
}


function clickDrawArea() {
  window.focus();
  document.IPCamera.IPCSetVGMode(0, 2);
  document.IPCamera.IPCSetVGInfo(0, "");
}

function clickSaveArea() {
  window.focus();
   
  var enb = $("#area_monitor_enb").is(":checked") == true ? 1 : 0
  var blink_enb = $("#area_blink_enb").is(":checked") == true ? 1 : 0;

  var str = "vgrect -act set -timestrategy " + $("#area_monitor_time_protection").selectTime('getData');
  str += " -enable " + enb + " -thresh " + $("#m_slider_span_area_monitor").html();
  str += " -indoor " + $("#select_area_scene_mode").val();
  str += " -dir " + $("#select_area_direction").val();
  str += " -blink " + blink_enb;

  do
  {
    var _ocx_info = document.IPCamera.IPCGetVGInfo(0);

    if(_ocx_info == '' || _ocx_info.indexOf("x0") < 0) {
      str += " -x0 0";
      str += " -y0 0";
      str += " -x1 0";
      str += " -y1 0";
      str += " -x2 0";
      str += " -y2 0";
      str += " -x3 0";
      str += " -y3 0";
    } else {
      var _map = parse_jcp_content(_ocx_info, {spliter1: '=', spliter2: ','});

      str += " -x0 " + getOCXShowXPosition(_map.x0);
      str += " -y0 " + getOCXShowYPosition(_map.y0);
      str += " -x1 " + getOCXShowXPosition(_map.x1);
      str += " -y1 " + getOCXShowYPosition(_map.y1);
      str += " -x2 " + getOCXShowXPosition(_map.x2);
      str += " -y2 " + getOCXShowYPosition(_map.y2);
      str += " -x3 " + getOCXShowXPosition(_map.x3);
      str += " -y3 " + getOCXShowYPosition(_map.y3);
    }
  }
  while (0);

 
  GetJCP({cmd:str , ParseJCP: function(result){
      if(result == "Error"){
        parent.paramFailTip(IDC_MSGBOX_SAVEFAIL);
      }else{
        parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
      }
      window.focus();
  }});
}

function clickDelArea() {
  window.focus();
  document.IPCamera.IPCSetVGInfo(0, "");
  var str = "vgrect -act set";
  str += " -x0 0";
  str += " -y0 0";
  str += " -x1 0";
  str += " -y1 0";
  str += " -x2 0";
  str += " -y2 0";
  str += " -x3 0";
  str += " -y3 0";
  GetJCP({cmd:str, ParseJCP: function(result){
    if(result == "Error"){
      parent.paramFailTip(IDC_MSGBOX_DELETEFAIL);
    }else{
      parent.paramSaveTip(IDC_MSGBOX_DELETEOK);
    }
  }})
}


_init_slider = function(){
    slider_area = new SliderModules({
      targetId: "m_slider_area_monitor",
      min: 1,
      max: 100
    }); 
    slider_area.create();
    slider_area.onchange = function () {
        $('#m_slider_span_area_monitor').text(slider_area.getValue());
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
  if("VGRECTDETECT"==alarm_msg.split(";")[0].split("=")[1]){
    $("#area_tip").html(IDC_monitor_area_alarm);
    window.setTimeout(function(){
      $("#area_tip").html('');
    },2000);
  }
}

function getOCXShowXPosition(x) {
  var _x = parseInt(x * veWidth / wndWidth);
  return _x > 1920 ? 1920 : _x;
}

function setOCXShowXPosition(x) {
  return parseInt(x * wndWidth / veWidth);
}

function getOCXShowYPosition(y) {
  var _y = parseInt(y * veHeight / wndHeight);
  return _y > 1080 ? 1080 : _y;
}

function setOCXShowYPosition(y) {
  return parseInt(y * wndHeight / veHeight);
}


function changeCrossDirection() {
  var r = $("#select_cross_direction").val();

  var _ocx_info = document.IPCamera.IPCGetVGInfo(0);

  if (_ocx_info == '' || _ocx_info.indexOf("x0") < 0) {
    return;
  }

  var info = parse_jcp_content(_ocx_info, {spliter1: '=', spliter2: ','});

  var msg = "";

  if (r == 0 || r == 1) {
    if (old_dir == 2) {
      var x3 = parseInt(info.x0) + parseInt(info.x1) - parseInt(info.x2);
      var y3 = parseInt(info.y0) + parseInt(info.y1) - parseInt(info.y2);

      msg = "x0=" + info.x0 + ",y0=" + info.y0 + ",";
      msg += "x1=" + info.x1 + ",y1=" + info.y1 + ",";

      if(r == 0) {
        msg += "x2=" + info.x2 + ",y2=" + info.y2 + ",";
        msg += "x3=" + x3 + ",y3=" + y3 + ",";
      } else {
        msg += "x2=" + x3 + ",y2=" + y3 + ",";
        msg += "x3=" + info.x2 + ",y3=" + info.y2 + ",";
      }
    } else {
      msg = "x0=" + info.x0 + ",y0=" + info.y0 + ",";
      msg += "x1=" + info.x1 + ",y1=" + info.y1 + ",";
      msg += "x2=" + info.x3 + ",y2=" + info.y3 + ",";
      msg += "x3=" + info.x2 + ",y3=" + info.y2 + ",";
    }
  } else {
    msg = "x0=" + info.x0 + ",y0=" + info.y0 + ",";
    msg += "x1=" + info.x1 + ",y1=" + info.y1 + ",";
    msg += "x2=" + info.x2 + ",y2=" + info.y2 + ",";
    msg += "x3=" + info.x2 + ",y3=" + info.y2 + ",";
  }

  old_dir = r;

  
  document.IPCamera.IPCSetVGMode(0, 1);
  document.IPCamera.IPCSetVGInfo(0, "");
  document.IPCamera.IPCSetVGInfo(0, msg);

}


var POINT_2_POINT_INTERVAL = 10;

function Point(x, y) {
  this.x = x;
  this.y = y;
}

/**
 * 0:A->B,1:B->A, 2:A<->B
 */
function JudgeDirection(p0, p1, p2, p3) {
	var res = -1;
	do
	{
		if (4 > arguments.length)
		{
			break;
		}

		// 计算方向
		if (IsP2PNear(p2, p3, POINT_2_POINT_INTERVAL) == true)
		{
			res = 2;
			break;
		}

		var fDP0P1 = Point2Degree(p0, p1);
		var fDP0D0 = Point2Degree(p0, p2);
		if (fDP0D0 < fDP0P1)
		{
			res = 0;
		}
		else
		{
			res = 1;
		}

		if (fDP0D0- fDP0P1 > Math.PI / 2)
		{
			res = 0;
		}
		else if(fDP0P1-fDP0D0 > Math.PI / 2)
		{
			res = 1;
		}

	} while (0);

	return res;
}	

function IsP2PNear(pntA, pntB, iInterval) {
  var fP2P = Math.sqrt(Math.pow(pntA.x - pntB.x, 2) + Math.pow(pntA.y - pntB.y, 2));
  return fP2P <= iInterval ? true : false;
}

function Point2Degree(pntA, pntB) {
  if (0.00001 >= Math.abs(pntA.x - pntB.x))
  {// atan无穷大
    if (pntA.y < pntB.y)
    {
      return Math.PI / 2;
    }
    else
    {
      return Math.PI / 2 * 3;
    }
  }
  else if (0.00001 >= Math.abs(pntA.y - pntB.y))
  {// atan为零
    if (pntA.x < pntB.x)
    {
      return 0;
    }
    else
    {
      return Math.PI;
    }
  }
  
  var fDegree = Math.atan((pntB.y - pntA.y) * 1.0 / (pntB.x - pntA.x));
  if (pntA.x > pntB.x)
  {// 二三象限
    fDegree += Math.PI;
  }
  fDegree = Math.PI * 2 <= fDegree ? fDegree - Math.PI * 2 : (0 > fDegree ? fDegree + Math.PI * 2 : fDegree);

  return fDegree;
}
