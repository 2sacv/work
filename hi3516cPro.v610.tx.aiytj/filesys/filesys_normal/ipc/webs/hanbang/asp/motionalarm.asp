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
<script type="text/javascript" src="/js/motionalarm.js"></script>
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
               <script>dwn(IDC_MENU_MOTION)</script>
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
                 <div class='sTime' id='motion_time_protection'></div>
               </td>
             </tr>
             <tr style="height:20px;"><td></td></tr>
             <tr><td class="hline" ></td></tr>
             <tr>
               <td   align="left">
                  <input type='checkbox' id='motion_enb' style='float: left;'>
                  <label id='motion_enb_lab' for='motion_enb' style='float: left;'>
                    <script>dwn(IDC_MD_ENABLE)</script>
                  </label>
                  <label id="mdTip" style='color:red;float: left; margin-left: 25px;'></label>
               </td>
             </tr>
              <tr>
               <td   align="left">
                  <div style='float: left;'><span id='ms_span'>
                    <script>dwn(IDC_MSENSITIVITY)</script>
                  </span></div>
                  <div style='float: left;margin-left:20px;width: 360px;height:4px;' id='m_slider'></div>
                  <div style='margin-left: 20px;'><span id='m_slider_span' style="display:-moz-inline-box; display:inline-block;width:20px;margin-left: 10px;"></span></div>
               </td>
             </tr>
            <tr>
              <td   align="left">
                  <button  class="BtnConfig"  id="m_select"><script>dwn(IDC_SELECT_TABLE)</script></button>
                  <button  class="BtnConfig"  id='m_setting'><script>dwn(IDC_SAVE)</script></button>
                  <button  class="BtnConfig"  id='m_delect'><script>dwn(IDC_DEL)</script></button>
              </td>
            </tr>
            <tr>
              <td   align="left" id='prompt_tr'>
                  <span id='select_prompt'>
                    <script>dwn(IDC_SELECT_PROMPT)</script>
                  </span>
              </td>
            </tr>
        </table>
    </div>
</body>
</html>