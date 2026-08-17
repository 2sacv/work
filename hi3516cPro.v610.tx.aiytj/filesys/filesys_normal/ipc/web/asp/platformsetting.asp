<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta http-equiv="pragma" content="no-cache" />
<meta http-equiv="Cache-Control" content="no-cache, must-revalidate" />
<meta http-equiv="expires" content="Thu, 1 Jan 1970 00:00:01 GMT" />

<link href="/css/bootstrap.css" type="text/css" rel="stylesheet"/>
<link href="/css/jquery-ui-1.10.4.css" type="text/css" rel="stylesheet"/>
<link href="/css/jquery.ipaddress.css" type="text/css" rel="stylesheet"/>
<link href="/css/index.css" type="text/css" rel="stylesheet"/>
<link href="/css/public_css.css" type="text/css" rel="stylesheet"/>

<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery-ui-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/jquery/jquery.caret.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/jquery/jquery.ipaddress.js"></script>
<script type="text/javascript" src="/js/platformsetting.js"></script>

<style type="text/css">
  span { display:-moz-inline-box; display:inline-block; width:150px;text-align:right;}
  .table_gb tr{height:38px;}
</style>
</head>

<body style="background: #2C2C2C;width:99%;height:100%">
    <div style='background: #3C3D3D;'>
      <div id="tabs" class='tabs ui-tabs ui-widget ui-widget-content ui-corner-all' style='width: 100%;'>
        <ul>
          <li id="liCloudplatform" style="display:none;"><a href="#tabs-7" id="tabCloudplatform"><script>dwn(IDC_PLATFORM)</script></a></li>
          <li id="liGuobiao" style="display:none;"><a href="#tabs-1" id="tabGuobiao"><script>dwn(IDC_GUOB)</script></a></li>
        </ul>

        <div id="tabs-1">
            <table width="100%" style="margin-top:5px;" class="table_gb">
              <tr>
                <td  align="left" colspan="3">
                  <font style="font-size:16px;">SIP</font>
                  <div style="margin:5px 5px 5px 0px;border:1px solid #666"></div>
                </td>
              </tr>

               <tr>
                  <td align="left">
                   <span><script>dwn(IDC_P2P_SWITCH)</script></span>
                    <input type="radio" name="guobiao_switch" value="1">
                    <script>dwn(IDC_GEN_SWITCH_OPEN);</script>
                    <input type="radio" name="guobiao_switch" value="0">
                    <script>dwn(IDC_GEN_SWITCH_CLOSE);</script>
                  </td>
                  <td align="left">
                    <span><script>dwn(IDC_STATUS_INFO)</script></span>
                    <label id="guobiao_status"></label>
                </td>
                <td align="left">
                    <span><script>dwn(IDC_GB_VERSION)</script></span>
                    <label id="guobiao_version"></label>
                </td>
              </tr>
              <tr>
                <td align="left">
                  <span><script>dwn(IDC_STREAM_TYPE)//码流索引</script></span>
                  <select id="videochannel" class="sysinput" style="width:180px;">
                        <option value="0">主码流</option>
                        <option value="1">子码流</option>
                </td>
                <td align="left">
                    <span><script>dwn(IDC_USER_GB)//sip 用户名</script></span>
                    <input type="text" id="SipUserName" class="sysinput" style="width:180px;" maxlength="31">
                </td>
                <td align="left">
                    <span><script>dwn(IDC_TIME_INTERVALS)//注册有效期</script></span>
                    <input type="text" id="SipTimeInter" class="sysinput" style="width:180px;"  onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);"  maxlength="5">
                    &nbsp;&nbsp;
                    <font ><script>dwn(IDC_GEN_UNIT_SECOND)//秒</script></font>
                </td>
              </tr>
              <tr>
                <td align="left">
                    <span><script>dwn("传输协议：")//传输协议</script></span>
                    <select id="SipTransport" class="sysinput" style="width:180px;">
                            <option value="0">udp</option>
                            <option value="1">tcp</option>
                </td>
                <td align="left">
                    <span><script>dwn(IDC_SIP_USER)//SIP 用户认证ID</script></span>
                    <input type="text" id="SipAuthenNames" class="sysinput" style="width:180px;" maxlength="31">
                </td>
                <td align="left">
                    <span><script>dwn(IDC_JCOMO_HBEATSVRPERIOD)//心跳时间间隔</script></span>
                    <input type="text" id="SipTimeHbda" class="sysinput" style="width:180px;"   onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);"  maxlength="5">
                    &nbsp;&nbsp;
                    <font ><script>dwn(IDC_GEN_UNIT_SECOND)//秒</script></font>
                </td>
              </tr>
              <tr>
                <td align="left">
                    <span><script>dwn(IDC_SIP_ADMINISTRATIVE_AREAS)//SIP服务器域</script></span>
                    <input type="text" id="dev_area" class="sysinput" style="width:180px;" maxlength="31">
                </td>
                <td align="left">
                    <span><script>dwn(IDC_SIP_PASSWORD)//SIP 用户密码</script></span>
                    <input type="password" maxlength="31" id="SipPwd"  class="sysinput" style="width:180px;" >
                </td>
                <td align="left">
                    <span><script>dwn(IDC_SYSTEM_NAMES)//设备名称</script></span>
                    <input type="text" id="SipSystemName" class="sysinput" style="width:180px;" maxlength="63">
                </td>
              </tr>
              <tr>
                <td align="left">
                    <span><script>dwn(IDC_SIP_SERVER_ID)//SIP服务器ID</script></span>
                    <input type="text" id="SipServerId" class="sysinput" style="width:180px;" maxlength="31">
                </td>
                <td align="left">
                    <span><script>dwn(IDC_ALARM_ID)//报警输入编码ID</script></span>
                    <input type="text" id="SipAlarms" class="sysinput" style="width:180px;" maxlength="31">
                </td>
              </tr>
              <tr>
                <td align="left">
                    <span><script>dwn(IDC_SIP_SERVERIP)//SIP服务器IP</script></span>
                    <input type="text" id="SipServerIp" class="sysinput" style="width:180px;" maxlength="31">
                </td>
                <td align="left">
                    <span><script>dwn(IDC_SIP_ID)//视频通道编码ID</script></span>
                    <input type="text" id="SipDev" class="sysinput" style="width:180px;" maxlength="31">
                </td>
              </tr>
              <tr>
                <td align="left">
                    <span><script>dwn(IDC_SIP_SELECTED_PORT)//SIP服务器端口</script></span>
                    <input type="text" id="SipPorts" class="sysinput" style="width:180px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" maxlength="5">
                </td>
                <td align="left">
                    <span><script>dwn(IDC_LOCAL_SELECTED_PORT)//本地SIP端口</script></span>
                    <input type="text" id="LocalPorts" class="sysinput" style="width:180px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" maxlength="5">
                </td>
              </tr>
              <tr>
                <td  align="left" colspan="2">
                  <script>dwn(IDC_LOCATION_INFO)//位置信息</script>
                  <div style="margin:5px 5px 5px 0px;border:1px solid #666"></div>
                </td>
              </tr>

              <tr>
                <td align="left"  colspan="2">
                  <span><script>dwn(IDC_LOCATION_INFORMATION)//位置信息</script></span>
                  <input type="text"  maxlength="31" id="GbLocation" class="sysinput" style="width:180px;" >
                </td>
              </tr>

               <tr>
                <td align="left"  colspan="2">
                  <span><script>dwn(IDC_LONGITUDE)//经度</script></span>
                  <input type="text"  maxlength="20" id="GbLongitude" class="sysinput" style="width:180px;">
                  </td>
              </tr>

                <td align="left"  colspan="2">
                  <span><script>dwn(IDC_LATITUDE)//纬度</script></span>
                  <input type="text" maxlength="20" id="GbLatitude"  class="sysinput" style="width:180px;" >
                </td>
              </tr>
              <tr>
                <td  align="left" colspan="2">
                  <div style="margin:5px 5px 5px 0px;border:1px solid #666"></div>
                  <button style="width:65px;margin-left: 180px" class="btn btn-inverse btn-black button_test index_btn" onclick="SaveNetGB()"><script>dwn(IDC_SAVE)</script></button>
                </td>
              </tr>
            </table>  
        </div>
        <div id="tabs-7">
            <table width="100%" style="margin-top:5px;" class="table_gb">
              <tr>
                <td align="left">
                    <span><script>dwn(IDC_STATUS_INFO)</script></span>
                    <label id="platform_status"></label>
                </td>
              </tr>
              <tr>
                <td align="left">
                  <span><script>dwn(IDC_DEVID)//设备ID</script></span>
                  <label id="platform_devid"></label>
                </td>
              </tr>
              <tr>
                <td align="left">
                  <span><script>dwn(IDC_QR_CODE)</script></span>
                  <img id="qrcode_path"  style="border:5px solid white"/>
                </td>
              </tr>
              <tr>
                <td  align="left">
                  <div style="margin:5px 5px 5px 0px;border:1px solid #666"></div>
                  <button style="width:90px;margin-left: 150px" class="btn btn-inverse btn-black button_test index_btn" onclick="initCloudPlatform()"><script>dwn(IDC_REFRESH)</script></button>
                </td>
              </tr>
            </table>  
        </div>
  </div>
</div>
</body>
</html>
