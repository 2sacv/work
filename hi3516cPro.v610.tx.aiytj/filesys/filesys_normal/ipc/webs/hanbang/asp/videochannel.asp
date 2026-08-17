<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/base.css" type="text/css" rel="stylesheet"/>
<link href="/css/form.css" type="text/css" rel="stylesheet"/>
<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/js/videochannel.js"></script>
<style type="text/css">
 input{width:196px;}
 select{width:200px;padding:1px;}
</style>
</head>

<body>
    <div class="left">
        <table border=0 cellPadding=3 cellSpacing=1 class="mainTable">
          <tr><td colspan="3" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_CHANNELSETTING)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="3"></td></tr>
            <tr>
              <td width="30%" class="caption"></td>
              <td width="35%" align="left">
               <label><script>dwn(IDC_STREAM_MASTER);</script></label>
              </td>
              <td width="35%" align="left">
               <label><script>dwn(IDC_STREAM_SLAVE);</script></label>
              </td>
            </tr>
            <tr><td class="hline" colspan="3"></td></tr>
            <!--<tr>
              <td width="30%" class="caption">
                 <script>dwn(IDC_STREAM_ENABLE);</script>
              </td>
              <td width="35%" align="left">
                 <input type="radio" id="m_open" name="rdMaster" onclick="DisableMaster(0);ChgOpen(0);" checked='checked' value='1'>
                 <label for="m_open"><script>dwn(IDC_GEN_SWITCH_OPEN);//开</script></label>
              </td>
              <td width="35%" align="left">
                   <input type="radio" id='s_open' name="rdSlave" value="1" onclick="DisableSlave(0);ChgOpen(1);">
                    <label for="s_open"><script>dwn(IDC_GEN_SWITCH_OPEN);//开</script></label>
                    <input type="radio" id='s_close' name="rdSlave" value="0" onclick="DisableSlave(1);ChgOpen(1);">
                    <label for="s_close"><script>dwn(IDC_GEN_SWITCH_CLOSE);//关</script></label>
              </td>
            </tr>-->
            <tr>
              <td width="30%" class="caption">
                <script>dwn(IDC_STREAM_SELECT);//码流选择</script>
              </td>
              <td width="35%" align="left">
                 <select id="selStreamMaster" class="sysinput" style="width:100px;" onchange="ChgStream(0);">
                    <option value="16">6M</option>
                    <option value="13">5M</option>
                    <option value="12">4M</option>
                    <option value="9">3M</option>
                    <option value="5">1080P</option>
                    <option value="8">960p</option>
                    <option value="3">720P</option>
                    <option value="2">D1</option>
                    <!--<option value="7">VGA</option>-->
                </select>
              </td>
              <td width="35%" align="left">
                    <select id="selStreamSlave" class="sysinput" style="width:100px;" onchange="ChgStream(1)">
                      <!--<option value="2">D1</option>-->
                      <option value="7">VGA</option>
                      <option value="6">QVGA</option>
                    </select>
              </td>
            </tr>
            <tr>
              <td width="30%" class="caption">
                <script>dwn(IDC_FRAMERATE);//帧率</script>
              </td>
              <td width="35%" align="left">
                  <input id="frmrateMaster" type="text" class="sysinput" style="width:100px;"  maxlength="2" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">
                                &nbsp;&nbsp;
                                <font id="frmrateMasterTip">(1~30)</font>
              </td>
              <td width="35%" align="left">
                 <input id="frmrateSlave" type="text" class="sysinput" style="width:100px;" value="10" maxlength="2" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">
                                &nbsp;&nbsp;
                                <font id="frmrateSlaveTip">(1~30)</font>
              </td>
            </tr>
            <tr>
              <td width="30%" class="caption">
                <script>dwn(IDC_BPS);//码率</script>
              </td>
              <td width="35%" align="left">
                  <input id="bitrateMaster" type="text" class="sysinput" style="width:100px;"  maxlength="4" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">
                                &nbsp;&nbsp;
                                <font >
                                    <span id="spanbitrateMaster"></span>
                                </font>
              </td>
              <td width="35%" align="left">
                  <input id="bitrateSlave" type="text" class="sysinput" style="width:100px;" value="384" maxlength="4" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">
                                &nbsp;&nbsp;
                                <font >
                                    <span id="spanbitrateSlave"></span>
                                </font>
              </td>
            </tr>
            <tr>
              <td width="30%" class="caption">
                <script>dwn(IDC_INTERVAL);//帧间隔</script>
              </td>
              <td width="35%" align="left">
                  <input id="frmintrMaster" type="text" class="sysinput" style="width:100px;"  maxlength="3" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">
                                &nbsp;&nbsp;
                                <font >(1~360)</font>
              </td>
              <td width="35%" align="left">
                   <input id="frmintrSlave" type="text" class="sysinput" style="width:100px;" value="50" maxlength="3" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">
                                &nbsp;&nbsp;
                                <font >(1~360)</font>
              </td>
            </tr>
            <tr>
              <td width="30%" class="caption">
                <script>dwn(IDC_BPSCONTROL);//码率控制</script>
              </td>
              <td width="35%" align="left">
                  <select id="selRateCtrlMaster" class="sysinput" style="width:100px">
                                    <option value="1"><script>dwn(IDC_FIXEDRATE);//定码率</script></option>
                                    <option value="0"><script>dwn(IDC_VBR);//变码率</script></option>
                                </select>
              </td>
              <td width="35%" align="left">
                  <select id="selRateCtrlSlave" class="sysinput" style="width:100px">
                                    <option value="1"><script>dwn(IDC_FIXEDRATE);//定码率</script></option>
                                    <option value="0"><script>dwn(IDC_VBR);//变码率</script></option>
                                </select>
              </td>
            </tr>
          
            <tr>
              <td width="30%" class="caption">
                <script>dwn(IDC_COMPRESSTYPE);//压缩格式</script>
              </td>
              <td width="35%" align="left">
                  <select id="selVeEncMaster" class="sysinput" style="width:100px;"  onchange="changeVeEncMaster()">
                                    <option value="2">H264</option>
                                    <option value="7">265+</option>
                                </select>
              </td>
              <td width="35%" align="left">
                  <select id="selVeEncSlave" class="sysinput" style="width:100px;"  onchange="changeVeEncSlave()">
                                    <option value="2">H264</option>
                                    <option value="7">265+</option>
                                </select>
              </td>
            </tr>
			
            <tr><td class="hline" colspan="3"></td></tr>
            <tr>
              <td colspan="3"  align="left">
                  <button  class="BtnConfig" onclick="SaveStream();"><script>dwn(IDC_SAVE)</script></button>
              </td>
            </tr>
        </table>
    </div>
</body>
</html>
