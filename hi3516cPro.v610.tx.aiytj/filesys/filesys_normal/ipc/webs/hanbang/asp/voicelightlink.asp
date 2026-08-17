<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />

<link href="/css/base.css" type="text/css" rel="stylesheet"/>
<link href="/css/form.css" type="text/css" rel="stylesheet"/>
<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/jquery/jquery.timers.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/js/voicelightlink.js"></script>
<style type="text/css">
.grayTable2{margin-left:0px;margin-top:5px;color:black;}
.grayTable2 tr{height:25px;}
.grayTable2 td{border: 1px solid #666;}
.select{width:120px !important;}
</style>
</head>
<body>
    <div class="left">
    <table style='width: 600px;margin-left: 0px;'> 
          <tr><td  valign="middle" height="20px" align="left"><b>
           <script>dwn(IDC_VOICE_LIGHT_LINK)</script>
         </b></td></tr>
         <tr><td class="hline" colspan="2"></td></tr>
           <tr>
             <td align="left">
                <script>dwn(IDC_LIGHT_ALARM_SWITCH+":");//灯光报警开关</script>
             </td>
             <td height="40" width="350" align="left">
                <input type="radio" name="LIGHT_ALARM_SWITCH" value="1" checked >
                <script>dwn(IDC_GEN_SWITCH_OPEN);</script>
                <input type="radio" name="LIGHT_ALARM_SWITCH" value="0">
                <script>dwn(IDC_GEN_SWITCH_CLOSE);</script>
             </td>
             <td align="left"></td>
           </tr>
           <tr>
             <td align="left">
                <script>dwn(IDC_LIGHT_ALARM_TIME+":");//灯光报警布防时间</script>
             </td>
             <td height="40" width="350" align="left">
                <select id="place" class="sysinput select" onchange="changePlace()">
                    <option value='0'><script>dwn(IDC_NIGHT_ARM);</script></option>
                    <option value='1'><script>dwn(IDC_DAY_ARM);</script></option>
                    <option value='2'><script>dwn(IDC_FULL_ARM);</script></option>
                    <option value='3'><script>dwn(IDC_CUSTOM_ARM);</script></option>
                </select>
             </td>
             <td align="left"></td>
           </tr>
            <tr id="td_sound_custom_time_key">
               <td align="left">
                         <script>
                             dwn(IDC_CUSTOM_TIME);//自定义时间
                         </script>
                     </td>
                     <td height="25" width="350" align="left" id="td_sound_custom_time_value"></td>
                     <td height="25"  align="left" width="30"></td>
             </tr>
          <tr>
             <td align="left">
                  <script>dwn(IDC_LIGHT_ALARM_LENGTH);//灯光报警亮灯时长</script>
              </td>
             <td align="left">
                  <input type="text" id="custom_time" style="width:120px;" maxlength="2" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);"   onchange="changeCustomTime()"  class="sysinput2">
                  <font color="white"><script>dwn(IDC_PTZ_second);</script>(10~60)</font>
              </td>
             <td align="left"></td>
          </tr>
          
           <tr>
             <td align="left">
                  <button style="width:85px;margin-left:0px;margin-top:0px;" class="btn btn-inverse btn-black index_btn"  onclick="SaveVoiceLightLink();"><script>dwn(IDC_SAVE);</script></button>
              </td>
           </tr> 
      </table>
      <div style="margin-top:15px;border:1px solid #666"></div>
      <tr><td class="hline" colspan="2"></td></tr>
      <table style='width: 600px;margin-left: 0px;' id="td_voice_alarm"> 
           <tr >
             <td align="left">
                <script>dwn(IDC_VOICE_ALARM_SWITCH+":");//声音报警开关</script>
             </td>
             <td align="left">
                <input type="radio" name="VOICE_ALARM_SWITCH" value="1" >
                <script>dwn(IDC_GEN_SWITCH_OPEN);</script>
                <input type="radio" name="VOICE_ALARM_SWITCH" value="0">
                <script>dwn(IDC_GEN_SWITCH_CLOSE);</script>
             </td>
             <td align="left"></td>
           </tr>
           <tr >
             <td align="left">
                <script>dwn(IDC_LINKAGE_AUDIO_SELECT+":");//报警声音类型</script>
             </td>
             <td height="40" width="350" align="left" colspan="3">
                <input type="radio" name="ALARM_VOICE" value="0" >
                <script>dwn(IDC_LINKAGE_AUDIO_DEFAULT);//报警声音</script>
                <input type="radio" name="ALARM_VOICE" value="1">
                <script>dwn(IDC_LINKAGE_AUDIO_DOG);//狗叫声</script>
                <input type="radio" name="ALARM_VOICE" value="2">
                <script>dwn(IDC_LINKAGE_AUDIO_CUSTOM);//自定义声音</script>
             </td>
             <td align="left"></td>
           </tr>
           <tr >
             <td align="left">
                <script>dwn(IDC_VOICE_ALARM_TIME+":");//声音报警布防时间</script>
             </td>
             <td height="40" width="350" align="left">
                <select id="voice_place" class="sysinput select" onchange="changeVoicePlace()">
                    <option value='0'><script>dwn(IDC_NIGHT_ARM);</script></option>
                    <option value='1'><script>dwn(IDC_DAY_ARM);</script></option>
                    <option value='2'><script>dwn(IDC_FULL_ARM);</script></option>
                    <option value='3'><script>dwn(IDC_CUSTOM_ARM);</script></option>
                </select>
             </td>
             <td align="left"></td>
            </tr>
            <tr id="td_voice_custom_time_key">
                 <td align="left">
                         <script>
                             dwn(IDC_CUSTOM_TIME);//自定义时间
                         </script>
                     </td>
                     <td height="25" width="350" align="left" id="td_voice_custom_time_value"></td>
                     <td height="25"  align="left" width="30"></td>
             </tr>

          <tr>
             <td align="left">
                  <script>dwn(IDC_VOICE_ALARM_COUNT);//声音报警次数</script>
              </td>
              <td height="40" width="350" align="left">
                <select id="voice_times" name="voice_times" class="sysinput" style="width:80px;">
                      <option value="1">1</option>
                      <option value="2">2</option>
                      <option value="3">3</option>
                      <option value="4">4</option>    
                      <option value="5">5</option>
                      <option value="6">6</option>
                      <option value="7">7</option>
                      <option value="8">8</option>
                      <option value="9">9</option>
                      <option value="10">10</option>
                  </select>
              </td>
             <td align="left"></td>
          </tr>
          
           <tr>
             <td align="left">
                  <button style="width:85px;margin-left:0px;margin-top:0px;" class="btn btn-inverse btn-black index_btn"  onclick="SaveVoiceAlarm();"><script>dwn(IDC_SAVE);</script></button>
              </td>
           </tr>
      </table>
      <table style='width: 600px;margin-left: 0px;'>
        <tr>
          <td height="40px;">&nbsp;</td>
        </tr>
        <tr>
             <td align="left">
              <div><span style='margin-top: 50px;'><script>dwn(IDC_SIMULATE_ALARM);//模拟报警</script></span></div>
              <div style="margin-top:15px;border:1px solid #666"></div>
          </td>
       </tr> 
        <tr>
			  <td height="40" class="sysinput" align="left" id="tdSimulate"></td>
       </tr> 
      </table>
    </div>
</body>
</html>
