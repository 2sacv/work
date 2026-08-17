<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta http-equiv="pragma" content="no-cache" />
<meta http-equiv="Cache-Control" content="no-cache, must-revalidate" />
<meta http-equiv="expires" content="Thu, 1 Jan 1970 00:00:01 GMT" />

<link href="/css/bootstrap.css" type="text/css" rel="stylesheet"/>
<link href="/css/jquery-ui-1.10.4.css" type="text/css" rel="stylesheet"/>
<link href="/jquery/selectTime/jquery.selectTime.css" type="text/css" rel="stylesheet"/>
<link href="/css/motiondetect.css" type="text/css" rel="stylesheet"/>
<link href="/css/public_css.css" type="text/css" rel="stylesheet"/>
</head>

<body style="background: #2C2C2C;width:98%;height:100%" id='index' >
  <div style='height:600px;'>
      <div id="roi_tabs" class='tabs ui-tabs ui-widget ui-widget-content ui-corner-all'>
        <ul>
          <li><a href="#tabs-1" id="roi_tab"></a></li>
        </ul>
        <div id="tabs-1">
            <table width='70%'>
                <tr>
                    <td>
                        <div id='roi_player' valign="middle">
                    </td>
                    <td>
                      <table height='200'>
                        <tr><td><select id='roi_option' style='margin-right: 20px;'></select></td></tr>
                        <tr><td><button id='roi_select' class='btn btn-inverse' style='margin-left: 25px;width:150px'></button></td></tr>
                        <tr><td><button id='roi_seting' class='btn btn-inverse' style='margin-left: 25px;width:150px'></button></td></tr>
                        <tr><td><button id='roi_delete' class='btn btn-inverse' style='margin-left: 25px;width:150px'></button></td></tr>
                      </table>
                    </td>
                </tr>
            </table>
      </div>
  </div>
</body>
<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script> 
<script type="text/javascript" src="/jquery/jquery-ui-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/jquery/bootstrap-min.js"></script>
<script type="text/javascript" src="/jquery/selectTime/jquery.selectTime.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/js/roikeyarea.js"></script>
</html>