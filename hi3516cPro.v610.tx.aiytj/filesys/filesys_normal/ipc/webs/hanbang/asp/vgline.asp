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
<script type="text/javascript" src="/js/vgline.js"></script>
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
               <script>dwn(IDC_monitor_cross_alarm)</script>
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
                 <div class='sTime' id='cross_monitor_time_protection'></div>
               </td>
             </tr>
             <tr style="height:20px;"><td></td></tr>
             <tr><td class="hline" ></td></tr>

             <tr>
              <td align="left">
                <input type='checkbox' id='cross_monitor_enb'>
                <label id='cross_monitor_enb_lab' for='cross_monitor_enb'><script>dwn(IDC_ENABLE_CROSS);</script></label>
                <label id="cross_tip" style='color:red;margin-left: 25px;'></label>
              </td>
            </tr>
            <tr>
              <td align="left">
                <input type='checkbox' id='cross_blink_enb'>
                <label for='cross_blink_enb'><script>dwn(IDC_BLINK_ENABLE);</script></label>
              </td>
            </tr>
            <tr height='50'>
              <td align="left">
                <span><script>dwn(IDC_scene_mode);</script></span>
                  <select id="select_cross_scene_mode" class="sysinput2" style="width:150px;" >
                  <option value="1"><script>dwn(IDC_PTZ_indoor_mode);</script></option>
                  <option value="0"><script>dwn(IDC_PTZ_outdoor_mode);</script></option>
                  </select>

                  <span style="margin-left:10px"><script>dwn(IDC_direction);</script></span>
                  <select id="select_cross_direction" class="sysinput2" style="width:100px;" onchange="changeCrossDirection()">
                  <option value="0">A->B</option>
                  <option value="1">B->A</option>
                  <option value="2">A<->B</option>
                  </select>
              </td>
            </tr>
            <tr height='50'>
              <td align="left">
                <div style='float: left;'><span id='ms_span'>
                  <script>dwn(IDC_sensitivity_cross)</script>
                </span></div>
                <div style='float: left;margin-left:20px;width: 360px;height:4px;' id='m_slider_cross_monitor'></div>
                <div style='margin-left: 20px;'><span id='m_slider_span_cross_monitor' style="display:-moz-inline-box; display:inline-block;width:20px;margin-left: 10px;"></span></div>
              </td>
            </tr>
            <tr>
              <td   align="left">
                  <button  class="BtnConfig"  onclick="clickDrawLine();" ><script>dwn(IDC_draw_line);</script></button>
                  <button  class="BtnConfig"  onclick="clickSaveLine();"><script>dwn(IDC_SAVE);</script></button>
                  <button  class="BtnConfig"  onclick="clickDelLine();"><script>dwn(IDC_DEL);</script></button>
              </td>
            </tr>
        </table>
    </div>
</body>
</html>


           
            