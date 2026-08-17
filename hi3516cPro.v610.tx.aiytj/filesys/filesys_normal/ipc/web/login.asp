<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta http-equiv="pragma" content="no-cache" />
<meta http-equiv="Cache-Control" content="no-cache, must-revalidate" />
<meta http-equiv="expires" content="0" />
<link rel="stylesheet" href="/css/login.css?g=1.0" type="text/css"/>
<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/jquery/jquery.md5.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/js/stream.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/login.js?g=1.0"></script>
</head>
<body>
  <center id="centerDiv">
    <div class='log_div'>
       <div  class="logCenterDiv">
          <div id="lan" class="lanDiv">
              <div class="languagepart">
                  <div class="languageshow" align="left">
                    <label id="laCurrentLanguage"></label>
                  </div>
                  <div class="languagechange" id="divLanguageChoose" style="display: none; ">
                    <div class="mouseoutlan" onmouseover="this.className='mouseoverlan'" onmouseout="this.className='mouseoutlan'" onclick="Language(0)"  align="left"><label>简体中文</label>
                    </div>
                    <div class="mouseoutlan" onmouseover="this.className='mouseoverlan'" onmouseout="this.className='mouseoutlan'" onclick="Language(1)"  align="left"><label>English</label>
                    </div>
                  </div>
              </div>
          </div>
          <form id="loginForm">
            <table class="loginTable" align='center'>
              <tr>
                <td width='30%' align='right'><span id='log_user'></span>:</td>
                <td align='left'>&nbsp;&nbsp;<input type='text' class="sysinput" id="loginuserName" maxlength="31"></td>
              </tr>
              <tr>
                <td align='right'><span id='log_pwd'></span>:</td>
                <td align='left'>&nbsp;&nbsp;<input type='password' class="sysinput" id="loginpasswd" maxlength="16"></td>
              </tr>
              <tr>
                <td align='right'><span id='log_rtsp'></span>:</td>
                <td align='left'>&nbsp;&nbsp;<input type='text' class="sysinput" id="loginrtsp" maxlength="5" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);"></td>
              </tr>
            </table>
          </form>
          <div class="btnDiv" >
              <span class="btnClass" style="margin-left:90px;" id='log_login'>
              </span>
              <span  class="btnClass" style="margin-left:60px;" id='log_reset'>
              </span>
          </div>
          <div align="center" class="ocxDiv">
            <span id="ocx" class="ocxSpan"></span>
          </div>
        </div>
    </div>
  </center>
</body>
</html>
