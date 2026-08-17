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
<script type="text/javascript" src="/js/sysupdate.js"></script>
</head> 
<body>
    <div class="left">
        <iframe id='upiframe' name="aa" style='display:none;'></iframe>
        <form action="" target="aa" method="post" enctype="multipart/form-data" id="frmUpdate" name='frmUpdate'>
        <table border=0 cellPadding=3 cellSpacing=1 class="mainTable">
             <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_SYSUPDATE)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td width="20%" class="caption"><script>dwn(IDC_UPDATE_FILE_PATH)</script></td>
                <td align="left" width="80%">
                  <input name="filepath" type="file" id="filepath" style='width: 400px;' onchange="changeFilepath();"/>
                
                  <button  class="BtnConfig" onclick="IframeUpdate();"><script>dwn(IDC_CONFIRM)</script></button>
                </td>
              </tr>
              <tr style="display:none;" id="trFileName">
                <td width="20%" class="caption"><script>dwn(IDC_FILE_NAME)</script></td>
                <td align="left">
                  <span id="fileName"></span>
                </td>
              </tr>
               <tr>
                  <td colspan="2"  align="left">
                    <div  id='progress_div' style='display: none;'>
                      <div id="progress" style="width:580px;float: left;height:15px;"></div>
                      <span id="progress_lab" style='float: left;margin-left: 15px;'>0%</span>
                    </div>
                  </td>
                </tr>
                 <tr>
                  <td colspan="2"  align="left">
                     <div style='margin-left: 5px;display: none;' id='restart_prompt_div'>
                        <span id='restart_prompt' style="font-size:12px; color:red;"><script>dwn(IDC_UPDATE_WAIT);</script></span>
                      </div>
                  </td>
                </tr>
            </table>
    </div>
</body>
</html>