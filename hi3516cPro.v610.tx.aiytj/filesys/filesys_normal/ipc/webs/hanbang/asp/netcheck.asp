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
<script type="text/javascript" src="/js/netcheck.js"></script>
</head>
<body>
    <div class="left">
        <table border=0 cellPadding=3 cellSpacing=1 class="mainTable">
             <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_NETCHECK_SETTINGS)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td width="35%" class="caption"><script>dwn(IDC_NETCHECK_ADDR)</script></td>
                <td align="left" width="65%">
                   <input type="text" id="notaddress" value="192.168.1.211" >
                </td>
              </tr>
               <tr>
                <td class="caption"><script>dwn(IDC_NETCHECK_COUNT)</script></td>
                <td align="left">
                  <input type="text" id="notcount" style="width:100px;" value="4" maxlength="3" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">&nbsp;&nbsp;<font >(1~20)</font>
                </td>
              </tr>
              <tr>
                <td class="caption"><script>dwn(IDC_NETCHECK_SIZE)</script></td>
                <td align="left">
                   <input type="text" id="notsize" style="width:100px;" value="16" maxlength="5" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">&nbsp;&nbsp;<font >(8~1472)</font>
                </td>
              </tr>
             
              <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td colspan="2"  align="left">
                    <button  class="BtnConfig" id="btnPing" onclick="Ping();"><script>dwn(IDC_GEN_IPCHECK_SEND)</script></button>
                </td>
              </tr>
               <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_NETCHECK_RESULT)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
             <tr>
                <td colspan="2"  align="left">
                     <textarea id="notresult" class='centent' readonly="readonly" >
                   </textarea>
                </td>
              </tr>
             <tr>
                <td colspan="2"  align="left">
                    <iframe name="frmsubmit" id="frmsubmit" width="100%" height="210"  frameborder="0" scrolling="no" style="display:none;">
                  </iframe>
                </td>
              </tr>
            </table>
    </div>
</body>
</html>