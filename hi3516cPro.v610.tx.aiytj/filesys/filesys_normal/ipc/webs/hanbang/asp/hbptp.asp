<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />

<link href="/css/base.css" type="text/css" rel="stylesheet"/>
<link href="/css/form.css" type="text/css" rel="stylesheet"/>
<link href="/css/jquery.ipaddress.css" type="text/css" rel="stylesheet"/>
<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/jquery/jquery.ipaddress.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/js/aliyun.js"></script>
</head>
<body>
    <div class="left">
            <table border=0 cellPadding=3 cellSpacing=1 class="mainTable">
              <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_DANALE_PLATFORM)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
              <!--<tr>
                  <td width="35%" class="caption">
                    <span><script>dwn(IDC_PPPOE_SWITCH)</script></span>
                  </td>
                  <td align="left" width="65%">
                       <input type="radio" name="aliyun_switch" value="1">
                    <script>dwn(IDC_GEN_SWITCH_OPEN);</script>
                    <input type="radio" name="aliyun_switch" value="0">
                    <script>dwn(IDC_GEN_SWITCH_CLOSE);</script>
                  </td>
              </tr>-->
              <tr>
                  <td width="35%" class="caption">
                    <span><script>dwn(IDC_STATUS_INFO)</script></span>
                  </td>
                  <td align="left" width="65%">
                       <label id="aliyun_status"></label>
                  </td>
              </tr>
              <tr>
                  <td width="35%" class="caption">
                    <span><script>dwn(IDC_DEVID)</script></span>
                  </td>
                  <td align="left" width="65%">
                    <label id="aliyun_devid"></label>
                  </td>
              </tr>
              <tr style="height:10px;">
                  <td width="35%" class="caption">
                  </td>
                  <td align="left" width="65%">
                  </td>
              </tr>
              <tr>
                  <td width="35%" class="caption">
                    <span><script>dwn(IDC_QR_CODE)</script></span>
                  </td>
                  <td align="left" width="65%">
                     <image id="aliyun_path"/>
                  </td>
              </tr>
              <tr style="height:30px;">
                  <td width="35%" class="caption">
                  </td>
                  <td align="left" width="65%">
                  </td>
              </tr>
<!--
              <tr>
                  <td width="35%" class="caption">
                    <span><script>dwn(IDC_HBAPP)</script></span>
                  </td>
                  <td align="left" width="65%">
                     <image src="/image/hbapp.png"/>
                  </td>
              </tr>
-->
              <tr style="height:10px;">
                  <td width="35%" class="caption">
                  </td>
                  <td align="left" width="65%">
                  </td>
              </tr>
			   <!--<tr>
                  <td  colspan="2"  align="left">
                    <span><script>dwn(IDC_HBAPP_TIP)</script></span>
                  </td>
              </tr>-->
              <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td colspan="2"  align="left">
                    <!--<button  class="BtnConfig" onclick="switchAliyun();"><script>dwn(IDC_SAVE)</script></button>-->
                    <button  class="BtnConfig" onclick="initAliyun();"><script>dwn(IDC_REFRESH)</script></button>
                </td>
              </tr>
            </table>  
    </div>
</body>
</html>
