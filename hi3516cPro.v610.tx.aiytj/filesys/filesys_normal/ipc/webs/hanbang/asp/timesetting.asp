<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />

<link href="/css/base.css" type="text/css" rel="stylesheet"/>
<link href="/css/form.css" type="text/css" rel="stylesheet"/>
<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/js/timesetting.js"></script>
</head>
<body>
    <div class="left">
        <table border=0 cellPadding=3 cellSpacing=1 class="mainTable">
             <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_TIMESETTING)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td width="35%" class="caption"><script>dwn(IDC_TIMESETTING_TIMEZONE)</script></td>
                <td align="left" width="65%">
                  <select id="TimeZone"  style="width:400px;font-size:10pt;">
                          <option value="0">(GMT-12:00)<script>dwn(IDC_NTP_GMT_1200_0);</script></option>
                          <option value="1">(GMT-11:00)<script>dwn(IDC_NTP_GMT_1100_0);</script></option>
                          <option value="2">(GMT-10:00)<script>dwn(IDC_NTP_GMT_1000_0);</script></option>
                          <option value="3">(GMT-09:00)<script>dwn(IDC_NTP_GMT_0900_0);</script></option>
                          <option value="4">(GMT-08:00)<script>dwn(IDC_NTP_GMT_0800_0);</script></option>
                          <option value="5">(GMT-07:00)<script>dwn(IDC_NTP_GMT_0700_0);</script></option>
                          <option value="6">(GMT-06:00)<script>dwn(IDC_NTP_GMT_0600_0);</script></option>
                          <option value="7">(GMT-05:00)<script>dwn(IDC_NTP_GMT_0500_0);</script></option>
                          <option value="8">(GMT-05:00)<script>dwn(IDC_NTP_GMT_0500_1);</script></option>
                          <option value="9">(GMT-04:00)<script>dwn(IDC_NTP_GMT_0400_0);</script></option>
                          <option value="10">(GMT-03:30)<script>dwn(IDC_NTP_GMT_0330_0);</script></option>
                          <option value="11">(GMT-03:00)<script>dwn(IDC_NTP_GMT_0300_0);</script></option>
                          <option value="12">(GMT-02:00)<script>dwn(IDC_NTP_GMT_0200_0);</script></option>
                          <option value="13">(GMT-01:00)<script>dwn(IDC_NTP_GMT_0100_0);</script></option>
                          <option value="14">(GMT)<script>dwn(IDC_NTP_GMT_0000_0);</script></option>
                          <option value="15">(GMT+01:00)<script>dwn(IDC_NTP_GMT__0100_0);</script></option>
                          <option value="16">(GMT+01:00)<script>dwn(IDC_NTP_GMT__0100_1);</script></option>
                          <option value="17">(GMT+01:00)<script>dwn(IDC_NTP_GMT__0100_2);</script></option>
                          <option value="18">(GMT+01:00)<script>dwn(IDC_NTP_GMT__0100_3);</script></option>
                          <option value="19">(GMT+02:00)<script>dwn(IDC_NTP_GMT__0200_0);</script></option>
                          <option value="20">(GMT+02:00)<script>dwn(IDC_NTP_GMT__0200_1);</script></option>
                          <option value="21">(GMT+03:00)<script>dwn(IDC_NTP_GMT__0300_0);</script></option>
                          <option value="22">(GMT+03:30)<script>dwn(IDC_NTP_GMT__0330_0);</script></option>
                          <option value="23">(GMT+04:00)<script>dwn(IDC_NTP_GMT__0400_0);</script></option>
                          <option value="24">(GMT+04:30)<script>dwn(IDC_NTP_GMT__0430_0);</script></option>
                          <option value="25">(GMT+05:00)<script>dwn(IDC_NTP_GMT__0500_0);</script></option>
                          <option value="26">(GMT+05:30)<script>dwn(IDC_NTP_GMT__0530_0);</script></option>
                          <option value="27">(GMT+06:00)<script>dwn(IDC_NTP_GMT__0600_0);</script></option>
                          <option value="28">(GMT+07:00)<script>dwn(IDC_NTP_GMT__0700_0);</script></option>
                          <option value="29">(GMT+08:00)<script>dwn(IDC_NTP_GMT__0800_0);</script></option>
                          <option value="30">(GMT+09:00)<script>dwn(IDC_NTP_GMT__0900_0);</script></option>
                          <option value="31">(GMT+09:30)<script>dwn(IDC_NTP_GMT__0930_0);</script></option>
                          <option value="32">(GMT+10:00)<script>dwn(IDC_NTP_GMT__1000_0);</script></option>
                          <option value="33">(GMT+11:00)<script>dwn(IDC_NTP_GMT__1100_0);</script></option>
                          <option value="34">(GMT+12:00)<script>dwn(IDC_NTP_GMT__1200_0);</script></option> 
                      </select>
                
                    <button  class="BtnConfig" onclick="SaveTimezoneSetting();"><script>dwn(IDC_SAVE)</script></button>
                </td>
              </tr>
              <tr>
                <td class="caption"><script>dwn(IDC_TIME_DATE)</script></td>
                <td align="left">
                  <input type="text" id="nvsdate" style="width:150px;"  maxlength="10">
                </td>
              </tr>
               <tr>
                <td class="caption"><script>dwn(IDC_TIMESETTING_TIME)</script></td>
                <td align="left">
                    <input type="text" id="nvstime" style="width:150px;"  maxlength="8">
                    <input type="checkbox" name="chksych" id="chksych" onclick="sychtime();">
                    <label for='chksych'><script>dwn(IDC_TIME_WITHPC);//与PC保持同步</script></label>
                </td>
              </tr>
              <tr>
              <td></td>
                <td  align="left">
                    <button  class="BtnConfig" onclick="SaveTimeSetting();"><script>dwn(IDC_SAVE)</script></button>
                </td>
              </tr>
              <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_NTP_SERVER)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>

              <tr>
                <td class="caption">NTP</td>
                <td align="left">
                  <input type="checkbox" id="ntpserviceen" onclick="ntpEnable()"/>
                      <label for='ntpserviceen'><script>dwn(IDC_INTERNETTIME_TITLE);//自动与Internet时钟服务器同步</script></label>
                </td>
              </tr>
              <tr>
                <td class="caption"><script>dwn(IDC_SERVERADDR)</script></td>
                <td align="left">
                  <input type="text" id="ntpserveraddr"  style="width:250px;"  maxlength="63">
                </td>
              </tr>
              <tr>
                <td class="caption"><script>dwn(IDC_SERVERPORT)</script></td>
                <td align="left">
                  <input type="text" id="ntpserverport" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);"  style="width:150px;" maxlength="5">
                      <font >(1~65535)</font>
                </td>
              </tr>
              <tr>
                <td class="caption"><script>dwn(IDC_NTP_SYC_TIME)</script></td>
                <td align="left">
                   <input type="text" id="ntpsyctime" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);"  style="width:150px;" maxlength="5">
                      <font >(1~65535)</font>
                </td>
              </tr>
              <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td colspan="2"  align="left">
                    <button  class="BtnConfig" onclick="SaveNtpSetting();"><script>dwn(IDC_SAVE)</script></button>
                </td>
              </tr>
            </table>
    </div>
</body>
</html>