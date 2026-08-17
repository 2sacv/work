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
<script type="text/javascript" src="/js/roisetting.js"></script>
</head>
<body>
    <div class="left">
        <table border=0 cellPadding=0 cellSpacing=0 class="mainTable">
             <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_ROI_SETTING)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
             <tr>
               <td  colspan="2" align="left">
                 <div id="objects" valign="middle"></div>
               </td>
             </tr>
             <tr>
                <td  colspan="2" align="left" >
                 <div style="border:1px solid gray;width:350px;">
                  <table>
                      <tr>
                        <TD>
                            <script>dwn(IDC_GEN_AREA);</script>
                                <select  style="width:65px;" onchange="RoiSelect()" id="selRoi">
                                    <option value="0">1</option>
                                    <option value="1">2</option>
                                    <option value="2">3</option>
                                    <option value="3">4</option>
                                    <option value="4">5</option>
                                    <option value="5">6</option>
                                    <option value="6">7</option>
                                    <option value="7">8</option>
                                </select>
                        </TD>
                      </tr>
                      <tr>
                        <TD>
                            <label for='checkRoi'><script>dwn(IDC_GEN_ENABLE);</script></label>
                            <input type="checkbox" id="checkRoi" onclick="RoiEnableSelect();"/>
                        </TD>
                      </tr>
                      <tr>
                        <TD>
                           <button   class="BtnConfig" onclick="SaveRoi();">
                                            <script>dwn(IDC_SAVE);</script>
                                        </button>
                            <button   class="BtnConfig"  onclick="DelRoi();">
                                            <script>dwn(IDC_DEL);</script>
                                        </button>
                        </TD>
                      </tr>
                  </table>
                  </div>
               </td>
             </tr>
        </table>
    </div>
</body>
</html>