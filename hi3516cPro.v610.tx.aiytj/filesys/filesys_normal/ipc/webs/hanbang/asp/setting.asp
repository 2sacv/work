<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta http-equiv="pragma" content="no-cache" />
<meta http-equiv="Cache-Control" content="no-cache, must-revalidate" />
<meta http-equiv="expires" content="Thu, 1 Jan 1970 00:00:01 GMT" />

<link href="/css/main.css" type="text/css" rel="stylesheet"/>
<link href="/css/left.css" type="text/css" rel="stylesheet"/>
<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery-ui-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/jquery/bootstrap-min.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/js/setting.js"></script>
</head>

<body>
      <div class="hd">
         <div style="float:left">
          <img id="top_logo_image" src="/image/logo.png" href="#"/>
       </div>
         <div style="float:right;margin-top:50px;margin-right:5px;font-size:14px;">
          <span id="spanCurrTime"></span>
       </div>
      </div>
      <div class="bd">
          <div class="side">
            <div style='background:url(/image/banner.png) repeat-x;height:52px;' class="banner">
              <span id="spanLiveview">
                <img src="/image/liveview.png"/></br>
                <label id="laLiveview"></label>
              </span>
              <span  id="spanPlayback">
                <img src="/image/playback.png"/></br>
                <label id="laPlayback"></label>
              </span>
              <span id="spanLog">
                  <img src="/image/log.png"/></br>
                  <label id="laLog"></label>
              </span>
              <span  id="spanSetting" class="active" >
                  <img src="/image/setting.png"/></br>
                  <label id="laSetting"></label>
              </span>
              <span  id="spanExit">
                  <img src="/image/exit.png"/></br>
                  <label id="laExit"></label>
              </span>
            </div>
            <div id="menuTreeDiv"></div>
          </div>
          <div class="main">
            <iframe id="homeFrame" frameborder="0" width="100%" height="100%"  scrolling="auto" marginwidth="0" marginheight="0" src="" style="position:absolute; border:1px #666666 solid">
            </iframe >
          </div>
      </div>
<div style="position:absolute;top:0;left:50%;height:25px;line-height:25px;color:white;font-size:14px;background:rgb(0,102,204);display:none;" id="paramSaveTip"></div>

<div style="position:absolute;top:60px;left:50%;height:25px;line-height:25px;color:black;font-size:14px;background:rgb(247,238,80);display:none;" id="paramFailTip"></div>

</body>

</html>