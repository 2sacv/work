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
<script type="text/javascript" src="/js/usermanage.js"></script>
</head>
<body>
    <div class="left">
        <table border=0 cellPadding=3 cellSpacing=1 class="mainTable">
             <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_DEVATTEST)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td width="35%" class="caption"><script>dwn(IDC_DEVATTEST)</script></td>
                <td align="left" width="65%">
                   <input type="radio" id="master" name="authchk" checked='checked' value='0'>
                       <label for="master"><script>dwn(IDC_UNALBE);//禁用</script></label>
                       <input type="radio" id="slave" name="authchk" value='1'>
                       <label for="slave"><script>dwn(IDC_GEN_BASIC);//基本認證</script></label>
                       <input type="radio" id="slave" name="authchk" value='2'>
                       <label for="slave"><script>dwn(IDC_GEN_DIGIT);//摘要认证</script></label>
                </td>
              </tr>
               <tr>
                <td width="35%" class="caption"></td>
                <td align="left" width="65%">
                       <font >
                        <script>dwn(IDC_DEV_TITLE);//(进入视频页面是否需要密码的验证)</script>
                     </font>
                </td>
              </tr>
             <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_USERLIST)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>

               <tr>
                <td class="caption"><script>dwn(IDC_USER_TEXT)</script></td>
                <td align="left">
                  <input type="text" id="uname" style="width:146px;" maxlength="31" >
                </td>
              </tr>
              <tr>
                <td class="caption"><script>dwn(IDC_GROUP)</script></td>
                <td align="left">
                  <select id="selGroup" style="width:150px;padding:1px;">
                        <option value="admin">admin</option>
                        <option value="operator">operator</option>
                        <option value="user">user</option>
                    </select>
                </td>
              </tr>
              <tr>
                <td class="caption"><script>dwn(IDC_PASSWORD)</script></td>
                <td align="left">
                  <input type="password" id="passwd" style="width:146px;" maxlength="15">
                </td>
              </tr>
              <tr>
                <td class="caption"><script>dwn(IDC_PWDOK_TEXT)</script></td>
                <td align="left">
                  <input type="password" id="passwdok" style="width:146px;" maxlength="15">
                </td>
              </tr>
              <tr>
                <td colspan="2"  align="left">
                    <font >(<script>dwn(IDC_TIP+IDC_USRPASSWD_MAX+","+IDC_USRPASSWD_NOT_MODIFY)</script>)</font>
                </td>
              </tr>
               <tr>
                <td colspan="2"  align="left">
                    <button  class="BtnConfig" onclick="AddUsr();"><script>dwn(IDC_ADD)</script></button>
                    <button  class="BtnConfig" onclick="ModifyUsr();"><script>dwn(IDC_MODIFY)</script></button>
                    <button  class="BtnConfig" onclick="DelUsr();"><script>dwn(IDC_DEL)</script></button>
                </td>
              </tr>
              <tr>
                <td colspan="2"  align="left"  style="border:1px solid gray;">
                    <table id="userList" style="width:100%;"  cellpadding="0" cellspacing="0"  border="1">
                       <THEAD>
                        <tr align='center' style="height:25px;background:rgb(235,234,219)">
                          <td><script>dwn(IDC_USER);//用户名</script></td>
                          <td><script>dwn(IDC_PASSWD_TEXT);//密码</script></td>
                          <td><script>dwn(IDC_GROUP_TABLE);//用户组</script></td>
                        </tr>
                        </THEAD>
                    </table>
                </td>
              </tr>
            </table>
    </div>
</body>
</html>