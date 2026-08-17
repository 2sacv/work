<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta http-equiv="pragma" content="no-cache" />
<meta http-equiv="Cache-Control" content="no-cache, must-revalidate" />
<meta http-equiv="expires" content="Thu, 1 Jan 1970 00:00:01 GMT" />

<link href="/css/bootstrap.css" type="text/css" rel="stylesheet"/>
<link href="/css/jquery-ui-1.10.4.css" type="text/css" rel="stylesheet"/>
<link href="/css/playback.css" type="text/css" rel="stylesheet"/>
<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery-ui-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/jquery/bootstrap-min.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/js/My97DatePicker/WdatePicker.js"></script>
<script type="text/javascript" src="/js/playback.js"></script>
<!-- 本地本件搜索返回事件 -->
<script language="javascript" for="PlayBack" event="FireSearchFile(strFileName)">
    $("#result_tab tbody").append("<tr><td align='left'><span style='margin-left:2px;'>"+strFileName+"</span></td></tr>")
</script>
<!-- 文件播放结束事件 -->
<script language="javascript" for="PlayBack" event="FireFileEnd(nChannelIndex)">
    finishVideo(nChannelIndex);
</script>
<!-- 切换通道事件 -->
<script language="javascript" for="PlayBack" event="FireChnIndex(nChannelIndex)">
    wndChgChn(nChannelIndex);
</script>
</head>

<body style='background: #1f1f1e;'>
    <div id="centerDiv" style="width:980px">
      <div style='height: 65px;background: #141414;width: 978px;' >
         <div id='top_logo_div'  align="left">
            <img id="top_logo_image" src="/image/logo.png" href="#" style="display:none;margin-top:-8px;margin-left:0px;"/>
            <span id="top_title_span" class="logo_title"></span>
        </div>
        <div id='top_menu_div' align="right">
          <button class='btn btn-inverse btn-black' id="videoBtn" style="margin-right:0px;">
            <img src="/image/playback.png" style="margin-top:-3px;">
            <span id='videoview'></span>
          </button>
          <button class='btn btn-inverse btn-black' id="setBtn"  style="margin-left:0px;margin-right:0px;padding-left:0px;" id="btn_play_back">
              <img src="/image/setting.png" style="margin-top:-3px;">
                <span id="set"></span>
          </button>
          <button class='btn btn-inverse btn-black' id="exitBtn" style="margin-left:0px;">
              <img src="/image/logout.png" style="margin-top:-3px;">
              <span  id='exit'></span>
          </button>
        </div>
      </div>
      <table  style="position:relative; width:100%; top:0px; left:0px;">
        <tr>
          <td width="300px" height="540">
            <div style="background:#2c2c2c;width:300;height:540px;">
              <div class='div_search'>
                <span id='search_video_lab'></span>
              </div>
              <table style='border: 1px solid #666;width:100%'>
                <tr>
                  <td width='70' align='right'><span class='lab' id='video_date'></span></td>
                  <td width='210'><input type='text' id='videotime' class='sysinput'></td>
                </tr>
                <tr>
                  <td align='right'><span class='lab' id='start_time'></span></td>
                  <td>
                    <input type='text' class='sysinput time_input' value='00' id='start_h' maxlength='2' onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" onblur="IsDigitBlur(this)">
                    <span class='lab'><script>dwn(IDC_SIMPLE_HOUR);</script></span>
                    <input type='text' class='sysinput time_input' value='00' id='start_m' maxlength='2' onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" onblur="IsDigitBlur(this)">
                    <span class='lab'><script>dwn(IDC_SIMPLE_MINUTE);</script></span>
                    <input type='text' class='sysinput time_input' value='00'  id='start_s' maxlength='2' onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" onblur="IsDigitBlur(this)">
                    <span class='lab'><script>dwn(IDC_SIMPLE_SECOND);</script></span>
                  </td>
                </tr>
                <tr>
                  <td align='right'><span class='lab' id='end_time'></span></td>
                  <td>
                    <input type='text' class='sysinput time_input' value='23' id='stop_h' maxlength='2' onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" onblur="IsDigitBlur(this)">
                    <span class='lab'><script>dwn(IDC_SIMPLE_HOUR);</script></span>
                    <input type='text' class='sysinput time_input' value='59' id='stop_m' maxlength='2' onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" onblur="IsDigitBlur(this)">
                    <span class='lab'><script>dwn(IDC_SIMPLE_MINUTE);</script></span>
                    <input type='text' class='sysinput time_input' value='59' id='stop_s' maxlength='2' onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" onblur="IsDigitBlur(this)">
                    <span class='lab'><script>dwn(IDC_SIMPLE_SECOND);</script></span>
                  </td>
                </tr>
                <tr>
                  <td align='right'><span class='lab' id='video_type'></span></td>
                  <td>
                    <select id='video_option' name='video_option'>
                        <option value="All"><script>dwn(IDC_TYPE_ALL);//所有文件</script></option>
                        <option value="Alarm"><script>dwn(IDC_TYPE_ALARM);//报警录像</script></option>
                        <option value="Manual"><script>dwn(IDC_TYPE_MANUAL);//手动录像</script></option>
                        <option value="1"><script>dwn(IDC_TYPE_DEV_SCHEDULE);//前端定时录像</script></option>
                        <option value="4"><script>dwn(IDC_TYPE_DEV_ALARM);//前端报警录像</script></option>
                        <option value="0"><script>dwn(IDC_TYPE_DEV_ALL);//前端所有录像</script></option>
                    </select>
                  </td>
                </tr>
                <tr>
                  <td colspan='2' align="right">
                    <button class='btn btn-inverse'  onclick="Search()" id='search'></button>
                  </td>
                </tr>
              </table>
              <div id='sresult_div'>
                <span id='sresult'></span>
              </div>

              <div style='overflow-y:auto;height: 50%;width:99%;'>
                <table id="result_tab" class="tableResult" onselectstart='return false;' style='-moz-user-select:none;cursor:pointer;'>
                  <tbody></tbody>
                </table>
              </div>
            </div>
          </td>
          <td width="680px">
              <div id='objects' style="width: 680px;height:540px;float:right;" align="center" valign="middle"></div>
          </td>
        </tr>
         <tr style='background:#424242;'> 
          <td align='left' width="300px">
               <img src="../image/playback/Open.png" id='openVideo' style="margin-left:10px;">&nbsp;&nbsp;
               <img src="../image/playback/Play.png" id='playVideo'>&nbsp;&nbsp;
               <img src="../image/playback/Stop.png" id='stopVideo'>&nbsp;&nbsp;
               <img src="../image/playback/Slow.png" id='slowVideo'>&nbsp;&nbsp;
               <img src="../image/playback/Fast.png" id='fastVideo'>&nbsp;&nbsp;
               <img src="../image/playback/screenshot.png" id='snapVideo'>&nbsp;&nbsp;
               <img src="../image/playback/volume.png" id='volume'>
          </td>
          <td width="680px">
               <table>
                 <tr>
                    <td  width="510px">
                      <div id='sliderplayback'></div>
                    </td>
                    <td  align='right' width="170px">
                      <span class='time_start'>00:00:00</span>
                      <span style="color:white;">/</span>
                      <span class='time_end'>00:00:00</span>
                    </td>
                 </tr>
               </table>
          </td>
        </tr>
      </table>
    </div>
</body>
</html>