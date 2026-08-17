<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta http-equiv="pragma" content="no-cache" />
<meta http-equiv="Cache-Control" content="no-cache, must-revalidate" />
<meta http-equiv="expires" content="Thu, 1 Jan 1970 00:00:01 GMT" />

<link href="/css/main.css" type="text/css" rel="stylesheet"/>
<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery-ui-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/js/jslider.js"></script>
<script type="text/javascript" src="/js/stream.js"></script>
<script type="text/javascript" src="/js/preview.js"></script>
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

<body>
      <div class="hd">
         <div style="float:left">
	        <img id="top_logo_image" src="/image/logo.png" href="#"/>
	     </div>
         <div style="float:right;margin-top:50px;margin-right:5px;font-size:14px;">
	        <span id="spanCurrTime"></span>
	     </div>
      </div>
      <div class="bd">
       		<div class="side">
       			<div style='background:url(/image/banner.png) repeat-x;height:52px;'  class="banner">
			        <span class="active" id="spanLiveview">
			        	<img src="/image/liveview.png"/></br>
			        	<label id="laLiveview"></label>
			        </span>
			        <span  id="spanPlayback">
			        	<img src="/image/playback.png"/></br>
			        	<label id="laPlayback"></label>
			        </span>
			        <span id="spanLog">
			            <img src="/image/log.png"/></br>
			            <label id="laLog"></label>
			        </span>
			        <span  id="spanSetting">
			            <img src="/image/setting.png"/></br>
			            <label id="laSetting"></label>
			        </span>
			        <span  id="spanExit">
			            <img src="/image/exit.png"/></br>
			            <label id="laExit"></label>
			        </span>
			      </div>

			      <div style="position:absolute;bottom:1px;">
			      	<div id="divColorTab" class="divTab">
			      			<table border=0 cellPadding=3 cellSpacing=1 style="width:100%;" id="tbColorParamSet">
			                    <tr  id="trBright">
			                        <td  align="left">
			                           <img src="/image/mainview/brightness.png" id="imgbrightness"/>
			                        </td>
			                        <td  align="left"  style="width:195px;">
			                            <div id="bright" style="width:195px;height:8px;">
			                                        </div>
			                        </td>
			                        <td align="left" id="tdBright">64</td>
			                    </tr>
			                    <tr  id="trContrast">
			                        <td align="left">
			                           <img src="/image/mainview/contrast.png" id="imgcontrast"/>
			                        </td>
			                        <td  align="left"  style="width:195px;">
			                            <div id="contrast" style="width:195px;height:8px;">
			                                        </div>
			                        </td>
			                        <td align="left" id="tdContrast">64</td>
			                    </tr>
			                    <tr >
			                        <td align="left">
			                           <img src="/image/mainview/saturation.png" id="imgsaturation"/>
			                        </td>
			                        <td  align="left" style="width:195px;">
			                            <div id="saturation" style="width:195px;height:8px;">
			                                        </div>
			                        </td>
			                        <td  align="left" id="tdSaturation">64</td>
			                    </tr>
			                    <tr >
			                        <td align="left">
			                           <img src="/image/mainview/sharpness.png" id="imgsharpness"/>
			                        </td>
			                        <td  align="left"  style="width:195px;">
			                            <div id="sharpness" style="width:195px;height:8px;">
			                                        </div>
			                        </td>
			                        <td  align="left" id="tdSharpness">64</td>
			                    </tr>
			                  </table>
			      	</div>
			      	<div align="center" class="ptz_tab" id="divColor">
			      		<label id="laColor"></label>
			      		<img src="/image/mainview/down.png" id="divColorImg"/>
			      	</div>
			      </div>
       		</div>
       		<div class="main">
       			<div class="video">
       				<div id="object" valign="middle" style='position:absolute;border: 0px solid black;height:100%;width:100%;background:black;'>
                    </div> 
       			</div>
       			<div class="icon-banner">
       			    <input type="radio" id="master" name="stream" style='margin-left: 10px;margin-top:5px;'  value='1'>
       			    <label id='master_span' for="master" style='margin-top:5px;'></label>
			        <input type="radio" id="slave" name="stream" style='margin-left: 10px;margin-top:5px;' value='0'>
			        <label id='slave_span' for="slave"  style='margin-top:5px;'></label>

       				<img src="../image/setting.png" style='margin:5px 15px 0 15px;cursor: pointer;' id='setting'>
		            <img src="../image/mainview/no-record.png" style='margin:5px 10px 0 0;cursor: pointer;' id='video'>
		            <img src="../image/mainview/screenshot.png" id='screen' style='margin:5px 10px 0 0;cursor: pointer;'>
		            <img src="../image/mainview/speak.png" id='speak' style='margin:5px 10px 0 0;cursor: pointer;display:none;'>
		            <img src="../image/mainview/volume_close.png" id='volume' style='margin:5px 10px 0 0;cursor: pointer;display:none;'>
		            <img src="../image/mainview/no-alarm.png" id='alarm' style='margin:5px 0px 0 0px;cursor: pointer;'>
		    
		            <img src="../image/mainview/line.png" style='margin:5px 0px 0 0;cursor: pointer;' href="#">
		            <img src="../image/mainview/fullscreen.png" style='margin: 5px 5px 0 0;cursor: pointer;' id='full'>
		            <img src="../image/mainview/area-full.png" style='margin:5px 10px 0 0;cursor: pointer;' id='zoom'>
		            <img src="../image/mainview/3d.png" style='margin:5px 5px 0 0;cursor: pointer;display:none;' id='positions' onclick="positions3d()">
		            <img src="../image/ptz/hide.png" id='ptzShow' style='margin:5px 5px 0 0px;cursor: pointer;display:none;' onclick="ptzShowOrHide()">

		             <img src="../image/mainview/line.png" style='margin:5px 0px 0 0;cursor: pointer;' href="#">
                    <img src="../image/mainview/full_window.png" style='margin: 5px 10px 0 0;cursor: pointer;' id='fullWindow'>
		            <img src="../image/mainview/original_scale_sel.png" style='margin:5px 10px 0 0;cursor: pointer;' id='originalScale'>
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


<div style="position:absolute;top:0;left:50%;height:25px;line-height:25px;color:white;font-size:14px;background:rgb(0,102,204);display:none;" id="paramSaveTip"></div>
</body>

</html>