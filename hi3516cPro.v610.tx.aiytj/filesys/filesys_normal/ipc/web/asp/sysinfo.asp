<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta http-equiv="pragma" content="no-cache" />
<meta http-equiv="Cache-Control" content="no-cache, must-revalidate" />
<meta http-equiv="expires" content="Thu, 1 Jan 1970 00:00:01 GMT" />

<link href="/css/bootstrap.css" type="text/css" rel="stylesheet"/>
<link href="/css/jquery-ui-1.10.4.css" type="text/css" rel="stylesheet"/>
<link href="/css/index.css" type="text/css" rel="stylesheet"/>

<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery-ui-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/jquery/jquery.timers.js"></script>
<script type="text/javascript" src="/jquery/bootstrap-min.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
</head>
<style>
  .ui-progressbar {
    position: relative;
  }
  .progress-label {
    position: absolute;
    left: 50%;
    top: 4px;z-index: 
    font-weight: bold;
    text-shadow: 1px 1px 0 #fff;
  }
  </style>

<body style="background: #2C2C2C;width:99%;height:100%">
    <div style='background: #3C3D3D;'>
      <div id="tabs" class='tabs ui-tabs ui-widget ui-widget-content ui-corner-all' style='width: 100%;' >
        <ul>
          <li><a href="#tabs-1" onclick="clickTabBasicInfo();"><script>dwn(IDC_BASICINFO)</script></a></li>
          <li><a href="#tabs-2" onclick="clickTabTimeSetting();"><script>dwn(IDC_TIMESETTING)</script></a></li>
          <li><a href="#tabs-3" onclick="clickTabSysOpera();"><script>dwn(IDC_SYSOPERA)</script></a></li>
          <li><a href="#tabs-4" onclick="clickTabSysUpdate();"><script>dwn(IDC_SYSUPDATE)</script></a></li>
          <li><a href="#tabs-5" onclick="clickTabUserManage();"><script>dwn(IDC_USERMANAGEMENT)</script></a></li>
          <li  style="display:none;" id="liPortSetting"><a href="#tabs-10" onclick="clickTabPortSetting();"><script>dwn(IDC_PORTSETTING)</script></a></li>
          <li><a href="#tabs-6" onclick="clickTabSysStatus();"><script>dwn(IDC_SYSTEMSTATUS)</script></a></li>
          <li><a href="#tabs-7" onclick="clickTabSysLog();"><script>dwn(IDC_SYSLOG_SYSLOG)</script></a></li>
          <li><a href="#tabs-8" onclick="clickTabAlarmLog();"><script>dwn(IDC_ALARM_LOG)</script></a></li>
          <li style="display:none;" id="liDMUpdate"><a href="#tabs-9" onclick="clickTabDmUpdate();"><script>dwn(IDC_DMUPDATE)</script></a></li>
        </ul>

        <div id="tabs-1">
            <table style='width: 100%;'>
              <tr>
                <td align="right" width="12%"><script>dwn(IDC_DEVNAME)</script></td>
                <td align="left"><input id="dev_name" type="text" class="sysinput" style="width:200px;"
                   maxlength="63"/></td>
              </tr>
               <tr>
                <td align="right"><script>dwn(IDC_DEVMODEL)</script></td>
                <td align="left"><div id="dev_model"   style="width:300px;margin-left: 15px;" /></td>
              </tr>
              <tr>
                <td align="right"><script>dwn(IDC_DEVNUM)</script></td>
                <td align="left"><div id="dev_num"   style="width:300px;margin-left: 15px;" /></td>
              </tr>
              <tr>
                <td align="right"><script>dwn(IDC_KERNELVERSION)</script></td>
                <td align="left"><div id="dev_bb" style="width:300px;margin-left: 15px;"/></td>
              </tr>
              <tr>
                <td align="right"><script>dwn(IDC_SERVERSION)</script></td>
                <td align="left"><div id="server_bb" style="width:300px;margin-left: 15px;"/></td>
              </tr>
              <tr>
                <td align="right"><script>dwn(IDC_WEBVERSION)</script></td>
                <td align="left"><div id="web_bb" style="width:300px;margin-left: 15px;"/></td>
              </tr>
              <tr>
                <td align="right"><script>dwn(IDC_OCXVERSION)</script></td>
                <td align="left"><div id="osd" style="width:300px;margin-left: 15px;"/></td>
              </tr>
              <tr>
                <td align="right">&nbsp;</td>
                <td align="left">
                    <button style="width:65px;" class="btn btn-inverse btn-black button_test index_btn" onclick="SaveDevName();"><script>dwn(IDC_SAVE)</script></button>
                </td>
              </tr>
            </table>
        </div>
        <div id="tabs-2">
          <table style='width: 100%;' >
               <tr>
                  <td align="left" colspan="3">
                      <div><span style='margin-top: 50px;'><script>dwn(IDC_TIMESETTING)</script></span></div>
                      <div style="margin:5px 30px 5px 0px;border:1px solid #666"></div>
                  </td>
               </tr>
               <tr height="25">
                  <td align="right"><script>dwn(IDC_TIMESETTING_TIMEZONE);//时区</script></td>
                  <td width="470" align="left">
                      <select id="TimeZone" class="sysinput" style="width:500px;font-size:10pt;">
                          <option value="0">(GMT-12:00)<script>dwn(IDC_NTP_GMT_1200_0);</script></option>
                          <option value="1">(GMT-11:00)<script>dwn(IDC_NTP_GMT_1100_0);</script></option>
                          <option value="2">(GMT-10:00)<script>dwn(IDC_NTP_GMT_1000_0);</script></option>
                          <option value="3">(GMT-09:00)<script>dwn(IDC_NTP_GMT_0900_0);</script></option>
                          <option value="4">(GMT-08:00)<script>dwn(IDC_NTP_GMT_0800_0);</script></option>
                          <option value="5">(GMT-07:00)<script>dwn(IDC_NTP_GMT_0700_0);</script></option>
                          <option value="6">(GMT-06:00)<script>dwn(IDC_NTP_GMT_0600_0);</script></option>
                          <option value="7">(GMT-05:00)<script>dwn(IDC_NTP_GMT_0500_0);</script></option>
                          <option value="8">(GMT-05:00)<script>dwn(IDC_NTP_GMT_0500_1);</script></option>
                          <option value="9">(GMT-04:00)<script>dwn(IDC_NTP_GMT_0400_0);</script></option>
                          <option value="10">(GMT-03:30)<script>dwn(IDC_NTP_GMT_0330_0);</script></option>
                          <option value="11">(GMT-03:00)<script>dwn(IDC_NTP_GMT_0300_0);</script></option>
                          <option value="12">(GMT-02:00)<script>dwn(IDC_NTP_GMT_0200_0);</script></option>
                          <option value="13">(GMT-01:00)<script>dwn(IDC_NTP_GMT_0100_0);</script></option>
                          <option value="14">(GMT)<script>dwn(IDC_NTP_GMT_0000_0);</script></option>
                          <option value="15">(GMT+01:00)<script>dwn(IDC_NTP_GMT__0100_0);</script></option>
                          <option value="16">(GMT+01:00)<script>dwn(IDC_NTP_GMT__0100_1);</script></option>
                          <option value="17">(GMT+01:00)<script>dwn(IDC_NTP_GMT__0100_2);</script></option>
                          <option value="18">(GMT+01:00)<script>dwn(IDC_NTP_GMT__0100_3);</script></option>
                          <option value="19">(GMT+02:00)<script>dwn(IDC_NTP_GMT__0200_0);</script></option>
                          <option value="20">(GMT+02:00)<script>dwn(IDC_NTP_GMT__0200_1);</script></option>
                          <option value="21">(GMT+03:00)<script>dwn(IDC_NTP_GMT__0300_0);</script></option>
                          <option value="22">(GMT+03:30)<script>dwn(IDC_NTP_GMT__0330_0);</script></option>
                          <option value="23">(GMT+04:00)<script>dwn(IDC_NTP_GMT__0400_0);</script></option>
                          <option value="24">(GMT+04:30)<script>dwn(IDC_NTP_GMT__0430_0);</script></option>
                          <option value="25">(GMT+05:00)<script>dwn(IDC_NTP_GMT__0500_0);</script></option>
                          <option value="26">(GMT+05:30)<script>dwn(IDC_NTP_GMT__0530_0);</script></option>
                          <option value="27">(GMT+06:00)<script>dwn(IDC_NTP_GMT__0600_0);</script></option>
                          <option value="28">(GMT+07:00)<script>dwn(IDC_NTP_GMT__0700_0);</script></option>
                          <option value="29">(GMT+08:00)<script>dwn(IDC_NTP_GMT__0800_0);</script></option>
                          <option value="30">(GMT+09:00)<script>dwn(IDC_NTP_GMT__0900_0);</script></option>
                          <option value="31">(GMT+09:30)<script>dwn(IDC_NTP_GMT__0930_0);</script></option>
                          <option value="32">(GMT+10:00)<script>dwn(IDC_NTP_GMT__1000_0);</script></option>
                          <option value="33">(GMT+11:00)<script>dwn(IDC_NTP_GMT__1100_0);</script></option>
                          <option value="34">(GMT+12:00)<script>dwn(IDC_NTP_GMT__1200_0);</script></option> 
                      </select>
                   </td>
                   <td>
                       <button class="btn btn-inverse btn-black button_test index_btn" style="width:65px;margin: 0px 10px 0px 10px;" onclick="SaveTimezoneSetting();"><script>dwn(IDC_SAVE)</script></button>
                  </td>
              </tr>
              <tr height="25">
                <td align="right"><script>dwn(IDC_TIME_DATE);//日期</script></td>
                <td align="left">
                    <input type="text" id="nvsdate" style="width:150px;" class="sysinput" maxlength="10">
                    <script>dwn(IDC_TIMESETTING_TIME);//时间</script>
                    <input type="text" id="nvstime" style="width:150px;" class="sysinput" maxlength="8">
                    <input type="checkbox" name="chksych" id="chksych" onclick="sychtime();">
                    <label for='chksych'><script>dwn(IDC_TIME_WITHPC);//与PC保持同步</script></label>
                </td>
                <td>
                    <button class="btn btn-inverse btn-black button_test index_btn" style="width:65px;margin: 0px 10px 0px 10px;" onclick="SaveTimeSetting();"><script>dwn(IDC_SAVE)</script></button>
                </td>
              </tr>      
               <tr>
                  <td align="left" colspan="3">
                      <div><span style='margin-top: 50px;'><script>dwn(IDC_NTP_SERVER)</script></span></div>
                      <div style="margin:5px 30px 5px 0px;border:1px solid #666"></div>
                  </td>
               </tr>
               <tr height="25">
                  <td width="50" align="right"></td>
                  <td width="470" align="left">
                      <input type="checkbox" id="ntpserviceen" style="margin-left:15px;" onclick="ntpEnable()"/>
                      <label for='ntpserviceen'><script>dwn(IDC_INTERNETTIME_TITLE);//自动与Internet时钟服务器同步</script></label>
                  </td>
                  <td></td>
               </tr>
               <tr height="25">
                  <td  width="100" align="right">
                    <script>dwn(IDC_SERVERADDR);</script>
                  </td>
                  <td width="470" align="left">
                      <input type="text" id="ntpserveraddr" class="sysinput" style="width:250px;"  maxlength="63">
                  </td>
                  <td></td>
               </tr>
               <tr height="25">
                  <td width="150" align="right">
                    <script>dwn(IDC_SERVERPORT);</script>
                  </td>
                  <td width="470" align="left">
                      <input type="text" id="ntpserverport" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" class="sysinput" style="width:150px;" maxlength="5">
                      <font >(1~65535)</font>
                  </td>
                  <td></td>
               </tr>
               <tr height="25">
                  <td width="150" align="right">
                    <script>dwn(IDC_NTP_SYC_TIME);</script>
                  </td>
                  <td width="470" align="left">
                      <input type="text" id="ntpsyctime" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" class="sysinput" style="width:150px;" maxlength="5">
                      <font >(1~65535)</font>
                  </td>
                  <td></td>
               </tr>
              <tr height="25">
                <td width="50" align="right">&nbsp;</td>
                <td width="470" align="left">
                    <button style="width:65px;margin-left:15px;" class="btn btn-inverse btn-black button_test index_btn" onclick="SaveNtpSetting();"><script>dwn(IDC_SAVE)</script></button>
                </td>
                <td></td>
              </tr>         

          </table>
        </div>
        <div id="tabs-3">
          <table style='width: 80%;'>
               <tr>
                  <td align="left" colspan="2">
                     <label for='ckarb'><script>dwn(IDC_SYSREBOOT_TIME);</script></label>
                    <input id="ckarb" type="checkbox" value="0" style='margin-left: 0px;'/>
                     &nbsp;&nbsp;
                     <script>dwn(IDC_AUTO_REBOOT_TIME);</script>
                      <select id="weekend" name="weekend"  class="sysinput" style="width:75px;">
                          <option value="0"><script>dwn(IDC_SYSREBOOT_SUNDAY);</script></option>
                          <option value="1"><script>dwn(IDC_SYSREBOOT_MONDAY);</script></option>
                          <option value="2"><script>dwn(IDC_SYSREBOOT_TUESDAY);</script></option>
                          <option value="3"><script>dwn(IDC_SYSREBOOT_WEDNESDAY);</script></option>
                          <option value="4"><script>dwn(IDC_SYSREBOOT_THURSDAY);</script></option>    
                          <option value="5"><script>dwn(IDC_SYSREBOOT_FRIDAY);</script></option>
                          <option value="6"><script>dwn(IDC_SYSREBOOT_SATURDAY);</script></option>
                          <option value="7"><script>dwn(IDC_SYSREBOOT_EVERYDAY);</script></option>
                      </select>
                      <select id="time" name="time" class="sysinput" style="width:80px;">
                          <option value="0">0:00</option>
                          <option value="1">1:00</option>
                          <option value="2">2:00</option>
                          <option value="3">3:00</option>
                          <option value="4">4:00</option>    
                          <option value="5">5:00</option>
                          <option value="6">6:00</option>
                          <option value="7">7:00</option>
                          <option value="8">8:00</option>
                          <option value="9">9:00</option>
                          <option value="10">10:00</option>
                          <option value="11">11:00</option>
                          <option value="12">12:00</option>
                          <option value="13">13:00</option>
                          <option value="14">14:00</option>
                          <option value="15">15:00</option>
                          <option value="16">16:00</option>
                          <option value="17">17:00</option>
                          <option value="18">18:00</option>
                          <option value="19">19:00</option>
                          <option value="20">20:00</option>
                          <option value="21">21:00</option>
                          <option value="22">22:00</option>
                          <option value="23">23:00</option>
                      </select>
                      <button  onclick="SaveAutoreboot();" style="width:65px;margin-top:0px;" class="btn btn-inverse btn-black button_test index_btn"  id="autoreBtn">
                        <script>dwn(IDC_SAVE);</script>
                      </button>
                  </td>
                  <td></td>
              </tr>
              <tr>
                <td  align='left'>
                  <div>
                      <button style="width:200px;" class="btn btn-inverse btn-black index_btn" onclick="SysRet(3)" id="defaultBtn">
                          <script type="text/javascript">dwn(IDC_SIMPLEDEFAULT);</script>
                      </button>
                  </div>
                </td>
                <td align="left" width="100%">
                    <div style='margin-top: 11px;'><script>dwn(IDC_SIMPLEDEFAULT_DES);</script></div>
                </td>
              </tr>
              <tr>
                <td  align='left'>
                  <div>
                      <button style="width:200px;" class="btn btn-inverse btn-black index_btn" onclick="SysRet(2)" id="defaultBtn">
                          <script type="text/javascript">dwn(IDC_LOADDEFAULT);</script>
                      </button>
                  </div>
                </td>
                <td align="left" width="100%">
                    <div style='margin-top: 11px;'><script>dwn(IDC_LOADDEFAULT_DES);</script></div>
                </td>
              </tr>
              <tr>
                <td  align='left'>
                   <div>
                      <button style="width:200px;" class="btn btn-inverse btn-black index_btn" onclick="SysRet(0)" id="resetBtn">
                          <script>dwn(IDC_RESETDEV);</script>
                      </button>
                  </div>
                </td>
              </tr>
              <tr>
                  <td align='left'>
                      </br>
                      <div id="progressbar" style='display:none;'>
                          <div class="progress-label"><script>dwn(IDC_PROGRESS_PROMPT)</script></div>
                      </div>
                      </br>
                       <div id="sysrettip" style='display:none;font-size:14pt; color:red;'>
                      </div>
                  </td>
              </tr>
          </table>
        </div>

        <div id="tabs-4">
             <iframe id='upiframe' name="aa" style='display:none;'></iframe>
             <form action="" target="aa" method="post" enctype="multipart/form-data" id="frmUpdate" name='frmUpdate'>
              <table style='width: 100%;'>
                <tr>
                  <td align='left'>
                  <div style="float:left">
                    <script>dwn(IDC_UPDATE_FILE_PATH)//文件路径</script>
                  </div>
                  <div style="float:left">
                    <input name="filepath" type="file" id="filepath" style='width: 650px;' class='sysinput' lang="en" size="15" xml:lang="en" />
                  </div>
                  <div style="float:left">
                     <button class="btn btn-inverse btn-black index_btn"  style="width:80px;margin-top:-5px;" onclick="IframeUpdate();" id='update_confirm'><script>dwn(IDC_CONFIRM);//确认</script></button>
                  </div>
                </tr>
                <tr>
                  <td>
                    <div  id='progress_div' style='display: none;'>
                      <div id="progress" style="width:750px;float: left;height:15px;"></div>
                      <span id="progress_lab" style='float: left;margin-left: 15px;'>0%</span>
                    </div>
                  </td>
                </tr>
                 <tr>
                  <td>
                     <div style='margin-left: 5px;display: none;' id='restart_prompt_div'>
                        <span id='restart_prompt' style="font-size:14pt; color:red;"><script>dwn(IDC_UPDATE_WAIT);</script></span>
                      </div>
                  </td>
                </tr>
               </table>
            </form>
           
        </div>
        
         <div id="tabs-5">
            <table style='width: 99%;margin-left: 0px;'>
              <tr>
                <td align="left">
                  <div><span style='margin-top: 20px;'><script>dwn(IDC_DEVATTEST);//设备端认证</script></span></div>
                  <div style="margin:5px 30px 5px 0px;border:1px solid #666"></div>
                  <div style="margin:10px 0 0 100px;">
                       <input type="radio" id="authnone" name="authchk" checked='checked' value='0'>
                       <label for="authnone"><script>dwn(IDC_UNALBE);//禁用</script></label>
                       <input type="radio" id="authbasic" name="authchk" value='1'>
                       <label for="authbasic"><script>dwn(IDC_GEN_BASIC);//基本認證</script></label>
                       <input type="radio" id="authdigest" name="authchk" value='2'>
                       <label for="authdigest"><script>dwn(IDC_GEN_DIGIT);//摘要认证</script></label>

                       <font >
                        <script>dwn(IDC_DEV_TITLE);//(进入视频页面是否需要密码的验证)</script>
                     </font>
                  </div>
                </br>
                </td>
              </tr>
              <tr>
                <td align="left">
                    <div><span style='margin-top: 50px;'><script>dwn(IDC_USERLIST);//用户列表</script></span></div>
                    <div style="margin:5px 30px 5px 0px;border:1px solid #666"></div>
                </td>
              </tr>
              
              <tr>
                <td align="left">
                    <table style="width:60%;" >
                        <tr>
                          <td align="right"><script>dwn(IDC_USER_TEXT);//用户名</script></td>
                          <td>
                            <input type="text" id="uname" class="sysinput" style="width:150px;" maxlength="31" >
                          </td>
                          <td align="right"><script>dwn(IDC_PASSWORD);//密码</script></td>
                          <td>
                             <input type="password" id="passwd" class="sysinput" style="width:150px;" maxlength="16">
                          </td>
                        </tr>
                        <tr>
                          <td align="right"><script>dwn(IDC_GROUP);//用户组</script></td>
                          <td>
                            <select id="selGroup" style="width:150px;" class="sysinput">
                                    <option value="admin">admin</option>
                                    <option value="operator">operator</option>
                                    <option value="user">user</option>
                                </select>
                          </td>
                          <td align="right"><script>dwn(IDC_PWDOK_TEXT);//确认密码</script></td>
                          <td>
                             <input type="password" id="passwdok" class="sysinput" style="width:150px;" maxlength="16" >
                          </td>
                        </tr>
                        <tr>
                          <td colspan="4" align="center">
                               <button style="width:90px;" class="btn btn-inverse btn-black button_test index_btn" onclick="AddUsr();"><script>dwn(IDC_ADD);//添加</script></button>
                               <button style="width:90px;" class="btn btn-inverse btn-black button_test index_btn" onclick="ModifyUsr();"><script>dwn(IDC_MODIFY);//修改</script></button>
                               <button style="width:90px;" class="btn btn-inverse btn-black button_test index_btn" onclick="DelUsr();"><script>dwn(IDC_DEL);//删除</script></button>
                          </td>
                        </tr>
                        <tr>
                          <td colspan="4" align="center">
                             <font >(<script>dwn(IDC_USERINFO_TIP)</script>)</font>
                          </td>
                        </tr>
                    </table>
                </td>
              </tr>
              <tr>
                <td align="left">
                    <table id="userList" style="width:60%;"  cellspacing="1" border="1" class="grayTable">
                       <THEAD>
                        <tr align='center' style="height:25px;background-color:#2C2C2C">
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
        <div id="tabs-6">
          <table style='width: 100%;'>
              <tr>
                  <td height="25" align="left"><script>dwn(IDC_FLUSH_INTERVAL);</script>
                    <select id="sysStatusTime" style="width:100px;" class="sysinput2">
                      <option value="5">5</option>
                      <option value="10">10</option>
                      <option value="15">15</option>
                      <option value="20">20</option>
                      <option value="25">25</option>
                      <option value="30">30</option>
                      <option value="35">35</option>
                      <option value="40">40</option>
                      <option value="45">45</option>
                      <option value="50">50</option>
                      <option value="55">55</option>
                      <option value="60">60</option>
                    </select>
                      <label for='sysStatusEn'><script>dwn(IDC_AUTO_ENABLE);//自动刷新</script></label>
                      <input type="checkbox" id="sysStatusEn" onclick="checkSysStatus()"/>
                       <button  class="btn btn-inverse btn-black button_test index_btn" style="width:65px;margin: 0px 10px 0px 10px;" onclick="FlushStatus();"><script>dwn(IDC_FLUSH);</script></button>
                  </td>
              </tr>
              <tr>
                  <td height="25" width="600px" align="left">
                      <table id="tbStatus" name="tbStatus" cellpadding="0" cellspacing="0" border="1" width="600px" align="left" class="grayTable">
                          <tr style="height:25px;background-color:#2C2C2C">
                              <td  width="50px"  align="center"><script>dwn(IDC_SEQUENCE);//序号</script></td>
                              <td  width="550px" align="center"><script>dwn(IDC_STATUSINFO);//信息</script></td>
                          </tr>
                      </table>
                  </td>
              </tr>

          </table>
        </div>
        <div id="tabs-7">
          <table style='width: 100%;'>
              <tr>
                  <td height="25" align="left">
                    <script>dwn(IDC_SYSLOG_STIME);</script>
                    <input id="startTime" class="sysinput2"/>
                    &nbsp;&nbsp;
                    <script>dwn(IDC_SYSLOG_ETIME);</script>
                    <input id="endTime" class="sysinput2"/> 
                       <button  class="btn btn-inverse btn-black button_test index_btn" style="width:80px;margin: 0px 10px 0px 10px;" onclick="searchLog();" id="searchLog">
                        <script>dwn(IDC_SYSLOG_QUERY);</script></button>
                  </td>
              </tr>
              <tr>
                  <td height="25" width="600px" align="left">
                      <table id="tbLog" name="tbLog" cellpadding="0" cellspacing="0" border="1" width="800px" align="left" class="grayTable" >
                          <tr style="height:25px;">
                              <td  width="150px" align="center">
                                <script>dwn(IDC_SYSLOG_TIME);//时间</script>
                              </td>
                              <td  width="150px" align="center">
                                <script>dwn(IDC_SYSLOG_MODULE);//模块</script>
                              </td>
                              <td  width="500px" align="center">
                                <script>dwn(IDC_SYSLOG_EVENT);//事件</script>
                              </td>
                          </tr>
                      </table>
                  </td>
              </tr>
              <tr>
                <td align="left" width="600px">
                  <div id="pagebar" align="center" style="width:600px">
                      <button id="pagePrev"  disabled='disabled' onclick="pagePrev();"  class="btn btn-inverse btn-black button_test index_btn" style="width:80px;margin-top:0px;">
                        <script>dwn(IDC_SYSLOG_PREVIOUS);</script>
                      </button>
                      <select id="selPage" disabled='disabled' onchange="changePage();" style="width:50px;font-size:10pt;" class="sysinput2">
                      </select>
                      <button id="pageNext"  disabled='disabled' onclick="pageNext();" class="btn btn-inverse btn-black button_test index_btn" style="width:80px;margin-top:0px;">
                        <script>dwn(IDC_SYSLOG_NEXT);</script>
                      </button>
                  </div>
                </td>
              </tr>
          </table>
        </div>
        <div id="tabs-8">
          <table style='width: 100%;'>
              <tr>
                  <td height="25"  align="left">
                    <script>dwn(IDC_SYSLOG_STIME);</script>
                    <input id="startTimeAlarm" class="sysinput2"/>
                    &nbsp;
                    <script>dwn(IDC_SYSLOG_ETIME);</script>
                    <input id="endTimeAlarm" class="sysinput2"/>
                    &nbsp;
                    <script>dwn(IDC_ALARM_TYPE);</script>
                     <select id="selAlarmType" class="sysinput2"></select>  
                   
                       <button  class="btn btn-inverse btn-black button_test index_btn" style="width:80px;margin: 0px 10px 0px 10px;" onclick="searchAlarmLog();" id="searchAlarmLog">
                        <script>dwn(IDC_SYSLOG_QUERY);</script></button>
                  </td>
              </tr>
              <tr>
                  <td height="25" width="600px" align="left">
                      <table id="tbLogAlarm"  cellpadding="0" cellspacing="0" border="1" width="600px" align="left" class="grayTable" >
                          <tr style="height:25px;">
                              <td  width="150px" align="center">
                                <script>dwn(IDC_SYSLOG_TIME);//时间</script>
                              </td>
                              <td  width="150px" align="center">
                                <script>dwn(IDC_TYPE);//类型</script>
                              </td>
                              <td  width="300px" align="center">
                                <script>dwn(IDC_ALARM_DESC);//描述</script>
                              </td>
                          </tr>
                      </table>
                  </td>
              </tr>
              <tr>
                <td align="left" width="600px">
                  <div id="pagebar" align="center" style="width:600px;">
                      <button id="pagePrevAlarm"  disabled='disabled' onclick="pagePrevAlarm();"  class="btn btn-inverse btn-black button_test index_btn" style="width:80px;margin-top:0px;">
                        <script>dwn(IDC_SYSLOG_PREVIOUS);</script>
                      </button>
                      <select id="selPageAlarm" disabled='disabled' onchange="changePageAlarm();" style="width:50px;font-size:10pt;" class="sysinput2">
                      </select>
                      <button id="pageNextAlarm"  disabled='disabled' onclick="pageNextAlarm();" class="btn btn-inverse btn-black button_test index_btn" style="width:80px;margin-top:0px;">
                        <script>dwn(IDC_SYSLOG_NEXT);</script>
                      </button>
                  </div>
                </td>
              </tr>
          </table>
        </div>
        <div id="tabs-9">
             <iframe id="dmifrmaeupdate" name="bbb" style="display:none;"></iframe>
             <form action="" method="post" enctype="multipart/form-data" name="dmFrmUpdate" id="dmFrmUpdate" target="bbb">
              <table style='width: 100%;'>
                <tr>
                  <td width='12%' align='right'><script>dwn(IDC_QJ_VERSION)</script></td>
                  <td width='30%'>
                        <span id="versionSpan" style="margin-left:15px;"></span>
                  </td>
                  <td></td>
                  <td></td>
                </tr>
                <tr>
                  <td width='12%' align='right'><script>dwn(IDC_CAMERA_VERSION)</script></td>
                  <td width='30%'><span id="versionCamera" style="margin-left:15px;"></span></td>
                  <td></td>
                  <td></td>
                </tr>
                <tr>
                  <td width='12%' align='right'><script>dwn(IDC_UPDATE_FILE_PATH)//文件路径</script></td>
                  <td width='30%'>
                        <input name="file" type="file" id="file" style='width: 350px;' class='sysinput' lang="en" size="25" xml:lang="en" onchange="checkFilePath()"/>
                  </td>
                  <td width='10%'>
                    <button class="btn btn-inverse btn-black index_btn"  style="width:80px;margin-top:0px;" onclick="DmIframeUpdate();" id='dm_update_confirm'><script>dwn(IDC_CONFIRM);//确认</script></button>
                   </td>

                  <td></td>
                </tr>
                <tr id="trFileName" style="display:none;">
                  <td  width='12%'  align='right'><script>dwn(IDC_FILE_NAME)</script></td>
                  <td colspan="3"><span id="fileFullPath" style="margin-left:15px;"></span></td>
                </tr>
                <tr id='progress_div_dm' style='display: none;'>
                  <td colspan="2" width="42%"/>
                     <div id="dmprogress" style="width:480px;float: left;height:15px;"></div>
                  </td>
                  <td>
                    <span id="dmprogress_lab" style='float: left;margin-left: 15px;'>0%</span>
                  </td>
                  <td></td>
                </tr>
               </table>
            </form>
            <div style="width:520px;text-align:center;margin-top:28px;font-size:18px;display:none;" id="spanDiv">
                <span style="color:red" id="spanDivDM"><script>dwn(IDC_DMUPDATE_PROMPT);//文件正在升级，请稍等…… </script></span>
            </div>
        </div>
         <div id="tabs-10">
            <table style='width: 100%;'>
              <tr style="display:none">
                <td align="right" width="12%"><script>dwn(IDC_SERIALPORT);//串口类型</script></td>
                <td align="left">
                  <select id="comtype" style="width:150px" class="sysinput">
                      <option value="0">RS232</option>
                      <option value="1">RS485</option>
                  </select>
                </td>
              </tr>
              <tr>
                <td align="right" width="12%"><script>dwn(IDC_BAUDRATE);//波特率</script></td>
                <td align="left">
                  <select id="baudrate" style="width:150px" class="sysinput">
                      <option value="300" selected="selected">300</option>
                      <option value="600">600</option>
                      <option value="1200">1200</option>
                      <option value="2400">2400</option>
                      <option value="4800">4800</option>
                      <option value="9600">9600</option>
                      <option value="19200">19200</option>
                      <option value="38400">38400</option>
                      <option value="57600">57600</option>
                      <option value="115200">115200</option>
                  </select>
                </td>
              </tr>
              <tr>
                <td align="right" width="12%"><script>dwn(IDC_DATABIT);//数据位</script></td>
                <td align="left">
                   <select id="databits" style="width:150px" class="sysinput">
                        <option value="5" selected="selected">5</option>
                        <option value="6">6</option>
                        <option value="7">7</option>
                        <option value="8">8</option>
                    </select>
                </td>
              </tr>
              <tr>
                <td align="right" width="12%"><script>dwn(IDC_STOPBIT);//停止位</script></td>
                <td align="left">
                   <select id="stopbits" style="width:150px"  class="sysinput">
                        <option value="1" selected="selected">1</option>
                        <option value="2">2</option>
                    </select>
                </td>
              </tr>
              <tr>
                <td align="right" width="12%"><script>dwn(IDC_PARITYBIT);//校验位</script></td>
                <td align="left">
                   <select id="checktype" style="width:150px"  class="sysinput">
                        <option value="N" selected="selected">
                            <script>dwn(IDC_PARITYBIT_NOTHING);//无</script>
                        </option>
                        <option value="O"><script>dwn(IDC_PARITYBIT_ODD);//奇校验</script></option>
                        <option value="E"><script>dwn(IDC_PARITYBIT_EVEN);//偶校验</script></option>
                        <option value="S"><script>dwn(IDC_PARITYBIT_SPACE);//空格</script></option>
                    </select>
                </td>
              </tr>
              <tr>
                <td align="right" width="12%"><script>dwn(IDC_PROTOCOLNAME);//协议名称</script></td>
                <td align="left">
                   <select id="selProtocol" style="width:150px"  class="sysinput">
                    </select>
                </td>
              </tr>
              <tr>
                <td align="right" width="12%"><script>dwn(IDC_MACHINEADDRESS);//球机地址</script></td>
                <td align="left">
                   <select id="selAddr" style="width:150px"  class="sysinput">
                    </select>
                </td>
              </tr>
              <tr>
                <td align="right">&nbsp;</td>
                <td align="left">
                    <button style="width:65px;" class="btn btn-inverse btn-black button_test index_btn" onclick="SaveSerialPort();"><script>dwn(IDC_SAVE)</script></button>
                </td>
              </tr>
            </table>
        </div>
    </div>
</div>
<script type="text/javascript" src="/js/My97DatePicker/WdatePicker.js"></script>
<script type="text/javascript" src="/js/sysusermanage.js"></script>
<script type="text/javascript" src="/js/sysinfo.js"></script>
</body>
</html>
