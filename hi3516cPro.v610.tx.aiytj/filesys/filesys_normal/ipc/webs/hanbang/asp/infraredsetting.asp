<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />

<link href="/css/base.css" type="text/css" rel="stylesheet"/>
<link href="/css/form.css" type="text/css" rel="stylesheet"/>
<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/js/jslider.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/js/infraredsetting.js"></script>
</head>
<body>
    <div class="left">
        <table border=0 cellPadding=3 cellSpacing=1 class="mainTable">
             <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_FILL_LIGHT_SETTING)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
             
             <tr>
                 <td width="30%" class="caption">
             		    <script>dwn(IDC_INFRARED_MODE);/*灯光模式*/</script>
                 </td>
                 <td align="left" width="60%">
                     <select id="switchdevtype" style="width:150px;" onchange="changeDevtypeMode()">
                             <option value="2"><script>dwn(IDC_DBLIGHT)</script></option>
                             <option value="1"><script>dwn(IDC_STARLIGHT)</script></option>
                             <option value="0"><script>dwn(IDC_INFREDLIGHT)</script></option>
                     </select>
                 </td>
                 <td width="10%" align="right"></td>
             </tr>
             <tr>
                 <td width="30%" class="caption">
                 <script>dwn(IDC_CONTROLMODE);/*灯光开关*/</script>
                 </td>
                 <td align="left" width="60%">
                     <select id="switchmode" style="width:150px;" onchange="changeSwitchMode()">
								<option value="2"><script>dwn(IDC_AUTO_CONTROL)</script></option>
							    <option value="3"><script>dwn(IDC_DAY_AND_NIGHT)</script></option>
          			     </select>
                 </td>
                <td width="10%" align="right"></td>
             </tr>
              <tr id="tr_time_of_night" style="display:none">
                <td width="30%" class="caption">
                <script>dwn(IDC_TIME_OF_NIGHT);/*夜间时间*/</script>
                </td>
                <td align="left" width="60%" id="tdNightTime">
                </td>
                <td width="10%" align="right"></td>
              </tr>

              <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td colspan="3"  align="left">
                    <button  class="BtnConfig" onclick="SaveInfraredSet();"><script>dwn(IDC_SAVE)</script></button>
                </td>
              </tr>
            </table>
    </div>
</body>
</html>
