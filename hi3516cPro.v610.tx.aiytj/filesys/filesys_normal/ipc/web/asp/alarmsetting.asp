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

<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery-ui-1.10.4.custom-min.js"></script> 
<script type="text/javascript" src="/jquery/jquery-ui-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/jquery/bootstrap-min.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/jquery/selectTime/jquery.selectTime.js"></script>
<script type="text/javascript" src="/jquery/jquery.timers.js"></script>
<script type="text/javascript" src="/js/motiondetect.js"></script>
<script type="text/javascript" src="/js/alarmsetting.js"></script>
<script language="javascript" for="IPCamera" event="FireLogon()">
  IPCWndInit();
</script>
<script language="javascript" for="IPCamera" event="FireAlarm(channel_index, alarm_msg)">
  handleMotionDetect(alarm_msg);
</script>
<script type="text/javascript">
   $(function(){
    var $w = $(window).width();
        if($w < 1200){
           $("body").css("width",1200);
        }
   })
</script>
<style type="text/css">
.btn-black:hover,
.btn-black:focus,
.btn-black:active{
  background: #0d75a7;
}
.index_btn{
  width: 90%;height: 36px;margin: 10px 10px 0px 10px;font-size: 18px;text-align: center;
  background: url("../image/bg_tab_btn_bottom.png");
}
.grayTable{
  border-color: gray;
}
.grayTable td{
  border: 1px solid #666;
} 
.sysinput2 {
  box-shadow: rgba(255, 255, 255, 0.1) 0 1px 0, rgba(0, 0, 0, 0.8) 0 1px 7px 0px inset;
  background: #121212 !important;
  border: 1px solid black !important;
  outline: none;
  color: #EAEAE4;
}
input[type="checkbox"]{
  vertical-align:middle;
}
</style>

</head>

<body style="background: #2C2C2C;width:99%;height:100%">
    <div style='background: #3C3D3D;'>
      <div id="tabs" class='tabs ui-tabs ui-widget ui-widget-content ui-corner-all' style='width: 100%;'>
        <ul>
          <li id="liMD" style="display:none;"><a href="#tabs-1" onclick="clickMD()"><script>dwn(IDC_MENU_MOTION)</script></a></li>
          <li id="liMF"><a href="#tabs-1" onclick="clickMF()"><script>dwn(IDC_MOTION_FOLLOW)</script></a></li>
          <li id="liCross"><a href="#tabs-1" onclick="clickCrossAlarm()"><script>dwn(IDC_monitor_cross_alarm)</script></a></li>
          <li id="liArea"><a href="#tabs-1" onclick="clickAreaAlarm()"><script>dwn(IDC_monitor_area_alarm)</script></a></li>
          <li id="liPeople"><a href="#tabs-1" onclick="clickPeopleAlarm()"><script>dwn(IDC_HUMAN_DETECTION_ALARM)</script></a></li>
          <li id="liCarDetect"><a href="#tabs-1" onclick="clickPeopleCarDetect()"><script>dwn(IDC_PEOPLE_CAR_DETECT)</script></a></li>
          <li style="display:none"><a href="#tabs-2" onclick="initVL();"><script>dwn(IDC_VL_TIME_STRATEGY)</script></a></li>
          <li id="liVM" style="display:none"><a href="#tabs-3" onclick="initVM();"><script>dwn(IDC_VM_TIME_STRATEGY)</script></a></li>
          <li id="liAI"><a href="#tabs-4" onclick="initAI();"><script>dwn(JALARM_TYPE_AI)</script></a></li>
          <li id="liAO"><a href="#tabs-5" onclick="initLinkSet();"><script>dwn(IDC_ALARMLINKATTR)</script></a></li>
          <li><a href="#tabs-6" onclick="initAlarmLink();"><script>dwn(IDC_ALARMLINKAGE)</script></a></li>
          <li><a href="#tabs-7" onclick="initVoiceLightLink();"><script>dwn(IDC_VOICE_LIGHT_LINK)</script></a></li>
          <li id="liIO"><a href="#tabs-8" onclick="initIOAlarm();"><script>dwn(IDC_IO_ALARM)</script></a></li>
        </ul>
        <div id="tabs-1">
          <table width='95%'>
            <tr>
              <td width='48%'  align="left">
                <div id='motion_ipcamer' valign="middle"></div>
              </td>
              <td  align='left'>
                <table id='m_btn_tab' align="left" class="css_table">
                  <tr>
                    <td colspan='3'>
                      <div align="left"><span id="motion_tp_tip"></span></div>
                    </td>
                  </tr>
                  <tr>
                    <td colspan='3'>
                      <div align="left"><div id='motion_time_protection' class='sTime'></div></div>
                    </td>
                  </tr>
                  <tr>
                    <td colspan='3'>
                      <input type='checkbox' id='motion_enb' style='float: left;margin:5px 0 0 30px;'>
                      <label id='motion_enb_lab' for='motion_enb' style='float: left; margin-left: 15px;'></label>
                      <label id="mdTip" style='color:red;float: left; margin-left: 25px;'></label>
                    </td>
                  </tr>
                  <tr height='50'>
                    <td colspan='3'>
                      <div style='float: left;margin-left: 20px;'><span id='ms_span'></span></div>
                      <div style='float: left;margin: 5px 0 0 20px;' id='m_slider'  class="css_slider"></div>
                      <div style='margin-left: 20px;'><span id='m_slider_span' style="display:-moz-inline-box; display:inline-block;width:20px;margin-left: 10px;"></span></div>
                    </td>
                  </tr>
                  <tr>
                      <td>
                        <button class='btn btn-inverse btn-black index_btn m_btn' id='m_select' ></button>
                      </td>
                      <td>
                        <button class='btn btn-inverse btn-black index_btn m_btn' id='m_setting'></button>
                      </td>
                      <td>
                        <button class='btn btn-inverse btn-black index_btn m_btn' id='m_delect'></button>
                      </td>
                    </tr>
                    <tr>
                        <td height='45' id='prompt_tr' colspan='3'><span id='select_prompt'></span></td>
                    </tr>
                </table>
                                
               <table id='follow_monitor_tab' align="left" style="display:none;" class="css_table">
                  <tr>
                    <td colspan='3'>
                      <div align="left"><span><script>dwn(IDC_TIME_PROTECTION);</script></span></div>
                    </td>
                  </tr>
                  <tr>
                    <td colspan='3'>
                      <div align="left"><div id='follow_time_protection' class='sTime'></div></div>
                    </td>
                  </tr>
                  <tr>
                    <td colspan='3'>
                      <input type='checkbox' id='follow_enb' style='float: left;margin:5px 0 0 30px;'>
                      <label id='follow_enb_lab' for='follow_enb' style='float: left; margin-left: 15px;'></label>
                    </td>
                  </tr>
                  
                  <tr>
                    <td width=200px>
                             <span style="margin-left: 20px;"><script>dwn(IDC_PREVSET);</script></span>
                    </td>
                    <td >
                          <select id="preset" type="text" class="sysinput2" style="width:100px;">
                                  <option value="0"><script>dwn(IDC_PTZ_close);</script></option>
                                <option value="1">1</option>
                                <option value="2">2</option>
                                <option value="3">3</option>
                                <option value="4">4</option>
                                <option value="5">5</option>
                                <option value="6">6</option>
                          </select>
                    </td>
                  </tr>
                  
                  <tr>
                    <td margin-left=5px width=150px>
                             <span style="margin-left: 20px;"><script>dwn(IDC_REMAIN_TIME);</script></span>
                    </td>
                    <td >
                         <input id="remainedtime" type="text" class="sysinput2" style="width:100px;"  maxlength="3" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">
                    </td>
                  </tr>

                  <tr height='50'>
                      <td colspan='1'>
                          <button class='btn btn-inverse btn-black index_btn m_btn' onclick="clickSaveFollow();"><script>dwn(IDC_SAVE);</script></button>
                      </td>
                 </tr>
                 
                 <tr>
                      <td height='45' id='prompt_tr' colspan='3'><script>dwn(IDC_PRESET_NOTES);</script></td>
                 </tr>
               </table>
                
                 <table id='cross_monitor_tab' align="left" style="display:none;"  class="css_table">
                  <tr>
                    <td colspan='3'>
                      <div align="left"><span><script>dwn(IDC_TIME_PROTECTION);</script></span></div>
                    </td>
                  </tr>
                  <tr>
                    <td colspan='3'>
                      <div align="left"><div id='cross_monitor_time_protection' class='sTime'></div></div>
                    </td>
                  </tr>
                  <tr>
                    <td colspan='3'>
                      <input type='checkbox' id='cross_monitor_enb' style='float: left;margin:5px 0 0 30px;'>
                      <label id='cross_monitor_enb_lab' for='cross_monitor_enb' style='float: left; margin-left: 15px;'><script>dwn(IDC_ENABLE_CROSS);</script></label>
                      <label id="cross_tip" style='color:red;float: left; margin-left: 25px;'></label>
                    </td>
                  </tr>
                  <tr>
                    <td colspan='3'>
                      <input type='checkbox' id='cross_blink_enb' style='float: left;margin:5px 0 0 30px;'>
                      <label for='cross_blink_enb' style='float: left; margin-left: 15px;'><script>dwn(IDC_BLINK_ENABLE);</script></label>
                    </td>
                  </tr>
                  <tr height='50'>
                    <td colspan='3'>
                      <span style="margin-left: 20px;display: none;"><script>dwn(IDC_scene_mode);</script></span>
                      <select id="select_cross_scene_mode" class="sysinput2" style="width:150px;display: none;" >
                        <option value="1"><script>dwn(IDC_PTZ_indoor_mode);</script></option>
                        <option value="0"><script>dwn(IDC_PTZ_outdoor_mode);</script></option>
                      </select>
                      <span style="margin-left:20px"><script>dwn(IDC_direction);</script></span>
                      <select id="select_cross_direction" class="sysinput2" style="width:100px;" onchange="changeCrossDirection()">
                        <option value="0">A->B</option>
                        <option value="1">B->A</option>
                        <option value="2">A<->B</option>
                      </select>
                    </td>
                  </tr>
                  <tr height='50'>
                    <td colspan='3'>
                      <div style='float: left;margin-left: 20px;'><span id='ms_span_cross_monitor'><script>dwn(IDC_sensitivity_cross);</script></span></div>
                      <div style='float: left;margin: 5px 0 0 20px;' id='m_slider_cross_monitor'  class="css_slider"></div>
                      <div style='margin-left: 20px;'><span id='m_slider_span_cross_monitor' style="display:-moz-inline-box; display:inline-block;width:20px;margin-left: 10px;"></span></div>
                    </td>
                  </tr>
                  <tr>
                      <td>
                        <button class='btn btn-inverse btn-black index_btn m_btn' onclick="clickDrawLine();" ><script>dwn(IDC_draw_line);</script></button>
                      </td>
                      <td>
                        <button class='btn btn-inverse btn-black index_btn m_btn' onclick="clickSaveLine();"><script>dwn(IDC_SAVE);</script></button>
                      </td>
                      <td>
                        <button class='btn btn-inverse btn-black index_btn m_btn' onclick="clickDelLine();"><script>dwn(IDC_DEL);</script></button>
                      </td>
                    </tr>
                </table>
                 <table id='area_monitor_tab' align="left" style="display:none;"  class="css_table">
                  <tr>
                    <td colspan='3'>
                      <div align="left"><span><script>dwn(IDC_TIME_PROTECTION);</script></span></div>
                    </td>
                  </tr>
                  <tr>
                    <td colspan='3'>
                      <div align="left"><div id='area_monitor_time_protection' class='sTime'></div></div>
                    </td>
                  </tr>
                  <tr>
                    <td colspan='3'>
                      <input type='checkbox' id='area_monitor_enb' style='float: left;margin:5px 0 0 30px;'>
                      <label id='area_monitor_enb_lab' for='area_monitor_enb' style='float: left; margin-left: 15px;'><script>dwn(IDC_ENABLE_AREA);</script></label>
                      <label id="area_tip" style='color:red;float: left; margin-left: 25px;'></label>
                    </td>
                  </tr>
                  <tr>
                    <td colspan='3'>
                      <input type='checkbox' id='area_blink_enb' style='float: left;margin:5px 0 0 30px;'>
                      <label for='area_blink_enb' style='float: left; margin-left: 15px;'><script>dwn(IDC_BLINK_ENABLE);</script></label>
                    </td>
                  </tr>
                  <tr height='50'>
                    <td colspan='3' >
                      <span style="margin-left: 20px;display: none;"><script>dwn(IDC_scene_mode);</script></span>
                      <select id="select_area_scene_mode" class="sysinput2" style="width:150px;display: none;">
                        <option value="1"><script>dwn(IDC_PTZ_indoor_mode);</script></option>
                        <option value="0"><script>dwn(IDC_PTZ_outdoor_mode);</script></option>
                      </select>
                      <span style="margin-left:20px"><script>dwn(IDC_direction);</script></span>
                      <select id="select_area_direction" class="sysinput2" style="width:150px;" >
                        <option value="0"><script>dwn(IDC_trigger_in);</script></option>
                        <option value="1"><script>dwn(IDC_trigger_leave);</script></option>
                        <option value="2"><script>dwn(IDC_trigger_all);</script></option>
                      </select>
                    </td>
                  </tr>
                  <tr height='50'>
                    <td colspan='3'>
                      <div style='float: left;margin-left: 20px;'><span id='ms_span_area_monitor'><script>dwn(IDC_sensitivity_area);</script></span></div>
                      <div style='float: left;margin: 5px 0 0 20px;' id='m_slider_area_monitor'   class="css_slider"></div>
                      <div style='margin-left: 20px;'><span id='m_slider_span_area_monitor' style="display:-moz-inline-box; display:inline-block;width:20px;margin-left: 10px;"></span></div>
                    </td>
                  </tr>
                  <tr>
                      <td>
                        <button class='btn btn-inverse btn-black index_btn m_btn' onclick="clickDrawArea();" ><script>dwn(IDC_draw_area_);</script></button>
                      </td>
                      <td>
                        <button class='btn btn-inverse btn-black index_btn m_btn' onclick="clickSaveArea();"><script>dwn(IDC_SAVE);</script></button>
                      </td>
                      <td>
                        <button class='btn btn-inverse btn-black index_btn m_btn' onclick="clickDelArea();"><script>dwn(IDC_DEL);</script></button>
                      </td>
                    </tr>
                </table>
                <table id='people_monitor_tab' align="left" style="display:none;"  class="css_table">
                  <tr>
                    <td colspan='3'>
                      <div align="left"><span><script>dwn(IDC_TIME_PROTECTION);</script></span></div>
                    </td>
                  </tr>
                  <tr>
                    <td colspan='3'>
                      <div align="left"><div id='people_monitor_time_protection' class='sTime'></div></div>
                    </td>
                  </tr>
                  <tr>
                    <td colspan='3'>
                      <input type='checkbox' id='people_monitor_enb' style='float: left;margin:5px 0 0 30px;'>
                      <label id='people_monitor_enb_lab' for='people_monitor_enb' style='float: left; margin-left: 15px;'><script>dwn(IDC_ENABLE_HUMAN_DETECTION);</script></label>
                      <label id="people_tip" style='color:red;float: left; margin-left: 25px;'></label>
                    </td>
                  </tr>
                  <tr>
                    <td colspan='3'>
                      <input type='checkbox' id='people_blink_enb' style='float: left;margin:5px 0 0 30px;'>
                      <label for='people_blink_enb' style='float: left; margin-left: 15px;'><script>dwn(IDC_HUMAN_MARK_ENABLE);</script></label>
                    </td>
                  </tr>
                  <tr>
                    <td colspan='3'>
                      <input type='checkbox' id='people_center_enb' style='float: left;margin:5px 0 0 30px;'>
                      <label for='people_center_enb' style='float: left; margin-left: 15px;'><script>dwn(IDC_HUMAN_CENTER_ENABLE);</script></label>
                    </td>
                  </tr>
                  <tr>
                    <td colspan='3'>
                      <input type='checkbox' id='face_illumination_enb' style='float: left;margin:5px 0 0 30px;'>
                      <label for='face_illumination_enb' style='float: left; margin-left: 15px;'><script>dwn(IDC_FACEAE_ENABLE);</script></label>
                    </td>
                  </tr>
                  <tr>
                    <td colspan='3'>
                      <span style="margin-left: 20px;"><script>dwn(IDC_scene_mode);</script></span>
                      <select id="select_people_scene_mode" class="sysinput2" style="width:150px;" >
                        <option value="0"><script>dwn(IDC_INDOOR_HUMAN);</script></option>
                        <option value="1"><script>dwn(IDC_OUTDOOR_HUMAN);</script></option>
                        <option value="2"><script>dwn(IDC_OUTDOOR_CAR);</script></option>
                      </select>
                    </td>
                  </tr>
                  <tr height='45'>
                    <td colspan='3'>
                      <div style='float: left;margin-left: 20px;'><span><script>dwn(IDC_BODY_DISTANCE);</script></span></div>
                      <div style='float: left;margin: 5px 0 0 20px;' id='m_slider_body_distance'   class="css_slider"></div>
                      <div style='margin-left: 20px;'><span id='m_slider_span_body_distance' style="display:-moz-inline-box; display:inline-block;width:20px;margin-left: 10px;"></span></div>
                    </td>
                  </tr>
                  <tr>
                      <td>
                        <button class='btn btn-inverse btn-black index_btn m_btn' onclick="clickSavePeople();"><script>dwn(IDC_SAVE);</script></button>
                      </td>
                      <td>
                        <button class='btn btn-inverse btn-black index_btn m_btn' onclick="clickDelPeople();"><script>dwn(IDC_DEL);</script></button>
                      </td>
                    </tr>
                </table>

                <!--车辆侦测-->
                <table id='car_detect_monitor_tab' align="left" style="display:none;"  class="css_table">
                  <tr>
                    <td colspan='3'>
                      <div align="left"><span><script>dwn(IDC_TIME_PROTECTION);</script></span></div>
                    </td>
                  </tr>
                  <tr>
                    <td colspan='3'>
                      <div align="left"><div id='car_detect_monitor_time_protection' class='sTime'></div></div>
                    </td>
                  </tr>
                  <tr>
                    <td colspan='1'>
                      <input type='checkbox' id='car_detect_monitor_enb' style='float: left;margin:5px 0 0 30px;'>
                      <label for='car_detect_monitor_enb' style='float: left; margin-left: 15px;'><script>dwn(IDC_ENABLE_PEOPLE_CAR_DETECT);</script></label>
                      <label id="car_tip" style='color:red;float: left; margin-left: 25px;'></label>
                    </td> 
                  </tr>
                  <tr>
                    <td colspan='1'>
                      <input type='checkbox' id='car_detect_blink_enb' style='float: left;margin:5px 0 0 30px;'>
                      <label for='car_detect_blink_enb' style='float: left; margin-left: 15px;'><script>dwn(IDC_CAR_MARK_ENABLE);</script></label>
                    </td>
                  </tr>			  			  
                  <tr height='45'>
                    <td colspan='3'>
                      <div style='float: left;margin-left: 20px;'><span><script>dwn(IDC_SENSITIVITY);</script></span></div>
                      <div style='float: left;margin: 5px 0 0 20px;' id='m_slider_car_detect_sensitivity'   class="css_slider"></div>
                      <div style='margin-left: 20px;'><span id='m_slider_span_car_detect_sensitivity' style="display:-moz-inline-box; display:inline-block;width:20px;margin-left: 10px;"></span></div>
                    </td>
                  </tr>
                  <tr>
                      <td>
                        <div style="margin-left:225px;">
                        <button style="width:120px;" class='btn btn-inverse btn-black index_btn m_btn' onclick="clickSaveCarDetect();"><script>dwn(IDC_SAVE);</script></button>
                        </div>  
                      </td>
                    </tr>
                </table>

              </td>
            </tr>
          </table>
        </div>
        <div id="tabs-2" >
            
             <div>
                 <label for='losschk'><script>dwn(IDC_VL_VL_ALARM);//启用视频丢失报警</script></label>
                 <input type="checkbox" name="losschk" id="losschk">
             </div>
            </br>
            <div>
                <script>dwn(IDC_TIME_PROTECTION);//时间布防</script>
            </div>
            <div align="left">
                <div id='vl_time_protection'  class='sTime'></div>  
            </div>
            <div style="margin-left:250px;">
                      <button style="width:65px;" class="btn btn-inverse btn-black index_btn" onclick="SaveAlarmVL();"><script>dwn(IDC_SAVE);</script></button>
            </div>  
        </div>
        <div id="tabs-3">
            <div>
                <table >
                  <tr height='40'>
                    <td align="left">
                       <label for='vmchk'><script>dwn(IDC_VM_VM_ALARM);//启用视频遮挡报警</script></label>
                       <input type="checkbox" id="vmchk">
                    </td>
                    <td></td>
                    <td></td>
                  </tr>
                  <tr height='40'>
                    <td align="left">
                       <script>dwn(IDC_VM_ALARM_TYPE);//视频遮挡告警等级</script>
                    </td>
                    <td align='left'   width='200px'>
                       <div id="vmtype" style="width:100%;height:8px;"></div>
                    </td>
                    <td id="tdVmtype" align="right"   width='35px'>
                    </td>
                  </tr>
                </table>
            </div>
            </br>
            <div>
                <script>dwn(IDC_TIME_PROTECTION);</script>
            </div>
            <div id='vm_time_protection'  class='sTime'></div>  
            <div style="margin-left:250px;">
                      <button style="width:65px;" class="btn btn-inverse btn-black index_btn" onclick="SaveAlarmVM();"><script>dwn(IDC_SAVE);</script></button>
            </div>  
        </div>
        <div id="tabs-4">
            <div>
                <table width='750px'>
                  <tr height='40'>
                    <td align="left">
                       <label ><script>dwn(IDC_AI_SELECT);</script></label>
                       <select id="inputChannelSel" class="sysinput2" style="width:50px;" onchange="changeInputChannel()"></select>
                 
                       <label style="margin-left:15px;" for="enableIOTime"><script>dwn(IDC_GEN_ENABLE);</script></label>
                       <input type="checkbox" id="enableIOTime"/>

                    </td>
                  </tr>
                </table>
            </div>
            </br>
            <div>
                <script>dwn(IDC_TIME_PROTECTION);</script>
            </div>
            <div id='ai_time_protection'  class='sTime'></div>
            <div align="left">
                       <label><script>dwn(IDC_COPY_CHANNEL);</script></label>
                       <input type="checkbox" id="copyChannel" onclick="copyChannelAll()"  style="margin-left:10px;"/>
                       <label for="copyChannel"><script>dwn(IDC_ALL);</script></label>
                       </br>
                       <span id="spanCopy"></span>
            </div>
            <div style="margin-left:300px;">
                      <button style="width:65px;" class="btn btn-inverse btn-black index_btn" onclick="SaveAlarmIn();"><script>dwn(IDC_SAVE);</script></button>
            </div>  
        </div>
        <div id="tabs-5">
          <table style='width: 400px;' > 
                <tr style="height:40px;">
                  <td align="left" colspan="2">
                      <div><span style='margin-top: 50px;'><script>dwn(IDC_AO_PARAMETER)</script></span></div>
                      <div style="margin:5px 30px 5px 0px;border:1px solid #666"></div>
                  </td>
               </tr> 
               <tr style="height:40px;">
                 <td align="right">
                    <script>dwn(IDC_AO0_STATUS);////输出1状态</script>
                 </td>
                 <td align="left">
                    <input type="radio" name="ao0status" id="ao0status" value="0" checked >
                    <script>dwn(IDC_AO_LEVEL_LOW);//低</script>
                    <input type="radio" name="ao0status" id="ao0status" value="1">
                    <script>dwn(IDC_AO_LEVEL_HIGHT);//高</script>
                 </td>
               </tr>
               <tr style="height:40px;display:none;">
                  <td align="right">
                      <script>dwn(IDC_AO1_STATUS);//输出2状态</script>
                  </td>
                  <td align="left">
                      <input type="radio" name="ao1status" id="ao1status" value="0" checked >
                      <script>dwn(IDC_AO_LEVEL_LOW);//低</script>
                      <input type="radio" name="ao1status" id="ao1status" value="1">
                      <script>dwn(IDC_AO_LEVEL_HIGHT);//高</script>
                  </td>
              </tr>
              <tr style="height:40px;">
                  <td align="right">
                      <script>dwn(IDC_AO0_LEVEL);//输出1报警电平</script>
                  </td>
                  <td align="left">
                      <input type="radio" name="ao0level"value="0" checked >
                      <script>dwn(IDC_AO_LEVEL_LOW);//低</script>
                      <input type="radio" name="ao0level"value="1">
                      <script>dwn(IDC_AO_LEVEL_HIGHT);//高</script>
                  </td>
              </tr>
              <tr style="height:40px;display:none;"> 
                  <td align="right">
                      <script>dwn(IDC_AO1_LEVEL);//输出2报警电平</script>
                  </td>
                  <td align="left">
                      <input type="radio" name="ao1level"value="0" checked >
                      <script>dwn(IDC_AO_LEVEL_LOW);//低</script>
                      <input type="radio" name="ao1level"value="1">
                      <script>dwn(IDC_AO_LEVEL_HIGHT);//高</script>
                  </td>
              </tr>
              <tr style="height:40px;display:none;">
                  <td align="right">
                      <script>dwn(IDC_AO_HOLDTIME);//电平保持时间</script>
                  </td>
                  <td align="left">
                      <input type="text" id="aoholdtime" style="width:120px;" maxlength="5" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" class="sysinput2">
                      <font color="white">&nbsp;(1~36000)</font>
                  </td>
              </tr>
               <tr height="30px;">
                 <td align="left" colspan="2">
               </tr>
               <tr>
                  <td align="right"></td>
                  <td align="left">
                      <button style="width:85px;margin-left:0px;margin-top:0px;" class="btn btn-inverse btn-black index_btn"  onclick="SaveLinkSet();"><script>dwn(IDC_SAVE);</script></button>
                  </td>
               </tr>   
          </table>
        </div>
        <div id="tabs-6">
          <table style='width: 60%;'>  
            <!-- <tr>
                  <td>
                       <span><script>dwn(IDC_ALARM_AUDIO_TYPE)</script></span>
                       <input type="radio" name="radioAudioType" id="audio_type_alarm" value="1" checked >
                       <label for='audio_type_alarm'><script>dwn(IDC_ALARM_AUDIO);</script></label>
                       <input type="radio" name="radioAudioType" id="audio_type_dog" value="2">
                       <label for='audio_type_dog'><script>dwn(IDC_DOG_AUDIO);</script></label>
                       <input type="radio" name="radioAudioType" id="audio_type_custom" value="3">
                       <label for='audio_type_custom'><script>dwn(IDC_CUSTOM_AUDIO);</script></label>
                  </td>
               </tr> -->
            <tr>
              <td align="left">
                  <table id="tbAlarmLink" cellpadding="0" cellspacing="0" border="1" width="1000" align="left" class="grayTable">
                    <tr>
                      <td width="180px;">&nbsp;</td>
                      <td width="120px;" align="center"><script>dwn(IDC_LINKAGE_EMAIL);</script></td>
                      <td width="150px;" align="center"><script>dwn(IDC_TIME_INTERVAL);</script></td>
                    </tr>
                    <tr style="display:none;">
                      <td align="center"><script>dwn(IDC_MD_LINKAGE);</script></td>
                      <td align="center"><input type="checkbox" id="cbMDEmail"/></td>
                      <td align="center">
                          <input id="cbMDInterval" type="text" maxlength="2" style="width:30px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" class="sysinput2" onchange="changeInterval('cbMDInterval')">
                      </td>
                    </tr>
                    <tr>
                      <td align="center"><script>dwn(IDC_MD_VGLINE);</script></td>
                      <td align="center"><input type="checkbox" id="cbVGLineEmail"/></td>
                      <td align="center">
                          <input id="cbVGLineInterval" type="text" maxlength="2" style="width:30px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" class="sysinput2" onchange="changeInterval('cbVGLineInterval')">
                      </td>
                    </tr>
                    <tr>
                      <td align="center"><script>dwn(IDC_MD_VGRECT);</script></td>
                      <td align="center"><input type="checkbox" id="cbVGRectEmail"/></td>
                      <td align="center">
                          <input id="cbVGRectInterval" type="text" maxlength="2" style="width:30px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" class="sysinput2" onchange="changeInterval('cbVGRectInterval')">
                      </td>
                    </tr>
                    <tr>
                      <td align="center"><script>dwn(IDC_HUMAN_DETECTION);</script></td>
                      <td align="center"><input type="checkbox" id="cbHumanEmail"/></td>
                      <td align="center">
                          <input id="cbHumanInterval" type="text" maxlength="2" style="width:30px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" class="sysinput2" onchange="changeInterval('cbHumanInterval')">
                      </td>
                    </tr>
                    <tr>
                          <td align="center"><script>dwn(IDC_PEOPLE_CAR_DETECT);</script></td>
                          <td align="center"><input type="checkbox" id="cbCarEmail"/></td>
                          <td align="center">
                              <input id="cbCarInterval" type="text" maxlength="2" style="width:30px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" class="sysinput2" onchange="changeInterval('cbCarInterval')">
                         </td>
                    <tr style="display:none;">
                      <td align="center"><script>dwn(IDC_VL_LINKAGE);</script></td>
                      <td align="center"><input type="checkbox" id="cbVLEmail"/></td>
                      <td align="center">
                          <input id="cbVLInterval" type="text" maxlength="2" style="width:30px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" class="sysinput2"  onchange="changeInterval('cbVLInterval')">
                      </td>
                    </tr>
                    <tr style="display:none;">
                      <td align="center"><script>dwn(IDC_VM_LINKAGE);</script></td>
                      <td align="center"><input type="checkbox" id="cbVMEmail"/></td>
                      <td align="center">
                          <input id="cbVMInterval" type="text" maxlength="2" style="width:30px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" class="sysinput2" onchange="changeInterval('cbVMInterval')">
                      </td>
                    </tr>
                      <tr style="display:none;">
                      <td align="center"><script>dwn(JALARM_TYPE_DISK_ERR);</script></td>
                      <td align="center"><input type="checkbox" id="cbDEEmail"/></td>
                      <td align="center">
                          <input id="cbDEInterval" type="text" maxlength="2" style="width:30px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" class="sysinput2"  onchange="changeInterval('cbDEInterval')">
                      </td>
                    </tr>
                  </table>
              </td>
            </tr>
            <tr>
              <td align='left'>
                <button style="width:65px;" class="btn btn-inverse btn-black index_btn"  onclick="SaveAlarmLink();"><script>dwn(IDC_SAVE);</script></button>
              </td>
            </tr>
          </table>
        </div>
        <div id="tabs-7">
          <table style='width: 700px;margin-left: 0px;'> 
            <tr>
              <td height="40" align="right" width="150">
                <script>dwn(IDC_LIGHT_ALARM_SWITCH+":");</script>
              </td>
              <td height="40" width="350" align="left">
                <input type="radio" name="LIGHT_ALARM_SWITCH" value="1" checked >
                <script>dwn(IDC_GEN_SWITCH_OPEN);</script>
                <input type="radio" name="LIGHT_ALARM_SWITCH" value="0">
                <script>dwn(IDC_GEN_SWITCH_CLOSE);</script>
              </td>
              <td align="right">
              </td>
              <td align="left">
              </td>
      <td height="40" align="left" width="30"></td>
            </tr>
            <tr>
              <td height="40" align="right" width="150">
                <script>dwn(IDC_LIGHT_ALARM_TIME+":");</script>
              </td>
              <td height="40" width="350" align="left">
                <select id="place" class="sysinput" onchange="changePlace()">
                  <option value='0'><script>dwn(IDC_NIGHT_ARM);</script></option>
                  <option value='1'><script>dwn(IDC_DAY_ARM);</script></option>
                  <option value='2'><script>dwn(IDC_FULL_ARM);</script></option>
                  <option value='3'><script>dwn(IDC_CUSTOM_ARM);</script></option>
                </select>
              </td>
              <td height="40" align="left" width="30"></td>
            </tr>
            <tr id="td_custom_time_key">
              <td height="40" align="right" width="150">
                <script>dwn(IDC_CUSTOM_TIME+":");</script>
              </td>
              <td height="40" width="500" align="left" id="td_custom_time_value">
                <select id="beginhour" name="beginhour" class="sysinput" style="width:80px;">
                  <option value="0">0</option>
                  <option value="1">1</option>
                  <option value="2">2</option>
                  <option value="3">3</option>
                  <option value="4">4</option>
                  <option value="5">5</option>
                  <option value="6">6</option>
                  <option value="7">7</option>
                  <option value="8">8</option>
                  <option value="9">9</option>
                  <option value="10">10</option>
                  <option value="11">11</option>
                  <option value="12">12</option>
                  <option value="13">13</option>
                  <option value="14">14</option>
                  <option value="15">15</option>
                  <option value="16">16</option>
                  <option value="17">17</option>
                  <option value="18">18</option>
                  <option value="19">19</option>
                  <option value="20">20</option>
                  <option value="21">21</option>
                  <option value="22">22</option>
                  <option value="23">23</option>
                </select>
                :
                <select id="beginmin" name="beginmin" class="sysinput" style="width:80px;margin-left:0px;">
                  <option value="0">0</option>
                  <option value="1">1</option>
                  <option value="2">2</option>
                  <option value="3">3</option>
                  <option value="4">4</option>
                  <option value="5">5</option>
                  <option value="6">6</option>
                  <option value="7">7</option>
                  <option value="8">8</option>
                  <option value="9">9</option>
                  <option value="10">10</option>
                  <option value="11">11</option>
                  <option value="12">12</option>
                  <option value="13">13</option>
                  <option value="14">14</option>
                  <option value="15">15</option>
                  <option value="16">16</option>
                  <option value="17">17</option>
                  <option value="18">18</option>
                  <option value="19">19</option>
                  <option value="20">20</option>
                  <option value="21">21</option>
                  <option value="22">22</option>
                  <option value="23">23</option>
                  <option value="24">24</option>
                  <option value="25">25</option>
                  <option value="26">26</option>
                  <option value="27">27</option>
                  <option value="28">28</option>
                  <option value="29">29</option>
                  <option value="30">30</option>
                  <option value="31">31</option>
                  <option value="32">32</option>
                  <option value="33">33</option>
                  <option value="34">34</option>
                  <option value="35">35</option>
                  <option value="36">36</option>
                  <option value="37">37</option>
                  <option value="38">38</option>
                  <option value="39">39</option>
                  <option value="40">40</option>
                  <option value="41">41</option>
                  <option value="42">42</option>
                  <option value="43">43</option>
                  <option value="44">44</option>
                  <option value="45">45</option>
                  <option value="46">46</option>
                  <option value="47">47</option>
                  <option value="48">48</option>
                  <option value="49">49</option>
                  <option value="50">50</option>
                  <option value="51">51</option>
                  <option value="52">52</option>
                  <option value="53">53</option>
                  <option value="54">54</option>
                  <option value="55">55</option>
                  <option value="56">56</option>
                  <option value="57">57</option>
                  <option value="58">58</option>
                  <option value="59">59</option>
                </select>
                ~

                <select id="endhour" name="endhour" class="sysinput" style="width:80px;">
                  <option value="0">0</option>
                  <option value="1">1</option>
                  <option value="2">2</option>
                  <option value="3">3</option>
                  <option value="4">4</option>
                  <option value="5">5</option>
                  <option value="6">6</option>
                  <option value="7">7</option>
                  <option value="8">8</option>
                  <option value="9">9</option>
                  <option value="10">10</option>
                  <option value="11">11</option>
                  <option value="12">12</option>
                  <option value="13">13</option>
                  <option value="14">14</option>
                  <option value="15">15</option>
                  <option value="16">16</option>
                  <option value="17">17</option>
                  <option value="18">18</option>
                  <option value="19">19</option>
                  <option value="20">20</option>
                  <option value="21">21</option>
                  <option value="22">22</option>
                  <option value="23">23</option>
                </select>
                :
                <select id="endmin" name="endmin" class="sysinput" style="width:80px;margin-left:0px;">
                  <option value="0">0</option>
                  <option value="1">1</option>
                  <option value="2">2</option>
                  <option value="3">3</option>
                  <option value="4">4</option>
                  <option value="5">5</option>
                  <option value="6">6</option>
                  <option value="7">7</option>
                  <option value="8">8</option>
                  <option value="9">9</option>
                  <option value="10">10</option>
                  <option value="11">11</option>
                  <option value="12">12</option>
                  <option value="13">13</option>
                  <option value="14">14</option>
                  <option value="15">15</option>
                  <option value="16">16</option>
                  <option value="17">17</option>
                  <option value="18">18</option>
                  <option value="19">19</option>
                  <option value="20">20</option>
                  <option value="21">21</option>
                  <option value="22">22</option>
                  <option value="23">23</option>
                  <option value="24">24</option>
                  <option value="25">25</option>
                  <option value="26">26</option>
                  <option value="27">27</option>
                  <option value="28">28</option>
                  <option value="29">29</option>
                  <option value="30">30</option>
                  <option value="31">31</option>
                  <option value="32">32</option>
                  <option value="33">33</option>
                  <option value="34">34</option>
                  <option value="35">35</option>
                  <option value="36">36</option>
                  <option value="37">37</option>
                  <option value="38">38</option>
                  <option value="39">39</option>
                  <option value="40">40</option>
                  <option value="41">41</option>
                  <option value="42">42</option>
                  <option value="43">43</option>
                  <option value="44">44</option>
                  <option value="45">45</option>
                  <option value="46">46</option>
                  <option value="47">47</option>
                  <option value="48">48</option>
                  <option value="49">49</option>
                  <option value="50">50</option>
                  <option value="51">51</option>
                  <option value="52">52</option>
                  <option value="53">53</option>
                  <option value="54">54</option>
                  <option value="55">55</option>
                  <option value="56">56</option>
                  <option value="57">57</option>
                  <option value="58">58</option>
                  <option value="59">59</option>
                </select>
              </td>
              <td height="40" align="left" width="30"></td>
            </tr>
            <tr>
              <td height="40" align="right" width="150">
                <script>dwn(IDC_LIGHT_ALARM_LENGTH);</script>
              </td>
              <td height="40" width="350" align="left">
                <input type="text" id="custom_time" style="width:120px;" maxlength="2"
                  onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" onchange="changeCustomTime()"
                  class="sysinput2">
                <font color="white">
                  <script>dwn(IDC_PTZ_second);</script>(10~60)
                </font>
              </td>
              <td height="40" align="left" width="30"></td>
            </tr>
            <tr>
              <td align="right">
                <button style="width:85px;margin-left:0px;margin-top:0px;" class="btn btn-inverse btn-black index_btn"
                  onclick="SaveVoiceLightLink();">
                  <script>dwn(IDC_SAVE);</script>
                </button>
              </td>
            </tr>
          </table>
          <div style="width:400px; margin-top:15px;border:1px solid #666"></div>
          <table style='width: 700px;margin-left: 0px;' id="td_voice_alarm">
            <tr>
              <td height="40" align="right" width="150">
                <script>dwn(IDC_VOICE_ALARM_SWITCH + ":");</script>
              </td>
              <td height="40" width="350" align="left">
                <input type="radio" name="VOICE_ALARM_SWITCH" value="1">
                <script>dwn(IDC_GEN_SWITCH_OPEN);</script>
                <input type="radio" name="VOICE_ALARM_SWITCH" value="0">
                <script>dwn(IDC_GEN_SWITCH_CLOSE);</script>
              </td>
              <td align="right">
              </td>
              <td align="left">
              </td>
              <td height="40" align="left" width="30"></td>
            </tr>
            <tr>
              <td height="40" align="right" width="150">
                <script>dwn(IDC_ALARM_AUDIO_TYPE + ":");</script>
              </td>
              <td height="40" width="350" align="left" colspan="3">
                <input type="radio" name="ALARM_VOICE" value="0">
                <script>dwn(IDC_LINKAGE_AUDIO_DEFAULT);</script>
                <input type="radio" name="ALARM_VOICE" value="1">
                <script>dwn(IDC_DOG_AUDIO);</script>
                <input type="radio" name="ALARM_VOICE" value="2">
                <script>dwn(IDC_ALARM_AUDIO);</script>
                <input type="radio" name="ALARM_VOICE" value="3">
                <script>dwn(IDC_CUSTOM_AUDIO);</script>
              </td>
              <td height="40" align="left" width="30"></td>
            </tr>
            <tr>
              <td height="40" align="right" width="150">
                <script>dwn(IDC_VOICE_ALARM_TIME + ":");</script>
              </td>
              <td height="40" width="350" align="left">
                <select id="voice_place" class="sysinput" onchange="changeVoicePlace()">
                  <option value='0'><script>dwn(IDC_NIGHT_ARM);</script></option>
                  <option value='1'><script>dwn(IDC_DAY_ARM);</script></option>
                  <option value='2'><script>dwn(IDC_FULL_ARM);</script></option>
                  <option value='3'><script>dwn(IDC_CUSTOM_ARM);</script></option>
                </select>
              </td>
              <td height="40" align="left" width="30"></td>
            </tr>
            <tr id="td_voice_custom_time_key">
              <td height="40" align="right" width="150">
                <script>dwn(IDC_CUSTOM_TIME + ":");</script>
              </td>
              <td height="40" width="500" align="left" id="td_voice_custom_time_value">
                <select id="voice_beginhour" name="voice_beginhour" class="sysinput" style="width:80px;">
                  <option value="0">0</option>
                  <option value="1">1</option>
                  <option value="2">2</option>
                  <option value="3">3</option>
                  <option value="4">4</option>
                  <option value="5">5</option>
                  <option value="6">6</option>
                  <option value="7">7</option>
                  <option value="8">8</option>
                  <option value="9">9</option>
                  <option value="10">10</option>
                  <option value="11">11</option>
                  <option value="12">12</option>
                  <option value="13">13</option>
                  <option value="14">14</option>
                  <option value="15">15</option>
                  <option value="16">16</option>
                  <option value="17">17</option>
                  <option value="18">18</option>
                  <option value="19">19</option>
                  <option value="20">20</option>
                  <option value="21">21</option>
                  <option value="22">22</option>
                  <option value="23">23</option>
                </select>
                :
                <select id="voice_beginmin" name="voice_beginmin" class="sysinput" style="width:80px;margin-left:0px;">
                  <option value="0">0</option>
                  <option value="1">1</option>
                  <option value="2">2</option>
                  <option value="3">3</option>
                  <option value="4">4</option>
                  <option value="5">5</option>
                  <option value="6">6</option>
                  <option value="7">7</option>
                  <option value="8">8</option>
                  <option value="9">9</option>
                  <option value="10">10</option>
                  <option value="11">11</option>
                  <option value="12">12</option>
                  <option value="13">13</option>
                  <option value="14">14</option>
                  <option value="15">15</option>
                  <option value="16">16</option>
                  <option value="17">17</option>
                  <option value="18">18</option>
                  <option value="19">19</option>
                  <option value="20">20</option>
                  <option value="21">21</option>
                  <option value="22">22</option>
                  <option value="23">23</option>
                  <option value="24">24</option>
                  <option value="25">25</option>
                  <option value="26">26</option>
                  <option value="27">27</option>
                  <option value="28">28</option>
                  <option value="29">29</option>
                  <option value="30">30</option>
                  <option value="31">31</option>
                  <option value="32">32</option>
                  <option value="33">33</option>
                  <option value="34">34</option>
                  <option value="35">35</option>
                  <option value="36">36</option>
                  <option value="37">37</option>
                  <option value="38">38</option>
                  <option value="39">39</option>
                  <option value="40">40</option>
                  <option value="41">41</option>
                  <option value="42">42</option>
                  <option value="43">43</option>
                  <option value="44">44</option>
                  <option value="45">45</option>
                  <option value="46">46</option>
                  <option value="47">47</option>
                  <option value="48">48</option>
                  <option value="49">49</option>
                  <option value="50">50</option>
                  <option value="51">51</option>
                  <option value="52">52</option>
                  <option value="53">53</option>
                  <option value="54">54</option>
                  <option value="55">55</option>
                  <option value="56">56</option>
                  <option value="57">57</option>
                  <option value="58">58</option>
                  <option value="59">59</option>
                </select>
                ~
  
                <select id="voice_endhour" name="voice_endhour" class="sysinput" style="width:80px;">
                  <option value="0">0</option>
                  <option value="1">1</option>
                  <option value="2">2</option>
                  <option value="3">3</option>
                  <option value="4">4</option>
                  <option value="5">5</option>
                  <option value="6">6</option>
                  <option value="7">7</option>
                  <option value="8">8</option>
                  <option value="9">9</option>
                  <option value="10">10</option>
                  <option value="11">11</option>
                  <option value="12">12</option>
                  <option value="13">13</option>
                  <option value="14">14</option>
                  <option value="15">15</option>
                  <option value="16">16</option>
                  <option value="17">17</option>
                  <option value="18">18</option>
                  <option value="19">19</option>
                  <option value="20">20</option>
                  <option value="21">21</option>
                  <option value="22">22</option>
                  <option value="23">23</option>
                </select>
                :
                <select id="voice_endmin" name="voice_endmin" class="sysinput" style="width:80px;margin-left:0px;">
                  <option value="0">0</option>
                  <option value="1">1</option>
                  <option value="2">2</option>
                  <option value="3">3</option>
                  <option value="4">4</option>
                  <option value="5">5</option>
                  <option value="6">6</option>
                  <option value="7">7</option>
                  <option value="8">8</option>
                  <option value="9">9</option>
                  <option value="10">10</option>
                  <option value="11">11</option>
                  <option value="12">12</option>
                  <option value="13">13</option>
                  <option value="14">14</option>
                  <option value="15">15</option>
                  <option value="16">16</option>
                  <option value="17">17</option>
                  <option value="18">18</option>
                  <option value="19">19</option>
                  <option value="20">20</option>
                  <option value="21">21</option>
                  <option value="22">22</option>
                  <option value="23">23</option>
                  <option value="24">24</option>
                  <option value="25">25</option>
                  <option value="26">26</option>
                  <option value="27">27</option>
                  <option value="28">28</option>
                  <option value="29">29</option>
                  <option value="30">30</option>
                  <option value="31">31</option>
                  <option value="32">32</option>
                  <option value="33">33</option>
                  <option value="34">34</option>
                  <option value="35">35</option>
                  <option value="36">36</option>
                  <option value="37">37</option>
                  <option value="38">38</option>
                  <option value="39">39</option>
                  <option value="40">40</option>
                  <option value="41">41</option>
                  <option value="42">42</option>
                  <option value="43">43</option>
                  <option value="44">44</option>
                  <option value="45">45</option>
                  <option value="46">46</option>
                  <option value="47">47</option>
                  <option value="48">48</option>
                  <option value="49">49</option>
                  <option value="50">50</option>
                  <option value="51">51</option>
                  <option value="52">52</option>
                  <option value="53">53</option>
                  <option value="54">54</option>
                  <option value="55">55</option>
                  <option value="56">56</option>
                  <option value="57">57</option>
                  <option value="58">58</option>
                  <option value="59">59</option>
                </select>
              </td>
              <td height="40" align="left" width="30"></td>
            </tr>
            <tr>
              <td height="40" align="right" width="150">
                <script>dwn(IDC_VOICE_ALARM_COUNT);</script>
              </td>
              <td height="40" width="350" align="left">
                <select id="voice_times" name="voice_times" class="sysinput" style="width:80px;">
                  <option value="1">1</option>
                  <option value="2">2</option>
                  <option value="3">3</option>
                  <option value="4">4</option>
                  <option value="5">5</option>
                  <option value="6">6</option>
                  <option value="7">7</option>
                  <option value="8">8</option>
                  <option value="9">9</option>
                  <option value="10">10</option>
                </select>
              </td>
              <td height="40" align="left" width="30"></td>
            </tr>
            <tr>
              <td align="right">
                <button style="width:85px;margin-left:0px;margin-top:0px;" class="btn btn-inverse btn-black index_btn"
                  onclick="SaveVoiceAlarm();">
                  <script>dwn(IDC_SAVE);</script>
                </button>
              </td>
            </tr>
          </table>
          <table style='width: 600px;margin-left: 0px;'>
            <tr>
              <td height="40px;">&nbsp;</td>
            </tr>
            <tr>
              <td height="40" width="100" align="left">
                <div><span style='margin-top: 50px;'>
                    <script>dwn(IDC_SIMULATE_ALARM)</script>
                  </span></div>
                <div style="width:400px; margin-top:15px;border:1px solid #666"></div>
              </td>
            </tr>
            <tr>
              <td height="40" width="400" align="left" id="tdSimulate"></td>
            </tr>
          </table>
        </div>
        <div id="tabs-8">
          <table style='width: 700px;' > 
            <tr >
              <td height="40"  align="right" width="150">
                <script>dwn(IDC_IO_ALARM_SWITCH+":");</script>
              </td>
              <td height="40" width="350" align="left">
                <input type="radio" name="IO_ALARM_SWITCH" value="1" >
                <script>dwn(IDC_GEN_SWITCH_OPEN);</script>
                <input type="radio" name="IO_ALARM_SWITCH" value="0">
                <script>dwn(IDC_GEN_SWITCH_CLOSE);</script>
              </td>
              <td align="right">
              </td>
              <td align="left">
              </td>
            </tr>
            <tr >
              <td height="40"  align="right" width="150">
                <script>dwn(IDC_IO_ALARM_TINE+":");</script>
              </td>
              <td height="40" width="350" align="left">
                <select id="io_place" class="sysinput" onchange="changeIOPlace()">
                  <option value='0'><script>dwn(IDC_NIGHT_ARM);</script></option>
                  <option value='1'><script>dwn(IDC_DAY_ARM);</script></option>
                  <option value='2'><script>dwn(IDC_FULL_ARM);</script></option>
                  <option value='3'><script>dwn(IDC_CUSTOM_ARM);</script></option>
                </select>
                <label id="io_alarmtime_night">18:00~5:59</label>
                <label id="io_alarmtime_day">6:00~17:59</label>
              </td>
            </tr>
            <tr id="td_io_custom_time_key">
              <td height="40"  align="right" width="150">
                <script>dwn(IDC_CUSTOM_TIME+":");</script>
              </td>
              <td height="40" width="500" align="left" id="td_io_custom_time_value">
                <select id="io_beginhour" name="io_beginhour" class="sysinput" style="width:80px;">
                  <option value="0">0</option>
                  <option value="1">1</option>
                  <option value="2">2</option>
                  <option value="3">3</option>
                  <option value="4">4</option>    
                  <option value="5">5</option>
                  <option value="6">6</option>
                  <option value="7">7</option>
                  <option value="8">8</option>
                  <option value="9">9</option>
                  <option value="10">10</option>
                  <option value="11">11</option>
                  <option value="12">12</option>
                  <option value="13">13</option>
                  <option value="14">14</option>
                  <option value="15">15</option>
                  <option value="16">16</option>
                  <option value="17">17</option>
                  <option value="18">18</option>
                  <option value="19">19</option>
                  <option value="20">20</option>
                  <option value="21">21</option>
                  <option value="22">22</option>
                  <option value="23">23</option>
                </select>
                :
                <select id="io_beginmin" name="io_beginmin" class="sysinput" style="width:80px;margin-left:0px;">
                  <option value="0">0</option>
                  <option value="1">1</option>
                  <option value="2">2</option>
                  <option value="3">3</option>
                  <option value="4">4</option>    
                  <option value="5">5</option>
                  <option value="6">6</option>
                  <option value="7">7</option>
                  <option value="8">8</option>
                  <option value="9">9</option>
                  <option value="10">10</option>
                  <option value="11">11</option>
                  <option value="12">12</option>
                  <option value="13">13</option>
                  <option value="14">14</option>
                  <option value="15">15</option>
                  <option value="16">16</option>
                  <option value="17">17</option>
                  <option value="18">18</option>
                  <option value="19">19</option>
                  <option value="20">20</option>
                  <option value="21">21</option>
                  <option value="22">22</option>
                  <option value="23">23</option>
                  <option value="24">24</option>
                  <option value="25">25</option>
                  <option value="26">26</option>
                  <option value="27">27</option>
                  <option value="28">28</option>
                  <option value="29">29</option>
                  <option value="30">30</option>
                  <option value="31">31</option>
                  <option value="32">32</option>
                  <option value="33">33</option>
                  <option value="34">34</option>
                  <option value="35">35</option>
                  <option value="36">36</option>
                  <option value="37">37</option>
                  <option value="38">38</option>
                  <option value="39">39</option>
                  <option value="40">40</option>
                  <option value="41">41</option>
                  <option value="42">42</option>
                  <option value="43">43</option>
                  <option value="44">44</option>
                  <option value="45">45</option>
                  <option value="46">46</option>
                  <option value="47">47</option>
                  <option value="48">48</option>
                  <option value="49">49</option>
                  <option value="50">50</option>
                  <option value="51">51</option>
                  <option value="52">52</option>
                  <option value="53">53</option>
                  <option value="54">54</option>
                  <option value="55">55</option>
                  <option value="56">56</option>
                  <option value="57">57</option>
                  <option value="58">58</option>
                  <option value="59">59</option>
                </select>
                ~

                <select id="io_endhour" name="io_endhour" class="sysinput" style="width:80px;">
                  <option value="0">0</option>
                  <option value="1">1</option>
                  <option value="2">2</option>
                  <option value="3">3</option>
                  <option value="4">4</option>    
                  <option value="5">5</option>
                  <option value="6">6</option>
                  <option value="7">7</option>
                  <option value="8">8</option>
                  <option value="9">9</option>
                  <option value="10">10</option>
                  <option value="11">11</option>
                  <option value="12">12</option>
                  <option value="13">13</option>
                  <option value="14">14</option>
                  <option value="15">15</option>
                  <option value="16">16</option>
                  <option value="17">17</option>
                  <option value="18">18</option>
                  <option value="19">19</option>
                  <option value="20">20</option>
                  <option value="21">21</option>
                  <option value="22">22</option>
                  <option value="23">23</option>
                </select>
                :
                <select id="io_endmin" name="io_endmin" class="sysinput" style="width:80px;margin-left:0px;">
                  <option value="0">0</option>
                  <option value="1">1</option>
                  <option value="2">2</option>
                  <option value="3">3</option>
                  <option value="4">4</option>    
                  <option value="5">5</option>
                  <option value="6">6</option>
                  <option value="7">7</option>
                  <option value="8">8</option>
                  <option value="9">9</option>
                  <option value="10">10</option>
                  <option value="11">11</option>
                  <option value="12">12</option>
                  <option value="13">13</option>
                  <option value="14">14</option>
                  <option value="15">15</option>
                  <option value="16">16</option>
                  <option value="17">17</option>
                  <option value="18">18</option>
                  <option value="19">19</option>
                  <option value="20">20</option>
                  <option value="21">21</option>
                  <option value="22">22</option>
                  <option value="23">23</option>
                  <option value="24">24</option>
                  <option value="25">25</option>
                  <option value="26">26</option>
                  <option value="27">27</option>
                  <option value="28">28</option>
                  <option value="29">29</option>
                  <option value="30">30</option>
                  <option value="31">31</option>
                  <option value="32">32</option>
                  <option value="33">33</option>
                  <option value="34">34</option>
                  <option value="35">35</option>
                  <option value="36">36</option>
                  <option value="37">37</option>
                  <option value="38">38</option>
                  <option value="39">39</option>
                  <option value="40">40</option>
                  <option value="41">41</option>
                  <option value="42">42</option>
                  <option value="43">43</option>
                  <option value="44">44</option>
                  <option value="45">45</option>
                  <option value="46">46</option>
                  <option value="47">47</option>
                  <option value="48">48</option>
                  <option value="49">49</option>
                  <option value="50">50</option>
                  <option value="51">51</option>
                  <option value="52">52</option>
                  <option value="53">53</option>
                  <option value="54">54</option>
                  <option value="55">55</option>
                  <option value="56">56</option>
                  <option value="57">57</option>
                  <option value="58">58</option>
                  <option value="59">59</option>
                </select>
              </td>
            </tr>
            <tr>
              <td align="right">
                <button style="width:85px;margin-left:0px;margin-top:0px;" class="btn btn-inverse btn-black index_btn"  onclick="SaveIOAlarm();"><script>dwn(IDC_SAVE);</script></button>
              </td>
            </tr> 
          </table>
        </div>
    </div>
</div>
</body>
</html>
