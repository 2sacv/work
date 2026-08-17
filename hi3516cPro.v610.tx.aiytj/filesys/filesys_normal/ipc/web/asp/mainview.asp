<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta http-equiv="pragma" content="no-cache" />
<meta http-equiv="Cache-Control" content="no-cache, must-revalidate" />
<meta http-equiv="expires" content="0" />

<link href="/css/bootstrap.css" type="text/css" rel="stylesheet"/>
<link href="/css/jquery-ui-1.10.4.css" type="text/css" rel="stylesheet"/>
<link href="/css/index.css" type="text/css" rel="stylesheet"/>
<link href="/css/mainview.css" type="text/css" rel="stylesheet"/>

<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery-ui-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<link href="/jquery/messageTip/messagetip.css" type="text/css" rel="stylesheet"/>
<script type="text/javascript" src="/jquery/messageTip/messagetip.js"></script>
<script type="text/javascript" src="/jquery/bootstrap-min.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/js/stream.js"></script>
<script type="text/javascript" src="/js/mainview.js?g=1.0"></script>
<script language="javascript" for="IPCamera" event="FireAlarm(channel_index, alarm_msg)">
  triggerAlarm(alarm_msg);
</script>
<script language="javascript" for="IPCamera" event="FireZoomRectCanceled(channel_index)">
  triggerZoomRectCanceled(channel_index);
</script>
<script language="javascript" for="IPCamera" event="FireRecordDiskFull(channel_index)">
 stopRecordWhenDiskIsFull();
</script>
</head>

<body style="background: #211f20;" id='index'>
  <div style='background: #1f1f1e;'>
    <div style='height: 65px;background: #212121;margin-top: 0px;'> 
      <div style="margin-left: 5%;height:100%;background: #141414;width: 90%;" id="top_custom_div">
        <div id='top_logo_div'>
            <img id="top_logo_image" src="/image/logo.png" href="#" style="display:none;margin-top:-8px;"/>
            <span id="top_title_span" class="logo_title"></span>
        </div>
        <div id='top_menu_div' align="right">
          <button class='btn btn-inverse btn-black' id="setBtn" style="margin-right:0px;">
            <img src="/image/setting.png">
            <span id="set">
              <script type="text/javascript">dwn(IDC_PARAMETER_SET);</script>
            </span>
          </button>
          <button class='btn btn-inverse btn-black' id="playbackBtn"  style="margin-left:0px;margin-right:0px;padding-left:0px;" id="btn_play_back">
              <img src="/image/playback.png" style="margin-top:-3px;">
              <span id='playback'>
                <script type="text/javascript">dwn(IDC_PLAYBACK);</script>
              </span>
          </button>
          <button class='btn btn-inverse btn-black' id="exitBtn" style="margin-left:0px;">
              <img src="/image/logout.png" style="margin-top:-3px;">
              <span  id='exit'>
                <script type="text/javascript">dwn(IDC_EXIT);</script>
              </span>
          </button>
        </div>
      </div>
    </div>

    <div style='position: relative;margin-left: 5%;border: 1px solid #666666;width: 90%;' id="centerDiv" >
        <div style="position: absolute;width:250px;height:100%;border-left:1px solid #666666;background:#303030;display:none;" id="paramSetDiv">
          <div align="center"  class="action">
             <div class="speed">
                <span class="label">
                  <script type="text/javascript">dwn(IDC_PTZ_speed)</script>
                </span>
                <span class="slider" id="ptz_slider"></span>
                <span class="value" id="ptz_speed"></span>
            </div>
            <div>
               <table width="130" height="120" border="0" cellSpacing="0" cellPadding="0" class="ptzcontrolTb">
           <tr>
              <td>
                <image src="../image/ptz/left_up.png" onmouseover="this.src ='../image/ptz/left_up_sel.png'" onmouseout="this.src ='../image/ptz/left_up.png'" onmousedown="_doPtz(7);" onmouseup="_direction_stop();"/>
              </td>
              <td>
                <image src="../image/ptz/up.png" onmouseover="this.src ='../image/ptz/up_sel.png'" onmouseout="this.src ='../image/ptz/up.png'" onmousedown="_doPtz(1,'data2');" onmouseup="_direction_stop();"/>
              </td>
              <td>
                <image src="../image/ptz/right_up.png" onmouseover="this.src ='../image/ptz/right_up_sel.png'" onmouseout="this.src ='../image/ptz/right_up.png'" onmousedown="_doPtz(5);" onmouseup="_direction_stop();"/>
              </td>
            </tr>
            <tr>
              <td>
                <image src="../image/ptz/left.png" onmouseover="this.src ='../image/ptz/left_sel.png'" onmouseout="this.src ='../image/ptz/left.png'" onmousedown="_doPtz(3,'data1');" onmouseup="_direction_stop();"/>
              </td>
              <td>
                <image src="../image/ptz/center.png" onmouseover="this.src ='../image/ptz/center_sel.png'" onmouseout="this.src ='../image/ptz/center.png'" onmousedown="_auto_direction();"/>
              </td>
              <td>
                <image src="../image/ptz/right.png" onmouseover="this.src ='../image/ptz/right_sel.png'" onmouseout="this.src ='../image/ptz/right.png'" onmousedown="_doPtz(4,'data1');" onmouseup="_direction_stop();"/>
              </td>
            </tr>
            <tr>
              <td>
                <image src="../image/ptz/left_down.png" onmouseover="this.src ='../image/ptz/left_down_sel.png'" onmouseout="this.src ='../image/ptz/left_down.png'" onmousedown="_doPtz(8);" onmouseup="_direction_stop();"/>
              </td>
              <td>
                <image src="../image/ptz/down.png" onmouseover="this.src ='../image/ptz/down_sel.png'" onmouseout="this.src ='../image/ptz/down.png'" onmousedown="_doPtz(2,'data2');" onmouseup="_direction_stop();"/>
              </td>
              <td>
                <image src="../image/ptz/right_down.png" onmouseover="this.src ='../image/ptz/right_down_sel.png'" onmouseout="this.src ='../image/ptz/right_down.png'" onmousedown="_doPtz(6);" onmouseup="_direction_stop();"/>
              </td>
            </tr>
          </table>
            </div>
             <div class="group_line">
              <button class="wiper ptz_button" data-linaction="far">
                <script type="text/javascript">dwn(IDC_PTZ_far)</script>
              </button>
              <button class="wiper ptz_button" data-linaction="telephoto">
                <script type="text/javascript">dwn(IDC_PTZ_telephoto)</script>
              </button>
              </div>
             <div class="group_line">
               <button class="wiper ptz_button" data-linaction="nearly">
                <script type="text/javascript">dwn(IDC_PTZ_nearly)</script>
              </button>
              <button class="wiper ptz_button" data-linaction="nearlyburnt">
                <script type="text/javascript">dwn(IDC_PTZ_nearlyburnt)</script>
              </button>
            </div>
            <div class="group_line" style="margin-bottom:8px;">
             
              <button class="wiper ptz_button"  data-linaction="aperture_on">
                <script type="text/javascript">dwn(IDC_PTZ_Iris_on)</script>
              </button>
              <button class="wiper ptz_button" data-linaction="wiper_on">
                <script type="text/javascript">dwn(IDC_PTZ_wiper_on)</script>
              </button>
              </div>
             <div class="group_line">
              <button class="wiper ptz_button"  data-linaction="aperture_off">
                <script type="text/javascript">dwn(IDC_PTZ_Iris_off)</script>
              </button>
              <button class="wiper ptz_button" data-linaction="wiper_off">
                <script type="text/javascript">dwn(IDC_PTZ_wiper_off)</script>
              </button>
            </div>
            
              <div style="margin-left:22px;margin-top:20px;" >
                 <span  style="width:60px;color:white;">
                           <script type="text/javascript">dwn(IDC_PTZ_preset+IDC_PTZ_colon)</script>
                           </span>
                            <input id="preset_call_value" type="text" maxlength="3" style="width:60px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" />
              
              </div>
              
              <div align="center"  style="margin-top:5px;">
                <button id="preset_button_set" class="ptz_button" style="width:50px;height:25px;">
                    <script type="text/javascript">dwn(IDC_PTZ_set)</script>
                </button>                 
                <button id="preset_button_call" class="ptz_button"  style="width:50px;height:25px;">
                    <script type="text/javascript">dwn(IDC_PTZ_call)</script>
                  </button>
                <button id="preset_button_del" class="ptz_button"  style="width:50px;height:25px;">
                    <script type="text/javascript">dwn(IDC_PTZ_clear)</script>
                  </button>
              </div>
          </div>
        </div>
        <div id="object" valign="middle" style='position: absolute;border: 0px solid black;height:100%;'>
        </div> 
    </div>

    <script type="text/javascript">
        $("#centerDiv").css("height",$(window).height()-105);
    </script>
    <div id="bottom_img">
        <div id='stream_rdo' style="width:40%;float:left">
          <input type="radio" id="master" name="stream" style='margin-left: 10px;'  value='1'><label id='master_span' for="master"></label>
          <input type="radio" id="slave" name="stream" style='margin-left: 10px;' value='0'><label id='slave_span' for="slave"></label>
        </div>
        <div style="width:60%;float:right" align="right">
            <img src="../image/mainview/playsetting.png" style='margin:5px 15px 0 15px;cursor: pointer;' id='setting'>
            <img src="../image/mainview/no-record.png" style='margin:5px 10px 0 0;cursor: pointer;' id='video'>
            <img src="../image/mainview/screenshot.png" id='screen' style='margin:5px 10px 0 0;cursor: pointer;'>
            <img src="../image/mainview/speak.png" id='speak' style='margin:5px 10px 0 0;cursor: pointer;display:none;'>
            <img src="../image/mainview/volume_close.png" id='volume' style='margin:5px 10px 0 0;cursor: pointer;display:none;'>
            <img src="../image/mainview/no-alarm.png" id='alarm' style='margin:5px 5px 0 0px;cursor: pointer;'>
    
            <img src="../image/mainview/line.png" style='margin:5px 5px 0 0;cursor: pointer;' href="#">
            <img src="../image/mainview/area-full.png" style='margin:5px 10px 0 0;cursor: pointer;' id='zoom'>
            <img src="../image/mainview/fullscreen.png" style='margin: 5px 10px 0 0;cursor: pointer;' id='full'>
            <img src="../image/mainview/3d.png" style='margin:5px 5px 0 0;cursor: pointer;display:none;' id='positions' onclick="positions3d()">
            <img src="../image/ptz/hide.png" id='ptzShow' style='margin:5px 10px 0 0px;cursor: pointer;display:none;' onclick="ptzShowOrHide()">

      </div>
    </div>
  </div>
  <div id="playSetting" style="position:absolute;width:260px;height:210px;border:1px solid #2C2C2C;background-color:#2C2C2C;display:none;">
    <iframe id="playSettingFrame" fram
    eborder="0" width="100%" height="100%"  scrolling="no" src="playsetting.asp" marginwidth="0" marginheight="0">
    </iframe> 
   </div>

  <div id="containmsg" style="position:absolute;width:350px;height:210px;border:1px solid #2C2C2C;background-color:#2C2C2C;display:none;">
    <iframe id="containmsgframe" frameborder="0" width="100%" height="100%"  scrolling="no" src="alarmmsg.asp" marginwidth="0" marginheight="0">
    </iframe>
  </div>
</body>

</html>
