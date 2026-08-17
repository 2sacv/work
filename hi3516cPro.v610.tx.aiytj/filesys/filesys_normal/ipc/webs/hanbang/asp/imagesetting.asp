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
<script type="text/javascript" src="/js/jslider.js"></script>
<script type="text/javascript" src="/js/imagesetting.js"></script>
</head>
<body>
    <div class="left">
        <table border=0 cellPadding=0 cellSpacing=0 class="mainTable">
             <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_VIDEOSETTING)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
             <tr>
               <td  colspan="2" align="left">
                 <div id="objects" valign="middle"></div>
               </td>
             </tr>
              <tr><td colspan="2" align="left">
                <div class="spanTab spanTabActive" onclick="showTab(0)" id="divColorParamSet" style="display:none;"><script>dwn(IDC_COLORPARAMETER)</script></div>
                <div class="spanTab" onclick="showTab(1)"  id="divVideoControl" style="display:none;"><script>dwn(IDC_VIDEO_REVERSE)</script></div>
                <div class="spanTab" onclick="showTab(2)"  id="divAdvanceSet" style="display:none;"><script>dwn(IDC_AE)</script></div>
              </td>
            </tr>
            <tr><td colspan="2" align="left" style="border:1px solid gray">
                <table border=0 cellPadding=3 cellSpacing=1 style="width:500px;" id="tbColorParamSet" style="display:none;">
                    <tr  id="trBright">
                        <td width="30%" align="left">
                            <script>dwn(IDC_BRIGHT);</script>
                        </td>
                        <td width="60%" align="left">
                            <div id="bright" style="width:250px;height:8px;">
                                        </div>
                        </td>
                        <td width="10%" align="left" id="tdBright">64</td>
                    </tr>
                    <tr  id="trContrast">
                        <td width="30%" align="left">
                            <script>dwn(IDC_CONTRAST);</script>
                        </td>
                        <td width="60%" align="left">
                            <div id="contrast" style="width:250px;height:8px;">
                                        </div>
                        </td>
                        <td width="10%" align="left" id="tdContrast">64</td>
                    </tr>
                    <tr >
                        <td width="30%" align="left">
                            <script>dwn(IDC_SATURATION);</script>
                        </td>
                        <td width="60%" align="left">
                            <div id="saturation" style="width:250px;height:8px;">
                                        </div>
                        </td>
                        <td width="10%" align="left" id="tdSaturation">64</td>
                    </tr>
                    <tr >
                        <td width="30%" align="left">
                            <script>dwn(IDC_SHRPNESS);</script>
                        </td>
                        <td width="60%" align="left">
                            <div id="sharpness" style="width:250px;height:8px;">
                                        </div>
                        </td>
                        <td width="10%" align="left" id="tdSharpness">64</td>
                    </tr>
                    <tr >
                        <td width="30%" align="left">
                            <script>dwn(IDC_NIGHTLUMA);</script>
                        </td>
                        <td width="60%" align="left">
                            <div id="nightluma" style="width:250px;height:8px;">
                                        </div>
                        </td>
                        <td width="10%" align="left" id="tdNightluma">64</td>
                    </tr>
                    <tr >
                        <td width="30%" align="left">
                            <script>dwn(IDC_HIGH_LIGHT_SUPPRESS);</script>
                        </td>
                        <td width="60%" align="left">
                            <div id="highLightSuppress" style="width:250px;height:8px;">
                                        </div>
                        </td>
                        <td width="10%" align="left" id="tdHighLightSuppress">64</td>
                    </tr>
                    <tr >
                        <td width="30%" align="left">
                            <script>dwn(IDC_GAIN);</script>
                        </td>
                        <td width="60%" align="left">
                            <div id="gain" style="width:250px;height:8px;">
                                        </div>
                        </td>
                        <td width="10%" align="left" id="tdGain">64</td>
                    </tr>
                     <tr><td colspan="3" valign="middle" height="20px" align="left"><b>
                       <script>dwn(IDC_NOTE_GAIN)</script>
                     </b></td></tr>
                     <tr><td class="hline" colspan="3"></td></tr>
                     <tr id="trLamp">
                        <td align="left" colspan="3">
                        <script>dwn(IDC_LAMP);//光源频率</script>
                       
                            <input  type="radio" name="lampchk" onclick="lamchkSave()" value="1" id="lampchkOpen">
                            <label for="lampchkOpen">50Hz</label>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
                            <input type="radio" name="lampchk" onclick="lamchkSave()"  value="0" id="lampchkClose">
                            <label for="lampchkClose">60Hz</label>
                        </td>
                    </tr>
                    <tr>
                        <td align="left" colspan="3">
                           <b><script>dwn(IDC_LAMP_RATE);//光源频率与帧率相关</script></b>
                        </td>
                    </tr>
                    <tr><td class="hline" colspan="3"></td></tr>
                    <tr>
                        <td colspan="3" align="left">
                            <button  class="BtnConfig" onclick="DefaultColor();" ><script>dwn(IDC_DEFAULT)</script></button>
                        </td>
                    </tr> 
                </table>
                <table border=0 cellPadding=3 cellSpacing=1 class="mainTable" id="tbVideoControl" style="display:none;">
                  <tr style="height:20px;">
                      <td align="left">
                         <input type="radio" id='reverse_0' name="videoreverse" value="0" onclick="PreviewReverse()">
                                            <label for="reverse_0"><script>dwn(IDC_VIDEO_NORMAL);//正常</script></label>
                      </td>
                  </tr>
                  <tr style="height:20px;">
                      <td align="left">
                         <input type="radio" id='reverse_1' name="videoreverse" value="1" onclick="PreviewReverse()">
                                            <label for="reverse_1"><script>dwn(IDC_VIDEO_HORIZONTAL);//水平镜像</script></label>
                      </td>
                  </tr>
                  <tr style="height:20px;">
                      <td align="left">
                          <input type="radio" id='reverse_2' name="videoreverse" value="2" onclick="PreviewReverse()">
                                            <label for="reverse_2"><script>dwn(IDC_VIDEO_VERTICAL);//垂直镜像</script></label>
                      </td>
                  </tr>
                  <tr style="height:20px;">
                      <td align="left">
                          <input type="radio" id='reverse_3' name="videoreverse" value="3" onclick="PreviewReverse()">
                                            <label for="reverse_3"><script>dwn(IDC_VIDEO_DIAGONAL);//对角镜像</script></label>
                      </td>
                  </tr>
                </table>
                <table border=0 cellPadding=3 cellSpacing=1 class="mainTable" id="tbAdvanceSet" style="display:none;">
                     <tr>
                        <td align="left" width="35%"><script>dwn(IDC_AES)//电子快门</script>：</td>
                         <td align="left" width="65%">
                            <select id="AESelect" style="width:100px;">
                                <option value="0" selected><script>dwn(IDC_PTZ_AUTO)</script></option>
                                <option value="1">1/10000s</option>
                                <option value="2">1/5000s</option>
                                <option value="3">1/2000s</option>
                                <option value="4">1/1000s</option>
                                <option value="5">1/500s</option>
                                <option value="6">1/250s</option>
                                <option value="7">1/200s</option>
                                <option value="8">1/125s</option>
                                <option value="9">1/100s</option>
                                <option value="10">1/50s</option>
                                <option value="11">1/25s</option>
                            </select>
                        </td>
                    </tr>
                    <tr>
                        <td align="left"><script>dwn(IDC_AWB)</script>：</td>
                         <td align="left">
                            <select id="AWBSelect" onchange="changeAWBSelect();"  style="width:100px;">
                                <option value="0" selected><script>dwn(IDC_PTZ_AUTO)</script></option>
                                <option value="1"><script>dwn(IDC_WEATHER_SUNNY)</script></option>
                                <option value="2"><script>dwn(IDC_WEATHED_CLOUDY)</script></option>
                                <option value="3"><script>dwn(IDC_WEATHED_LAMPS)</script></option>
                                <option value="4"><script>dwn(IDC_WEATHED_DAYLIGHT)</script></option>
                                <option value="5"><script>dwn(IDC_PTZ_indoor)</script></option>
                                <option value="6"><script>dwn(IDC_PTZ_outdoor)</script></option>
                                <option value="7"><script>dwn(IDC_CUSTOM)</script></option>
                            </select>
                        </td>
                    </tr>
                    <tr>
                        <td align="left"><script>dwn(IDC_REDGain)</script>：</td>
                         <td align="left">
                           <input type="text" id="redgain" style="width:80px;" onblur="redgainBlur()"  onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" maxlength="3">&nbsp;(1~255)
                        </td>
                    </tr>
                    <tr>
                        <td align="left"><script>dwn(IDC_BLUEGain)</script>：</td>
                         <td align="left">
                          <input type="text"  id="blurgain" style="width:80px;" onblur="blurgainBlur();"  onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" maxlength="3">&nbsp;(1~255)
                        </td>
                    </tr>
                     <tr>
                          <td align="left"><script>dwn(IDC_LOWLIGHTENHANCE)//低照亮度增強</script>：</td>
                          <td  align="left">
                              <input type="text"  id="lowlightenhance" style="width:80px;" onblur="lowlightenhanceBlur();"  onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" maxlength="3">&nbsp;(0~100)
                          </td>
                      </tr>
<!--
                     <tr>
                          <td align="left"><script>dwn(IDC_STRENGTHEN_TO_MIST)//去雾增強</script>：</td>
                          <td  align="left">
                              <input type="text"  id="strengthenToMist" style="width:80px;" onblur="strengthenToMistBlur();"  onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" maxlength="3">&nbsp;(0~100)
                          </td>
                      </tr>
                      <tr>
                        <td align="left"><script>dwn(IDC_EXPOSURE_AREA_WEIGHT)//曝光区域权重</script>：</td>
                        <td align="left">
                          <select id="aewinweight" style="width:100px;">
                              <option value="0"><script>dwn(IDC_WEIGHT_DEFAULT)</script></option>
                              <option value="2"><script>dwn(IDC_WEIGHT_ABOVE)</script></option>
                              <option value="1"><script>dwn(IDC_WEIGHT_BELOW)</script></option>
                              <option value="3"><script>dwn(IDC_WEIGHT_LEFT)</script></option>
                              <option value="4"><script>dwn(IDC_WEIGHT_RIGHT)</script></option>
                              <option value="5"><script>dwn(IDC_WEIGHT_CENTER)</script></option>
                          </select>
                        </td>
                      </tr>
            					<tr>
2020.3.16注释
-->
            						<td align="left"><script>dwn(IDC_NIGHT_FACE_ENHANCE)//夜视人脸增强</script>：</td>
            						<td align="left">
            							<select id="nightfacemode" style="width:100px;">
            								<option value="0"><script>dwn(IDC_GEN_SWITCH_CLOSE)</script></option>
            								<option value="1"><script>dwn(IDC_NIGHT_FACE_ENHANCE_LEVEL1)</script></option>
            								<option value="2"><script>dwn(IDC_NIGHT_FACE_ENHANCE_LEVEL2)</script></option>
            								<option value="3"><script>dwn(IDC_NIGHT_FACE_ENHANCE_LEVEL3)</script></option>
            							</select>
            						</td>
            					</tr>

                      <tr><td class="hline" colspan="2"></td></tr>
                        <tr>
                          <td colspan="2"  align="left">
                              <button  class="BtnConfig" onclick="AWBSave();"><script>dwn(IDC_SAVE)</script></button>
                          </td>
                      </tr>
                </table>
              </td>
            </tr>
            </table>
    </div>
</body>
</html>