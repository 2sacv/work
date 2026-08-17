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
<script type="text/javascript" src="/js/extendsetting.js"></script>
</head>
<body>
    <div class="left">
        <table border=0 cellPadding=3 cellSpacing=1 class="mainTable">
             <tr><td colspan="3" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_3D_NOISE_REDUCTION)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="3"></td></tr>
              <tr>
                <td width="30%" class="caption"><script>dwn(IDC_NOISE_REDUCTION_ENABLE)</script></td>
                <td align="left" width="60%">
                  <input type="radio" id='denoise_open' name="denoise" value="1">
                    <label for="denoise_open"><script>dwn(IDC_GEN_SWITCH_OPEN);//开</script></label>                   
                    <input type="radio" id='denoise_close' name="denoise" value="0">
                    <label for="denoise_close"><script>dwn(IDC_GEN_SWITCH_CLOSE);//关</script></label>
                </td>
                <td width="10%"></td>
              </tr>
              <tr>
                <td width="30%" class="caption"><script>dwn(IDC_NOISE_REDUCTION_STRENGTH)</script></td>
                <td align="left" width="60%">
                 <div id="StrengthSlider" style="margin-left:0px;margin-top:-5px;width:400px;height:5px;"/>
                </td>
                <td width="10%">
                  <span id="tdStrengthValue" style="margin-left:10px;">0</span>
                </td>
              </tr>
              <tr><td class="hline" colspan="3"></td></tr>
              <tr>
                <td colspan="3"  align="left">
                    <button  class="BtnConfig" onclick="SaveExtCfgNoise();"><script>dwn(IDC_SAVE)</script></button>
                </td>
              </tr>

              <tr><td colspan="3" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_VIDEO_ENCODE_SETTINGS)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="3"></td></tr>

              <tr>
                <td width="30%" class="caption"><script>dwn(IDC_VIDEO_ENCODE_TYPE)</script></td>
                <td align="left" width="60%">
                   <select id="selEncodeSize" style="width:100px;" onchange="changeEncodeSize(this.value)">
                            <option value="6">QVGA</option>
                            <option value="1">CIF</option>
                            <option value="7">VGA</option>
                            <option value="2">D1</option>
                            <option value="3">720P</option>
                            <option value="8">960p</option>
                            <option value="5">1080P</option>
                            <option value="9">3M</option>
                            <option value="12">4M</option>
                            <option value="13">5M</option>
                        </select>
                </td>
                <td width="10%"></td>
              </tr>

              <tr>
                <td width="30%" class="caption"><script>dwn(IDC_VIDEO_ENCODE_CHILD_TYPE)</script></td>
                <td align="left" width="60%">
                   <select id="selChildEncodeType" style="width:100px">
                            <option value="0">HIGH</option>
                            <option value="1">MAIN</option>
                            <option value="2">BASE</option>
                        </select>
                </td>
                <td width="10%"></td>
              </tr>
              <tr><td class="hline" colspan="3"></td></tr>
              <tr>
                <td colspan="3"  align="left">
                    <button  class="BtnConfig" onclick="SaveExtCfgEncode();"><script>dwn(IDC_SAVE)</script></button>
                </td>
              </tr>
			   <tr><td style="height:30px" colspan="3"></td></tr>
			   <tr>
                    <td align="left"  colspan="3">
						 <script>dwn(IDC_HIKVISON_NVR_ENABLE);</script>
						 <input type="radio" id='nvr_open' name="nvroise" value="1" onclick="SaveHikvisonNVR(1);">
                         <label for="nvr_open"><script>dwn(IDC_GEN_SWITCH_OPEN);//开</script></label>                   
                         <input type="radio" id='nvr_close' name="nvroise" value="0" onclick="SaveHikvisonNVR(0);">
                         <label for="nvr_close"><script>dwn(IDC_GEN_SWITCH_CLOSE);//关</script></label>
                         <div style="margin:5px 5px 5px 0px;border:1px solid #666"></div>
                    </td>
                </tr>
            </table>
    </div>
</body>
</html>