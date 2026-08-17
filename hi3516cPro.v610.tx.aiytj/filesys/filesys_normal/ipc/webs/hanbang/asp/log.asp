<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta http-equiv="pragma" content="no-cache" />
<meta http-equiv="Cache-Control" content="no-cache, must-revalidate" />
<meta http-equiv="expires" content="Thu, 1 Jan 1970 00:00:01 GMT" />

<link href="/css/main.css" type="text/css" rel="stylesheet"/>
<link href="/css/playback.css" type="text/css" rel="stylesheet"/>
<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery-ui-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/jquery/bootstrap-min.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/js/My97DatePicker/WdatePicker.js"></script>
<script type="text/javascript" src="/js/log.js"></script>
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
              <span id="spanLog" class="active" >
                  <img src="/image/log.png"/></br>
                  <label id="laLog"></label>
              </span>
              <span  id="spanSetting">
                  <img src="/image/setting.png"/></br>
                  <label id="laSetting"></label>
              </span>
              <span  id="spanExit">
                  <img src="/image/exit.png"/></br>
                  <label id="laExit"></label>
              </span>
            </div>


              <div class='div_search'>
                <span id='sp_sys_log'></span>
              </div>
              <div class='lab' id='div_sys_log_start_time' align="center"></div>
              <div align="center">
                <input id="sys_log_start_time" style="width:190px;"/>
              </div>
              <div class='lab' id='div_sys_log_end_time' align="center"></div>
              <div align="center">
                <input id="sys_log_end_time" style="width:190px;"/>
              </div>
              <div  align="center">
                  <button onclick="SearchSysLog()" id='search_syslog' style="width:200px;margin-top:15px;line-height:30px;height:30px;"></button>
              </div>


              <div class='div_search'>
                <span id='sp_alarm_log'></span>
              </div>
              <div class='lab' id='div_alarm_log_start_time' align="center"></div>
              <div align="center">
                <input id="alarm_log_start_time" style="width:190px;"/>
              </div>
              <div class='lab' id='div_alarm_log_end_time' align="center"></div>
              <div align="center">
                <input id="alarm_log_end_time" style="width:190px;"/>
              </div>
              <div class='lab' id='div_alarm_log_type' align="center"></div>
              <div align="center">
                <select id="selAlarmType" style="width:200px;"></select>  
              </div>
              <div  align="center">
                  <button onclick="SearchAlarmLog()" id='search_alarmlog' style="width:200px;margin-top:15px;line-height:30px;height:30px;"></button>
              </div>
          </div>
          <div class="main">
            <div class="video" style="background:rgb(182,187,194);border:1px solid #666;width:100%">
               <table id="tbLog"  cellpadding="0" cellspacing="0" border="1" width="100%" align="center">
                    <tr style="height:25px;background:rgb(235,234,219)">
                        <td  width="20%" align="left" id="td1"></td>
                        <td  width="20%" align="left" id="td2"></td>
                        <td  width="60%" align="left" id="td3"></td>
                    </tr>
                </table>
            </div>
            <div class="icon-banner" align="center" >
               <div align="center" style="padding:5px;">
                 <button id="pagePrev"  disabled='disabled' onclick="pagePrev();" style="width:80px;">
                          <script>dwn(IDC_SYSLOG_PREVIOUS);</script>
                  </button>
                  <select id="selPage" disabled='disabled' onchange="changePage();" style="width:50px;font-size:10pt;"></select>
                  <button id="pageNext"  disabled='disabled' onclick="pageNext();" style="width:80px;">
                          <script>dwn(IDC_SYSLOG_NEXT);</script>
                  </button>
                </div>
            </div>
          </div>
      </div>

<div style="position:absolute;top:60px;left:50%;height:25px;line-height:25px;color:black;font-size:14px;background:rgb(247,238,80);display:none;" id="paramFailTip"></div>

</body>
</body>

</html>