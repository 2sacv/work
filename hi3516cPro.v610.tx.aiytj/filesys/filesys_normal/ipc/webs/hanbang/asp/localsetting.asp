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
<script type="text/javascript" src="/js/localsetting.js"></script>
</head>
<body>
    <div class="left">
        <table border=0 cellPadding=3 cellSpacing=1 class="mainTable">
             <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_RECMEMORY_SETTING)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td width="35%" class="caption"><script>dwn(IDC_REC_TIME)</script></td>
                <td align="left" width="65%">
                  <select id="RectimeSel">
                          <option value="1">1</option>
                          <option value="5">5</option>
                          <option value="10" selected="selected">10</option>
                          <option value="15">15</option>
                          <option value="20">20</option>
                          <option value="25">25</option>
                          <option value="30">30</option>
                          <option value="60">60</option>
                      </select>
                      <font ><script>dwn(IDC_GEN_UNIT_MINUTE);//分</script></font>
                </td>
              </tr>
               <tr>
                  <td  width="35%" class="caption"><script>dwn(IDC_STORAGE_PATH);</script></td>
                  <td align="left" width="65%">
                    <input type="text" id="RecPath" value="D:\IPCamera"  maxlength="50">
                  </td>
              </tr>
               <tr>
                <td></td>
                <td align="left"><b><script>dwn(IDC_STORAGE_PATH_TITLE)</script></b></td>
              </tr>

               <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_PRE_REC_SETTING)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>

              <tr>
                <td class="caption"><script>dwn(IDC_PRE_REC_TIME)</script></td>
                <td align="left">
                  <input type="text" id="PreRecTime" value="5" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" maxlength="2">
                    <font >(1~10)<script>dwn(IDC_GEN_UNIT_SECOND);//秒</script></font>
                </td>
              </tr>
              <tr>
                <td class="caption"><script>dwn(IDC_PRE_REC_ENABLE)</script></td>
                <td align="left">
                  <input type="checkbox" name="PreRecCk" id="PreRecCk">
                </td>
              </tr>

               <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_LINK_REC_SETTING)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>

              <tr>
                <td class="caption"><script>dwn(IDC_LINK_REC_TIME)</script></td>
                <td align="left">
                  <input type="text" id="AlarmRecTime" value="20"  onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" maxlength="4" >
                    <font >(1~3600)<script>dwn(IDC_GEN_UNIT_SECOND);</script></font>
                </td>
              </tr>
              <tr>
                <td class="caption"><script>dwn(IDC_LINK_REC_EN)</script></td>
                <td align="left">
                  <input type="checkbox" name="AlarmRecCk" id="AlarmRecCk">
                </td>
              </tr>
              <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td colspan="2"  align="left">
                    <button  class="BtnConfig" onclick="SaveLocalSetting();"><script>dwn(IDC_SAVE)</script></button>
                </td>
              </tr>
            </table>
    </div>
</body>
</html>