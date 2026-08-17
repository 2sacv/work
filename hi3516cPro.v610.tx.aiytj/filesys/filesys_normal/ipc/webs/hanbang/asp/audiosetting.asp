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
<script type="text/javascript" src="/js/audiosetting.js"></script>
</head>
<body>
    <div class="left">
        <table border=0 cellPadding=3 cellSpacing=1 class="mainTable">
             <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_AUDIOSET)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td width="30%" class="caption">
                <script>dwn(IDC_AUDIOINPUTENABLE);//音频输入开关</script>
                </td>
                <td align="left" width="60%">
                  <input type="radio" name="rdAudioIN" value="1" onclick="SetDisableEnable(1)">
                            <script>dwn(IDC_GEN_SWITCH_OPEN);//开</script>
                            <input type="radio" name="rdAudioIN" value="0" onclick="SetDisableEnable(0)">
                            <script>dwn(IDC_GEN_SWITCH_CLOSE);//关</script>
                </td>
                <td width="10%" align="right"></td>
              </tr>
              <tr>
                <td width="30%" class="caption">
                <script>dwn(IDC_AUDIO_INPUTTYPE);</script>
                </td>
                <td align="left" width="60%">
                  <select id="inputtype" style="width:80px;margin-left:0px;" >
                                <option value="0">Mic</option>
                                <option value="1">Line-In</option>
                            </select>
                </td>
                <td width="10%" align="right"></td>
              </tr>
              <tr>
                <td width="30%" class="caption">
                <script>dwn(IDC_AUDIO_CODETYPE);</script>
                </td>
                <td align="left" width="60%">
                  <select id="codetype" style="width:80px;">
                                <option value="1">G711A</option>
                                <option value="2">G711U</option>
                            </select>
                </td>
                <td width="10%" align="right"></td>
              </tr>
              <tr  style="display:none">
                <td width="30%" class="caption">
                <script>dwn(IDC_BPS);</script>
                </td>
                <td align="left" width="60%">
                 <select id="amrbps" style="width:80px;">
                                <option value="0">4.75K</option>
                                <option value="1">5.15K</option>
                                <option value="2">5.9K</option>
                                <option value="3">6.7K</option>
                                <option value="4">7.4K</option>
                                <option value="5">7.95K</option>
                                <option value="6">10.2K</option>
                                <option value="7">12.2K</option>
                            </select>&nbsp;bps
                </td>
                <td width="10%" align="right"></td>
              </tr>
              <tr>
                <td width="30%" class="caption">
                <script>dwn(IDC_AUDIOINPUT);</script>
                </td>
                <td align="left" width="60%">
                    <div id="divInvolume" style="display:none;"></div>
                    <div id="involume" style="width:300px;height:5px;"/>
                </td>
                <td width="10%" align="left" id="tdInvolume">50</td>
              </tr>
              <tr>
                <td width="30%" class="caption">
                <script>dwn(IDC_AUDIOOUTPUT);</script>
                </td>
                <td align="left" width="60%">
                    <div id="divOutvolume" style="display:none;"></div>
                    <div id="outvolume" style="width:300px;height:5px;"/>
                </td>
                <td width="10%" align="left" id="tdOutvolume">50</td>
              </tr>
              <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td colspan="3"  align="left">
                    <button  class="BtnConfig" onclick="DefauleVolume();"><script>dwn(IDC_DEFAULT)</script></button>
                    <button  class="BtnConfig" onclick="SaveAudioSet();"><script>dwn(IDC_SAVE)</script></button>
                </td>
              </tr>
            </table>
    </div>
</body>
</html>