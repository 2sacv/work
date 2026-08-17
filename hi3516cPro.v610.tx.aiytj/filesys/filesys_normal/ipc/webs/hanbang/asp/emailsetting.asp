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
<script type="text/javascript" src="/js/emailsetting.js"></script>
</head>
<body>
    <div class="left">
        <table border=0 cellPadding=3 cellSpacing=1 class="mainTable">
             <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_EMAIL_SETTING)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td width="35%" class="caption"><script>dwn(IDC_EMAIL_SERVER)</script></td>
                <td align="left" width="65%">
                    <input type="text" id="emailserver" maxlength="63" >
                </td>
              </tr>
               <tr>
                <td class="caption"><script>dwn(IDC_EMAIL_USERNAME)</script></td>
                <td align="left">
                   <input type="text" id="emailuser" maxlength="31" >
                </td>
              </tr>
              <tr>
                <td class="caption"><script>dwn(IDC_EMAIL_PASSWORD)</script></td>
                <td align="left">
                    <input type="password" id="emailpasswd" maxlength="31" >
                </td>
              </tr>
              <tr>
                <td class="caption"><script>dwn(IDC_EMAIL_TOADDR)</script></td>
                <td align="left">
                    <input type="text" id="emailtoaddr" maxlength="63" >
                </td>
              </tr>
             
              <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td colspan="2"  align="left">
                    <button  class="BtnConfig" onclick="SaveEmail();"><script>dwn(IDC_SAVE)</script></button>
                </td>
              </tr>
            </table>
    </div>
</body>
</html>