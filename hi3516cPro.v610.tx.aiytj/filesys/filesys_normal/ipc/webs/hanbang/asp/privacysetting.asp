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
<script type="text/javascript" src="/js/privacysetting.js"></script>
</head>
<body>
    <div class="left">
        <table border=0 cellPadding=0 cellSpacing=0 class="mainTable">
             <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_MENU_PRIVACY_MASK)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
             <tr>
               <td  colspan="2" align="left">
                 <div id="objects" valign="middle"></div>
               </td>
             </tr>
             <tr>
                 <td colspan="2" align="left">
                     <div style="border:1px solid gray;width:350px;">
                         <table>
                                <tr>
                                    <td width="20%" align="left">
                                        <script>dwn(IDC_BLOCKCOLOR);</script>
                                        <select id="selMaskColor"  style="width:190px;" onchange="SetColorChg();">
                                            <option value="0" bgvalue="RGB(8,6,7)" style="background: RGB(8,6,7)"></option>
                                            <option value="1" bgvalue="RGB(32,34,33)" style="background: RGB(32,34,33)"></option>
                                            <option value="2" bgvalue="RGB(61,59,60)" style="background: RGB(61,59,60)"></option>
                                            <option value="3" bgvalue="RGB(88,87,86)" style="background: RGB(88,87,86)"></option>
                                            <option value="4" bgvalue="RGB(113,111,112)" style="background: RGB(113,111,112)"></option>
                                            <option value="5" bgvalue="RGB(139,137,138)" style="background: RGB(139,137,138)"></option>
                                            <option value="6" bgvalue="RGB(169,167,168)" style="background: RGB(169,167,168)"></option>
                                            <option value="7" bgvalue="RGB(195,193,194)"style="background: RGB(195,193,194)"></option>
                                            <option value="8" bgvalue="RGB(178,44,51)"style="background: RGB(178,44,51)"></option>
                                            <option value="9" bgvalue="RGB(24,170,0)" style="background: RGB(24,170,0)"></option>
                                            <option value="10" bgvalue="RGB(18,66,250)"style="background: RGB(18,66,250)"></option>
                                            <option value="11" bgvalue="RGB(33,175,221)" style="background: RGB(33,175,221)"></option>
                                            <option value="12" bgvalue="RGB(189,185,0)" style="background: RGB(189,185,0)"></option>
                                            <option value="13" bgvalue="RGB(180,37,163)" style="background: RGB(180,37,163)"></option>
                                        </select>
                                    </td>
                                </tr>
                                <tr>
                                    <td width="20%" align="left">
                                        <script>dwn(IDC_BLOCKSET_AREA);</script>
                                        <select  style="width:65px;" onchange="AreaMaskSelect()" id="selAreaMask" >
                                            <option value="0">1</option>
                                            <option value="1">2</option>
                                            <option value="2">3</option>
                                            <option value="3">4</option>
                                        </select>
                                    </td>
                                </tr>
                                <tr>
                                    <td>
                                         <label for='checkMask'><script>dwn(IDC_BLOCKSET_EN);</script></label>
                                         <input type="checkbox" id="checkMask" onclick="MaskEnableSelect();"/>
                                    </td>
                                </tr>
                                <tr>
                                    <td width="20%" align="left">
                                        <button   class="BtnConfig" onclick="SaveVideoMask();">
                                            <script>dwn(IDC_SAVE);</script>
                                        </button>
                                        <button   class="BtnConfig"  onclick="DelVideoMask();">
                                            <script>dwn(IDC_DEL);</script>
                                        </button>
                                    </td>
                                </tr>
                            </table>
                     </div>
                 </td>
             </tr>
        </table>
    </div>
</body>
</html>
