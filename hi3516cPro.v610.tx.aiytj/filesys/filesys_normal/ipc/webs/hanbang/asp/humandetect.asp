<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />

<link href="/css/base.css" type="text/css" rel="stylesheet"/>
<link href="/css/form.css" type="text/css" rel="stylesheet"/>
<link href="/jquery/selectTime/jquery.selectTime.css" type="text/css" rel="stylesheet"/>
<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/jquery/selectTime/jquery.selectTime.js"></script>
<script type="text/javascript" src="/js/jslider.js"></script>
<script type="text/javascript" src="/js/humandetect.js"></script>
<script language="javascript" for="IPCamera" event="FireLogon()">
  IPCWndInit();
</script>
<script language="javascript" for="IPCamera" event="FireAlarm(channel_index, alarm_msg)">
  handleMotionDetect(alarm_msg);
</script>
</head>

<body style="overflow-y:auto;">
    <div class="left">
        <table border=0 cellPadding=0 cellSpacing=0 class="mainTable" style="width:650px;">
             <tr><td  valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_HUMAN_DETECTION_ALARM)</script>
             </b></td></tr>
             <tr><td class="hline" ></td></tr>
             <tr>
               <td   align="left">
                       <div id="objects" valign="middle"></div>
               </td>
             </tr>
             <tr><td  valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_TIME_PROTECTION)</script>
             </td></tr>
             <tr><td class="hline" ></td></tr>
             <tr>
               <td   align="left">
                 <div class='sTime' id='people_monitor_time_protection'></div>
               </td>
             </tr>
             <tr style="height:20px;"><td></td></tr>
             <tr><td class="hline" ></td></tr>
             <tr>
              <td align="left">
                <input type='checkbox' id='people_monitor_enb'>
                <label id='people_monitor_enb_lab' for='people_monitor_enb'><script>dwn(IDC_ENABLE_HUMAN_DETECTION);</script></label>
                <label id="people_tip" style='color:red;margin-left: 25px;'></label>
              </td>
            </tr>
            <tr>
              <td align="left">
                <input type='checkbox' id='people_blink_enb'>
                <label for='people_blink_enb'><script>dwn(IDC_HUMAN_MARK_ENABLE);</script></label>
              </td>
            </tr>
            <tr height='50'>
              <td align="left">
                <span><script>dwn(IDC_scene_mode);</script></span>
                <select id="select_people_scene_mode" class="sysinput2" style="width:150px;">
					<option value="0"><script>dwn(IDC_INDOOR_HUMAN);</script></option>
					<option value="1"><script>dwn(IDC_OUTDOOR_HUMAN);</script></option>
					<option value="2"><script>dwn(IDC_OUTDOOR_CAR);</script></option>
                </select>
              </td>
            </tr>
            <tr height='50' style="display:none;">
              <td align="left">
                <div style='float: left;'><span id='ms_span'><script>dwn(IDC_ENV_SENSITIVITY)</script></span></div>
                <div style='float: left;margin-left:20px;width: 360px;height:4px;' id='m_slider_env_sensitivity'></div>
                <div style='margin-left: 20px;'><span id='m_slider_span_env_sensitivity' style="display:-moz-inline-box; display:inline-block;width:20px;margin-left: 10px;"></span></div>
              </td>
            </tr>
            <tr height='50'>
              <td align="left">
                <div style='float: left;'><span id='ms_span'><script>dwn(IDC_BODY_DISTANCE)</script></span></div>
                <div style='float: left;margin-left:20px;width: 360px;height:4px;' id='m_slider_body_distance'></div>
                <div style='margin-left: 20px;'><span id='m_slider_span_body_distance' style="display:-moz-inline-box; display:inline-block;width:20px;margin-left: 10px;"></span></div>
              </td>
            </tr>
            <tr>
              <td  align="left">
                  <button  class="BtnConfig"  onclick="clickSavePeople();" ><script>dwn(IDC_SAVE);</script></button>
                  <button  class="BtnConfig"  onclick="clickDelPeople();"><script>dwn(IDC_DEL);</script></button>
              </td>
            </tr>
        </table>
    </div>
</body>
</html>


           
            