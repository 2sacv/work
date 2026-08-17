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
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/jquery/selectTime/jquery.selectTime.js"></script>
<script type="text/javascript" src="/js/vlalarm.js"></script>
</head>
<body style="overflow:auto">
    <div class="left">
        <table border=0 cellPadding=0 cellSpacing=0 class="mainTable" style="width:650px;">
             <tr><td  valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_VL_TIME_STRATEGY)</script>
             </b></td></tr>
             <tr><td class="hline" ></td></tr>
             <tr>
              <td align="left">
                <label for='losschk'><script>dwn(IDC_VL_VL_ALARM);</script></label>
                 <input type="checkbox" name="losschk" id="losschk">
              </td>
             </tr>


             <tr><td  valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_TIME_PROTECTION)</script>
             </td></tr>
             <tr><td class="hline" ></td></tr>
             <tr>
               <td   align="left">
                 <div class='sTime' id='vl_time_protection'></div>
               </td>
             </tr>
             <tr style="height:20px;"><td></td></tr>
             <tr><td class="hline" ></td></tr>
            <tr>
              <td   align="left">
                  <button  class="BtnConfig"  onclick="SaveAlarmVL()"><script>dwn(IDC_SAVE)</script></button>
              </td>
            </tr>
           
        </table>
    </div>
</body>
</html>