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
<script type="text/javascript" src="/js/ioalarm.js"></script>
</head>
<body>
    <div class="left">
        <table border=0 cellPadding=3 cellSpacing=1 class="mainTable">
             <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_AO_PARAMETER)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td width="35%" class="caption"><script>dwn(IDC_AO0_STATUS)</script></td>
                <td align="left" width="65%">
                    <input type="radio" name="ao0status" id="ao0status" value="0" checked >
                    <script>dwn(IDC_AO_LEVEL_LOW);//低</script>
                    <input type="radio" name="ao0status" id="ao0status" value="1">
                    <script>dwn(IDC_AO_LEVEL_HIGHT);//高</script>
                </td>
              </tr>
               <tr style="display:none;">
                <td class="caption"><script>dwn(IDC_AO1_STATUS)</script></td>
                <td align="left">
                   <input type="radio" name="ao1status" id="ao1status" value="0" checked >
                      <script>dwn(IDC_AO_LEVEL_LOW);//低</script>
                      <input type="radio" name="ao1status" id="ao1status" value="1">
                      <script>dwn(IDC_AO_LEVEL_HIGHT);//高</script>
                </td>
              </tr>
              <tr>
                <td class="caption"><script>dwn(IDC_AO0_LEVEL)</script></td>
                <td align="left">
                    <input type="radio" name="ao0level"value="0" checked >
                      <script>dwn(IDC_AO_LEVEL_LOW);//低</script>
                      <input type="radio" name="ao0level"value="1">
                      <script>dwn(IDC_AO_LEVEL_HIGHT);//高</script>
                </td>
              </tr>
              <tr style="display:none;">
                <td class="caption"><script>dwn(IDC_AO1_LEVEL)</script></td>
                <td align="left">
                  <input type="radio" name="ao1level"value="0" checked >
                      <script>dwn(IDC_AO_LEVEL_LOW);//低</script>
                      <input type="radio" name="ao1level"value="1">
                      <script>dwn(IDC_AO_LEVEL_HIGHT);//高</script>
                </td>
              </tr>
              <tr style="display:none;">
                <td class="caption"><script>dwn(IDC_AO_HOLDTIME)</script></td>
                <td align="left">
                  <input type="text" id="aoholdtime" style="width:120px;" maxlength="5" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">
                      <font color="black">&nbsp;(1~36000)</font>
                </td>
              </tr>
             
              <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td colspan="2"  align="left">
                    <button  class="BtnConfig" onclick="SaveLinkSet();"><script>dwn(IDC_SAVE)</script></button>
                </td>
              </tr>
            </table>
    </div>
</body>
</html>