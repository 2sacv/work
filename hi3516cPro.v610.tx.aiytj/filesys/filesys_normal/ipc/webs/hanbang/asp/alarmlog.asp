<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />

<link href="/css/base.css" type="text/css" rel="stylesheet"/>
<link href="/css/form.css" type="text/css" rel="stylesheet"/>
<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/js/My97DatePicker/WdatePicker.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/js/alarmlog.js"></script>
<style type="text/css">
  #tbLogAlarm tr:hover{background:rgb(93,93,93)}
</style>
</head>
<body>
    <div class="left">
        <table border=0 cellPadding=3 cellSpacing=1 class="mainTable">
             <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_ALARM_LOG)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td colspan="2"  align="left">
                    <script>dwn(IDC_SYSLOG_STIME);</script>
                    <input id="startTimeAlarm" style="width:130px;"/>
                    &nbsp;
                    <script>dwn(IDC_SYSLOG_ETIME);</script>
                    <input id="endTimeAlarm" style="width:130px;"/>
                    &nbsp;
                    <script>dwn(IDC_ALARM_TYPE);</script>
                     <select id="selAlarmType" style="width:130px;"></select>  
                    <button  class="BtnConfig" onclick="searchAlarmLog();" id="searchAlarmLog"><script>dwn(IDC_SYSLOG_QUERY)</script></button>
                </td>
              </tr>
              <tr>
                <td colspan="2"  align="left" style="border:1px solid gray;">
                  <table id="tbLogAlarm"  cellpadding="1" cellspacing="3"  width="700px" align="left" class="grayTable">
                          <tr style="height:25px;background-color:rgb(210,213,221);">
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
                <td colspan="2"  align="left">
                 <div id="pagebar" align="center" style="width:700px">
                    <button  class="BtnConfig" id="pagePrevAlarm"  disabled='disabled' onclick="pagePrevAlarm();" ><script>dwn(IDC_SYSLOG_PREVIOUS)</script></button>
                      <select id="selPageAlarm" disabled='disabled' onchange="changePageAlarm();" style="width:50px;font-size:10pt;" >
                      </select>
                    <button  class="BtnConfig" id="pageNextAlarm"  disabled='disabled' onclick="pageNextAlarm();"><script>dwn(IDC_SYSLOG_NEXT)</script></button>
                  </div>
                </td>
              </tr>
            </table>
    </div>
</body>
</html>