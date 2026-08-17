<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta http-equiv="pragma" content="no-cache" />
<meta http-equiv="Cache-Control" content="no-cache, must-revalidate" />
<meta http-equiv="expires" content="Thu, 1 Jan 1970 00:00:01 GMT" />
<meta http-equiv="X-UA-Compatible"content="IE=11; IE=10; IE=9; IE=8; IE=7; IE=EDGE">

<link href="/css/bootstrap.css" type="text/css" rel="stylesheet"/>
<link href="/css/jquery-ui-1.10.4.css" type="text/css" rel="stylesheet"/>
<link href="/jquery/selectTime/jquery.selectTime.css" type="text/css" rel="stylesheet"/>
<link href="/css/motiondetect.css" type="text/css" rel="stylesheet"/>
<link href="/css/public_css.css" type="text/css" rel="stylesheet"/>

<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script> 
<script type="text/javascript" src="/jquery/jquery-ui-1.10.4.custom-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.timers.js"></script>
<script type="text/javascript" src="/jquery/jquery-ui-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/jquery/bootstrap-min.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/jquery/selectTime/jquery.selectTime.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/js/stream.js"></script>
<script type="text/javascript" src="/js/videosnap.js"></script>
</head>
<style type="text/css">
    .left{ margin-left: 10px; width: 120px;}
    #samba_open,#samba_close,#nfs_open,#nfs_close{margin-left: 5px;}
    .grayTable{
      border-color: gray;
    }
    .grayTable tr{
      height:25px;
    }
    .grayTable td{
      border: 1px solid #666;
    }
</style>

<body style="background: #2C2C2C;width:99%;height:100%" id='index' >
  <div style='height:600px;'>
      <div id="frontend_tabs" class='tabs ui-tabs ui-widget ui-widget-content ui-corner-all'>
        <ul>
          <li><a href="#tabs-1" id="video_lab" onclick='show(0)'></a></li>
          <li style="display:none;"><a href="#tabs-1" id="snap_lab"  onclick='show(1)'></a></li>
          <li style="display:none;"><a href="#tabs-3" id="remote_lab" onclick='remote()'></a></li>
          <li><a href="#tabs-4" id="disk_lab" onclick='disk_tab()'></a></li>
        </ul>
        <div id="tabs-1">
          <table>
            <tr style="vertical-align: top;">
              <td width='49%' height='500px;' align='center'>
                <div id='video_ipcamer' style="margin-top:0px;">
              </td>
              <td width='50%' align='left' height='420'>
                <table width='575px'  id='video_tab'>
                  <tr>
                    <td colspan='2' align='left' width="575px">
                      <span id='video_time_strategy'></span>
                    </td>
                  </tr>
                  <tr>
                    <td colspan='2' width="575px">
                      <div class='sTime' id='video_time'></div>
                    </td>
                  </tr>
                  <tr>
                    <td align='right'><span id='video_status'></span></td>
                    <td > 
                        <span id='video_status_lab'></span>
                    </td>
                  </tr>
                  <tr>
                    <td align='right'><span id='pre_enb'></span></td>
                    <td align='left' style="width:320px;">
                        <input type='radio' id='pre_open' name='pre_rdo' value='1' style='margin-left: 15px;'>
                        <label id='pre_open_lab' for="pre_open"></label>
                        <input type='radio' id='pre_close' name='pre_rdo' value='0' checked style='margin-left: 5px;'>
                        <label id='pre_close_lab' for="pre_close"></label>
                    </td>
                  </tr>
                <tr>
                      <td align='right'><span id='pre_time_lab' ></span></td>
                      <td  align='left' >
                        <input type='text' id='pre_time' maxlength='2' class='sysinput' style='margin-top: 5px;width:60px;' onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">
                        <span id='pre_time_prompt'></span>
                      </td>
                  </tr>
                  <tr>
                    <td align='right'><span id='file_length_span'></span></td>
                    <td  align='left'>
                      <input type='text' id='file_length' maxlength='2' class='sysinput' style='margin-top: 5px;width:60px;' onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" disabled>
                      <span id='file_prompt'></span>
                    </td>
                    
                  </tr>
                  <tr>
                    <td align='right'><span id='alarm_length_span'></span></td>
                    <td  align='left'>
                      <input type='text' id='alarm_length' maxlength='2' class='sysinput' style='margin-top: 5px;width:60px;' onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" disabled>
                      <span id='alarm_prompt'></span>
                    </td>
                  </tr>
                  <tr>
                    <td align='right'><span id='disk_space_strategy'></span></td>
                    <td align='left'>
                      <input type='radio' id='disk_stop' name='disk_strategy' value='0' style='margin-left: 15px;'>
                      <label id='stop_video_lab' for="disk_stop"></label>
                      <input type='radio' id='disk_delfile' name='disk_strategy' value='1' checked style='margin-left: 5px;'>
                      <label id='del_oldfile' for="disk_delfile"></label>
                    </td>
                  </tr>
                  <tr>
                      <td><span id='video_size' style='float: right;' ></span></td>
                      <td>
                        <input type='radio' name='rec_type' value='0' style='margin-left: 15px;'>
                        <script>dwn(MASTER_STREAM);</script>
                        <input type='radio' name='rec_type' value='1' style='margin-left: 5px;'>
                        <script>dwn(SLAVE_STREAM);</script>
                      </td>
                  </tr>
                  <tr>
                    <td align='right'><button class='btn btn-inverse index_btn' id='start_video' style="width:130px;"></button></td>
                    <td align='left'><button class='btn btn-inverse index_btn' id='http_download'  style="width:130px;"></button></td>
                   
                  </tr>
                  <tr>
                    <td align='right'><button class='btn btn-inverse index_btn' id='video_save'  style="width:130px;"></button></td>
                    <td align='left'><button class='btn btn-inverse index_btn' id='ftp_download'  style="width:130px;display:none;"></button></td>
                  </tr>
                </table>

                <table width='50%' id='snap_tab' style='display: none;margin-top:0px;'>
                  <tr>
                    <td colspan='4' align='left'><span id='snap_time_strategy' style="margin-left:10px;"></span></td>
                  </tr>
                  <tr>
                    <td colspan='4'>
                      <div class='sTime' id='snap_time' style='margin: 0px 10px 10px 10px;'></div>
                    </td>
                  </tr>
                  <tr height='50'>
                    <td align='right' width='150'><span id='snap_num_lab'></span></td>
                    <td align='left' width='100'>
                      <select id='snap_num' style="width:80px;"></select>
                    </td>
                    <td  align='right' width="100">
                      <span id='stream_size_lab'></span></td>
                    </td>
                    <td id='stream_size' align='left'></td>
                  </tr>
                  <tr height='50'>
                    <td align='right'><span id='alarm_interval_lab'></span></td>
                    <td align='left'  colspan="2">
                      <input type='text' class='sysinput' style="margin-left:15px !important;width:80px;" id='alarm_time_interval' maxlength='2' onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">
                      <span id='alarm_second'></span>
                    </td>
                   <td align='left'>
                        <button class='btn btn-inverse btn-black index_btn' style="width: 120px;margin-left:0px;" id='snap_picture'></button>
                    </td>
                  </tr>
                  <tr>
                        <td  align='right'><span id='interval_lab'></span></td>
                        <td align='left'  colspan="2">
                            <input type='text' class='sysinput' style="margin-left:15px !important;width:80px;" id='time_interval' maxlength='3' onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">
                            <span id='second'></span>
                        </td>
                        <td align='left'>
                            <button class='btn btn-inverse btn-black index_btn' style="width: 120px;margin-left:0px;" id='snap_save'></button>
                        </td>
                  </tr>
                </table>
              </td>
            </tr>
          </table>
        </div>

        <div id="tabs-3">
          <table width='100%' style='margin-left: 0;'>
            <tr>
              <td>NFS</td>
              <td width='10%'>&nbsp;</td>
              <td>SAMBA</td>
            </tr>
            <tr>
              <td width='45%' align='center'>
                <table height='220' width='90%' style='border: 1px solid #666;'>
                  <tr height='50'>
                    <td align='right' width='30%'><span id='nfs_enb_lab'></span></td>
                    <td align='left'>
                      <input type='radio' id='nfs_k' value='1' name='nfs_switch' style='margin-left: 18px;'>
                      <label id='nfs_open' for="nfs_k"></label>
                      <input type='radio' id='nfs_g' value='0' name='nfs_switch' style='margin-left: 18px;'>
                      <label id='nfs_close' for="nfs_g"></label>
                    </td>
                  </tr>
                  <tr height='50'>
                    <td align='right'><span id='nfs_user_lab'></span></td>
                    <td align='left'><input type='text' class='sysinput' style="width:180px;" id='nfs_user' maxlength="31" ></td>
                  </tr>
                  <tr height='50'>
                    <td align='right'><span id='nfs_pwd_lab'></span></td>
                    <td align='left'><input type='password' class='sysinput' style="width:180px;" id='nfs_pwd' maxlength="31" ></td>
                  </tr>
                  <tr height='50'>
                    <td align='right'><span id='nfs_path_lab'></span></td>
                    <td align='left'><input type='text' class='sysinput' style="width:180px;" id='nfs_path' maxlength="80" ></td>
                  </tr>
                  <tr>
                    <td colspan='2' align='center'>
                        <button class='btn btn-inverse btn-black index_btn left' id='nfs_save'></button>
                    </td>
                  </tr>
                </table>
              </td>
              <td></td>
              <td width='45%' align='center'>
                <table align='center' height='220' width='90%' style='border: 1px solid #666;'>
                  <tr height='50'>
                    <td align='right' width='30%'><span id='samba_enb_lab'></span></td>
                    <td align='left'>
                      <input type='radio' id='s_open' name='samba_switch' value='1' style='margin-left: 18px;'>
                      <label id='samba_open' for="s_open"></label>
                      <input type='radio' id='s_close' name='samba_switch' value='0' style='margin-left: 18px;'>
                      <label id='samba_close' for="s_close"></label>
                    </td>
                  </tr>
                  <tr height='50'>
                    <td align='right'><span id='samba_user_lab'></span></td>
                    <td align='left'><input type='text' class='sysinput' style="width:180px;" id='samba_user' maxlength="31" ></td>
                  </tr>
                  <tr height='50'>
                    <td align='right'><span id='samba_pwd_lab'></span></td>
                    <td align='left'><input type='password' class='sysinput' style="width:180px;" id='samba_pwd' maxlength="31" ></td>
                  </tr>
                  <tr height='50'>
                    <td align='right'><span id='samba_path_lab'></span></td>
                    <td align='left'><input type='text' class='sysinput' style="width:180px;" id='samba_path' maxlength="127" ></td>
                  </tr>
                  <tr >
                    <td colspan='2' align='center'>
                      <button class='btn btn-inverse btn-black index_btn left' id='samba_save'></button>
                    </td>
                  </tr>
                </table>
              </td>
            </tr>
          </table>
        </div>

        <div id="tabs-4">
          <div>
            <span id='disk_interval'></span>
            <select id='disk_intervals'></select>
            <input type='checkbox' id='auto_refresh' style='margin-left: 15px;'>
            <label id='refresh_lab' for='auto_refresh'></label>
            <button class='btn btn-inverse btn-black index_btn' id='refresh'></button>
          </div>
          <div style="display:none;border:none;" id="diskRefreshLoad">
            <img src="/image/loading.gif">
          </div>
          <table border='1' width='70%'  id='tbDisk'  cellpadding="0" cellspacing="0" class="grayTable">
            <tr align='center'>
              <td><span id='partition'></span></td>
              <td><span id='total'></span></td>
              <td><span id='haveused'></span></td>
              <td><span id='remaining'></span></td>
              <td><span id='haveuseds'></span></td>
              <td><span id='format'></span></td>
            </tr>
          </table>
          <div style="margin-top:20px;color:red;">
            <label id="formatTip"></label>
          </div>
        </div>
      </div>
  </div>
</body>
</html>