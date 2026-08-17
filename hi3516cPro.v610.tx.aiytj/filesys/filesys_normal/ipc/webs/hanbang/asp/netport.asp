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
<script type="text/javascript" src="/js/netport.js"></script>
</head>
<body>
    <div class="left">
        <table border=0 cellPadding=3 cellSpacing=1 class="mainTable">
             <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_NETPORT)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td width="35%" class="caption"><script>dwn(IDC_WEB_PORT)</script></td>
                <td align="left" width="65%">
                    <input id="porthttp" type="text" maxlength="5" style="width:150px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">&nbsp;&nbsp;
                    <label for='httpen' id="lahttpen"><script>dwn(IDC_UPNP_SWITCH);//UPNP开关</script></label>
                    <input type="checkbox" name="httpen" id="httpen" onclick="webPort()">
                    <font id="webPort_text" name="webPort_text" style="display:none"></font>
                </td>
              </tr>
               <tr  id="trFtp">
                <td class="caption"><script>dwn(IDC_FTP_PORT)</script></td>
                <td align="left">
                  <input id="portftp" type="text" maxlength="5" style="width:150px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">&nbsp;&nbsp;
                    <label for='ftpen'><script>dwn(IDC_UPNP_SWITCH);//UPNP开关</script></label>
                    <input type="checkbox" name="ftpen" id="ftpen" onclick="frpPort()">
                    <font id="ftpPort_text" name="ftpPort_text" style="display:none"></font>
                </td>
              </tr>
              <tr>
                <td class="caption"><script>dwn(IDC_RTSP_PORT)</script></td>
                <td align="left">
                   <input id="portrtsp" type="text" maxlength="5" style="width:150px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">&nbsp;&nbsp;
                  <label for='rtspen' id="lartspen"><script>dwn(IDC_UPNP_SWITCH);//UPNP 开关</script></label>
                  <input type="checkbox" name="rtspen" id="rtspen" onclick="rtspPort()">
                  <font id="rtspPort_text" name="rtspPort_text" style="display:none"></font>
                </td>
              </tr>
              <tr style="display:none">
                <td class="caption"><script>dwn(IDC_SPEAK_PORT)</script></td>
                <td align="left">
                  <input id="portvoice" type="text" maxlength="5" style="width:150px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">&nbsp;&nbsp;
                    <label for='voiceen'><script>dwn(IDC_UPNP_SWITCH);//UPNP 开关</script></label>
                    <input type="checkbox" name="voiceen" id="voiceen" onclick="speakPort()">
                    <font id="speakPort_text" name="speakPort_text" style="display:none"></font>
                </td>
              </tr>
              <tr style="display:none">
                <td class="caption"><script>dwn(IDC_UPDATE_PORT)</script></td>
                <td align="left">
                 <input id="portupdate" type="text" maxlength="5" style="width:150px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">&nbsp;&nbsp;
                    <label for='updateen'><script>dwn(IDC_UPNP_SWITCH);//UPNP 开关</script></label>
                    <input type="checkbox" id="updateen" onclick="updatePort()">
                    <font id="updatePort_text" style="display:none"></font>
                </td>
              </tr>
              <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td colspan="2"  align="left">
                    <button  class="BtnConfig" onclick="SavePort();"><script>dwn(IDC_SAVE)</script></button>
                </td>
              </tr>
            </table>
    </div>
</body>
</html>