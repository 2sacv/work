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
<script type="text/javascript" src="/js/guobiao.js"></script>
</head>
<body>
    <div class="left">
            <table border=0 cellPadding=3 cellSpacing=1 class="mainTable">
              <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               SIP
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                  <td  width="35%" class="caption">
                   <span><script>dwn(IDC_P2P_SWITCH)</script></span>
                  </td>
                  <td align="left" width="65%">
                    <input type="radio" name="guobiao_switch" value="1">
                    <script>dwn(IDC_GEN_SWITCH_OPEN);</script>
                    <input type="radio" name="guobiao_switch" value="0">
                    <script>dwn(IDC_GEN_SWITCH_CLOSE);</script>
                </td>
              </tr>
              <tr>
                  <td align="left">
                    <span><script>dwn(IDC_STATUS_INFO)</script></span>
                </td>
                  <td align="left">
                    <label id="guobiao_status"></label>
                </td>
              </tr>
              <tr>
              		<td width="35%" class="caption">
              				<span><script>dwn(IDC_STREAM_TYPE)//码流类型</script></span>
              		</td>
              		<td align="left" width="65%">
              				<select id="videochannel" style="width:300px;">
              						<option value="0"> <script>dwn(IDC_STREAM_MASTER);</script> </option>
              						<option value="1"> <script>dwn(IDC_STREAM_SLAVE);</script> </option>
              				</select>
              		</td>
              </tr>
              <tr>
                  <td width="35%" class="caption">
                   <span><script>dwn(IDC_SIP_ADMINISTRATIVE_AREAS)//SIP服务器域</script></span>
                  </td>
                  <td align="left" width="65%">
                      <input type="text" id="dev_area"   maxlength="31">
                  </td>
              </tr>
              <tr>
                  <td width="35%" class="caption">
                  <span><script>dwn(IDC_SIP_SERVERIP)//SIP服务器IP</script></span>
                  </td>
                  <td align="left" width="65%">
                      <input type="text" id="SipServerIp"   maxlength="31">
                  </td>
              </tr>
              <tr>
                  <td width="35%" class="caption">
                  <span><script>dwn(IDC_SIP_SELECTED_PORT)//SIP服务器端口</script></span>
                  </td>
                  <td align="left" width="65%">
                      <input type="text" id="SipPorts"   onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" maxlength="5">
                  </td>
              </tr>
              <tr>
                  <td width="35%" class="caption">
                  <span><script>dwn(IDC_SIP_SERVER_ID)//SIP服务器ID</script></span>
                  </td>
                  <td align="left" width="65%">
                      <input type="text" id="SipServerId"   maxlength="31">
                  </td>
              </tr>
              <tr>
                  <td width="35%" class="caption">
                  <span><script>dwn(IDC_SIP_USER)//SIP 用户认证ID</script></span>
                  </td>
                  <td align="left" width="65%">
                      <input type="text" id="SipDev"   maxlength="31">
                  </td>
              </tr>
              <tr>
                  <td width="35%" class="caption">
                  <span><script>dwn(IDC_SIP_PASSWORD)//SIP 用户密码</script></span>
                  </td>
                  <td align="left" width="65%">
                      <input type="password" maxlength="31" id="SipPwd"    >
                  </td>
              </tr>
              <tr>
                  <td width="35%" class="caption">
                  <span><script>dwn(IDC_SYSTEM_NAMES)//设备名称</script></span>
                  </td>
                  <td align="left" width="65%">
                      <input type="text" id="SipSystemName"   maxlength="63">
                  </td>
              </tr>
              <tr>
                  <td width="35%" class="caption">
                  <span><script>dwn(IDC_ALARM_ID)//报警ID</script></span>
                  </td>
                  <td align="left" width="65%">
                     <input type="text" id="SipAlarms"   maxlength="31">
                  </td>
              </tr>
              <tr>
                  <td width="35%" class="caption">
                  <span><script>dwn(IDC_TIME_INTERVALS)//注册间隔</script></span>
                  </td>
                  <td align="left" width="65%">
                      <input type="text" id="SipTimeInter"    onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);"  maxlength="5">
                  </td>
              </tr>
              <tr>
                  <td width="35%" class="caption">
                  <span><script>dwn(IDC_HBEATSVRPERIOD)//心跳周期</script></span>
                  </td>
                  <td align="left" width="65%">
                      <input type="text" id="SipTimeHbda"     onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);"  maxlength="5">
                  </td>
              </tr>
              <tr>
                <td align="left">
                  <span><script>dwn(IDC_LOCAL_SELECTED_PORT)//本地端口</script></span>
                </td>
                <td align="left">
                  <input type="text" id="LocalPorts" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" maxlength="5">
                </td>
              </tr>
              <tr>
                  <td width="35%" class="caption">
                   <script>dwn(IDC_LOCATION_INFORMATION)//位置信息</script>
                  </td>
                  <td align="left" width="65%">
                      <input type="text"  maxlength="31" id="GbLocation"   >
                  </td>
              </tr>
              <tr>
                  <td width="35%" class="caption">
                   <span><script>dwn(IDC_LONGITUDE)//经度</script></span>
                  </td>
                  <td align="left" width="65%">
                      <input type="text"  maxlength="20" id="GbLongitude"  >
                  </td>
              </tr>
              <tr>
                  <td width="35%" class="caption">
                   <span><script>dwn(IDC_LATITUDE)//纬度</script></span>
                  </td>
                  <td align="left" width="65%">
                      <input type="text" maxlength="20" id="GbLatitude"    >
                  </td>
              </tr>
              <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td align="left">
                    <button  class="BtnConfig" onclick="SaveNetGB();"><script>dwn(IDC_SAVE)</script></button>
                </td>
                <td  align="left">
                  <button class="BtnConfig" onclick="initGuoBiao()"><script>dwn(IDC_REFRESH)</script></button>
                </td>
              </tr>
            </table>  
    </div>
</body>
</html>
