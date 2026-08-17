<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
    <meta http-equiv="pragma" content="no-cache" />
    <meta http-equiv="Cache-Control" content="no-cache, must-revalidate" />
    <meta http-equiv="expires" content="0" />
    <title>Hanbang Web Service</title>
    <style type="text/css">
        #ID_BODY
        {
            margin: 0px;
            padding: 0px;
            width: 100%;
            height: 100%;
            font-family: arial,sans-serif;
            background-color: rgb(255,255,255);
            overflow: hidden;
        }
        #ID_DOWNLOAD
        {
            position: fixed;
            top: 50%;
            width: 100%;
            text-align: center;
            z-index: 10;
            font-size: 14px;
        }
        #ID_WEB_SERVICE
        {
            position: fixed;
            margin-top: 120px;
            width: 100%;
            z-index: 20;
        }
        #ID_TABLE
        {
            background: url('/image/login/web_service.jpg');
            background-repeat: no-repeat;
            background-position: center center;
            width: 533px;
            height: 400px;
        }
        .BUTTON_CLASS
        {
            border: 0px;
            background: url('/image/login/button_out.png');
            width: 63px;
            height: 23px;
            font-size: 14px;
        }
    #ID_DOWNLOAD_MANUAL
    {
            position: fixed;
            top: 600px;
            width: 100%;
            text-align: center;
            z-index: 10;
            font-size: 14px;
    
    }
        #ID_FOOTER
        {
            position: fixed;
            bottom: 10px;
            width: 100%;
            text-align: center;
            font-size: 14px;
        }
    </style>

    <script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
    <script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
    <script type="text/javascript" src="/jquery/jquery.md5.js"></script>
    <script type="text/javascript" src="/js/jcpcmd.js"></script>
    <script type="text/javascript" src="/js/stream.js"></script>
    <script type="text/javascript" src="/js/public_function.js"></script>
    <script type="text/javascript" src="/js/login.js"></script>

</head>
<body id="ID_BODY">
    <div id="ID_WEB_SERVICE"  style="display: none;">
        <table id="ID_TABLE" border="0" align="center">
            <tr>
                <td width="300px" height="200px">
                </td>
                <td>
                    <table height="200px" border="0" style="margin-top: 30px; font-size: 14px;">
                        <tr>
                            <td width="90px" height="45px">
                                <a id="log_user"></a>
                            </td>
                            <td>
                                <input id="loginuserName" type="text" tabindex="1" style="width: 100px; font-size: 14px;" />
                            </td>
                        </tr>
                        <tr>
                            <td height="45px">
                                <a id="log_pwd"></a>
                            </td>
                            <td>
                                <input id="loginpasswd" type="password" tabindex="2" style="width: 100px; font-size: 14px;" />
                            </td>
                        </tr>
                        <tr>
                            <td height="45px">
                                <a id="log_rtsp"></a>
                            </td>
                            <td>
                                <input id="loginrtsp" type="text" value="8101" tabindex="3" style="width: 100px; font-size: 14px;" />
                            </td>
                        </tr>
                        <tr>
                            <td height="45px">
                                <a id="laCurrentLanguage"></a>
                            </td>
                            <td>
                                <select id="ID_LANGUAGE" tabindex="4" style="width: 100px; font-size: 14px;" onchange="Language()">
                                    <option value="1">English</option>
                                    <option value="0">简体中文</option>
                                </select>
                            </td>
                        </tr>
                        <tr>
                            <td height="45px">
                                <input id="log_login" class="BUTTON_CLASS" type="button" value="Login" tabindex="5"
                                    style="margin-left: 23px" onclick="OnClickLogin()" onmouseover="OnMouseOverLogin()"
                                    onmouseout="OnMouseOutLogin()" />
                            </td>
                            <td>
                                <input id="log_reset" class="BUTTON_CLASS" type="button" value="Reset" tabindex="6"
                                    style="margin-left: 8px" onclick="OnClickReset()" onmouseover="OnMouseOverReset()"
                                    onmouseout="OnMouseOutReset()" />
                            </td>
                        </tr>
                    </table>
                </td>
            </tr>
            <tr>
                <td height="100px" colspan="2">
                </td>
            </tr>
        </table>
    </div>
  <div id="ID_DOWNLOAD_MANUAL"  style="display: none">
        <a id="ID_DOWNLOAD_COOMET_MANUAL"></a>
        <a id="ID_DOWNLOAD_LINK_MANUAL" target="_blank" href="http://www.cnjabsco.com/ocx/IPCameraOCX.rar"></a>&nbsp;    
  </div>
  <div id="ID_FOOTER"  style="display: none">
        <a id="ID_COPYRIGHT"></a>
  </div>
</body>
</html>
