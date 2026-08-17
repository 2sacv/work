<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />

<link href="/css/base.css" type="text/css" rel="stylesheet"/>
<link href="/css/form.css" type="text/css" rel="stylesheet"/>
<link href="/jquery/selectTime/jquery.selectTime.css" type="text/css" rel="stylesheet"/>
<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jslider.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/jquery/selectTime/jquery.selectTime.js"></script>
<script type="text/javascript" src="/js/vmalarm.js"></script>
</head>
<body style="overflow:auto">
    <div class="left">
        <table border=0 cellPadding=0 cellSpacing=0 class="mainTable" style="width:650px;">
             <tr><td  valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_VM_TIME_STRATEGY)</script>
             </b></td></tr>
             <tr><td class="hline" ></td></tr>
             <tr>
              <td align="left">
                <label for='vmchk' style="width:150px;display:inline-block"><script>dwn(IDC_VM_VM_ALARM);//启用视频遮挡报警</script></label>
                 <input type="checkbox" name="vmchk" id="vmchk">
              </td>
             </tr>
             <tr>
              <td align="left">
                <div style="float:left;width:150px"><script>dwn(IDC_VM_ALARM_TYPE);//视频遮挡告警等级</script></div>
                <div id="vmtype" style="float:left;width:300px;height:4px;"></div>
                <div id="tdVmtype" style="float:left;margin-left:10px;"></div>
              </td>
             </tr>


             <tr><td  valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_TIME_PROTECTION)</script>
             </td></tr>
             <tr><td class="hline" ></td></tr>
             <tr>
               <td   align="left">
                 <div class='sTime' id='vm_time_protection'></div>
               </td>
             </tr>
             <tr style="height:20px;"><td></td></tr>
             <tr><td class="hline" ></td></tr>
            <tr>
              <td   align="left">
                  <button  class="BtnConfig"  onclick="SaveAlarmVM()"><script>dwn(IDC_SAVE)</script></button>
              </td>
            </tr>
           
        </table>
    </div>
</body>
</html>
