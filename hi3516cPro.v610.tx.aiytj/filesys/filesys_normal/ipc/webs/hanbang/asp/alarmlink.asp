<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />

<link href="/css/base.css" type="text/css" rel="stylesheet"/>
<link href="/css/form.css" type="text/css" rel="stylesheet"/>
<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/jquery/jquery.timers.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/js/alarmlink.js"></script>
<style type="text/css">
.grayTable2{margin-left:0px;margin-top:5px;color:black;}
.grayTable2 tr{height:25px;}
.grayTable2 td{border: 1px solid #666;}
</style>
</head>
<body>
    <div class="left">
        <table border=0 cellPadding=3 cellSpacing=1 class="mainTable">
              <tr><td  valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_ALARMLINKAGE)</script>
             </b></td></tr>
             <tr><td class="hline" ></td></tr>
              <tr>
                  <td align="left">
                      <table id="tbAlarmLink" name="tbAlarmLink" cellpadding="1" cellspacing="1" border="0" width="1000" align="left" class="grayTable2">
                        <tr style="background-color:rgb(210,213,221)">
                          <td width="180px;">&nbsp;</td>
                          <td width="120px;" align="center"><script>dwn(IDC_LINKAGE_EMAIL);</script></td>
                          <td width="120px;" align="center"><script>dwn(IDC_TIME_INTERVAL);</script></td>
                        </tr>
                        <tr>
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
                          <td align="center"><script>dwn(IDC_VL_LINKAGE);</script></td>
                          <td align="center"><input type="checkbox" id="cbVLEmail"/></td>
                          <td align="center">
                              <input id="cbVLInterval" type="text" maxlength="2" style="width:30px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" class="sysinput2"  onchange="changeInterval('cbVLInterval')">
                         </td>
                        </tr>
                        <tr>
                          <td align="center"><script>dwn(IDC_VM_LINKAGE);</script></td>
                          <td align="center"><input type="checkbox" id="cbVMEmail"/></td>
                          <td align="center">
                              <input id="cbVMInterval" type="text" maxlength="2" style="width:30px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" class="sysinput2" onchange="changeInterval('cbVMInterval')">
                         </td>
                        </tr>
                         <!--<tr>
                          <td align="center"><script>dwn(JALARM_TYPE_DISK_ERR);</script></td>
                          <td align="center"><input type="checkbox" id="cbDEEmail"/></td>
                          <td align="center">
                              <input id="cbDEInterval" type="text" maxlength="2" style="width:30px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" class="sysinput2"  onchange="changeInterval('cbDEInterval')">
                         </td>
                        </tr>-->
                      </table>
                  </td>
                </tr>
                <tr>
                  <td align='left'>
                    <button  class="BtnConfig" onclick="SaveAlarmLink();"><script>dwn(IDC_SAVE)</script></button>
                  </td>
                </tr>  
        </table>
    </div>
</body>
</html>
