<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/bootstrap.css" type="text/css" rel="stylesheet"/>
<link href="/css/base.css" type="text/css" rel="stylesheet"/>
<link href="/css/form.css" type="text/css" rel="stylesheet"/>
<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jslider.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/js/sysopt.js"></script>
</head>
<body>
    <div class="left">
        <table border=0 cellPadding=3 cellSpacing=1 class="mainTable">
             <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_SYSOPERA)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td width="35%" class="caption"><script>dwn(IDC_SYSREBOOT+IDC_GEN_ENABLE)</script></td>
                <td align="left" width="65%">
                  <input id="ckarb" type="checkbox" value="0" style='margin-left: 0px;'/>
                </td>
              </tr>
               <tr>
                <td class="caption"><script>dwn(IDC_AUTO_REBOOT_TIME)</script></td>
                <td align="left">
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
                      <select id="time" name="time" class="sysinput" style="width:60px;">
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
                </td>
              </tr>
              <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td colspan="2"  align="left">
                    <button  class="BtnConfig" onclick="SaveAutoreboot();" id="autoreBtn"><script>dwn(IDC_SAVE)</script></button>
                    </br></br>
                </td>
              </tr>
              <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td colspan="2"  align="left">
                    <button  class="BtnConfig" onclick="SysRet(2);" id="defaultBtn"><script>dwn(IDC_LOADDEFAULT)</script></button>
                    <button  class="BtnConfig" onclick="SysRet(0);" id="resetBtn"><script>dwn(IDC_RESETDEV)</script></button>
                </td>
              </tr>
            
              <tr>
                <td colspan="2"  align="left" id="trProgress" style="display:none;">
                    </br>
                      <div id="progressbar" style='float:left;width:350px;height:4px;'></div>
                      <div class="progress-label" style="float:left;margin-left:20px;"></div>
                </td>
              </tr>
               <tr>
                <td colspan="2"  align="left">
                    </br>
                    <div id="sysrettip" style='display:none;font-size:12pt; color:red;'></div>
                </td>
              </tr>
            </table>
    </div>
</body>
</html>