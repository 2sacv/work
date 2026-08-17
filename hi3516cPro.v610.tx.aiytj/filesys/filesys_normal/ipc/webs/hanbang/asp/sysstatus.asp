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
<script type="text/javascript" src="/js/sysstatus.js"></script>
<style type="text/css">
  #tbStatus tr:hover{background:rgb(49,106,197)}
</style>
</head>
<body>
    <div class="left">
        <table border=0 cellPadding=3 cellSpacing=1 class="mainTable">
             <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_SYSTEMSTATUS)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td colspan="2"  align="left">
                    <script>dwn(IDC_FLUSH_INTERVAL);</script>
                    <select id="sysStatusTime" style="width:100px;">
                      <option value="5">5</option>
                      <option value="10">10</option>
                      <option value="15">15</option>
                      <option value="20">20</option>
                      <option value="25">25</option>
                      <option value="30">30</option>
                      <option value="35">35</option>
                      <option value="40">40</option>
                      <option value="45">45</option>
                      <option value="50">50</option>
                      <option value="55">55</option>
                      <option value="60">60</option>
                    </select>
                      <label for='sysStatusEn'><script>dwn(IDC_AUTO_ENABLE);//自动刷新</script></label>
                      <input type="checkbox" id="sysStatusEn" onclick="checkSysStatus()"/>
                       <button  class="BtnConfig" onclick="FlushStatus();"><script>dwn(IDC_FLUSH)</script></button>
                </td>
              </tr>
              <tr>
                <td colspan="2"  align="left"  style="border:1px solid gray;">
                    <table id="tbStatus" name="tbStatus"  cellpadding="0" cellspacing="0"  border="1" width="100%" align="left" >
                          <tr style="height:25px;background:rgb(235,234,219)">
                              <td  width="50px"  align="center"><script>dwn(IDC_SEQUENCE);//序号</script></td>
                              <td  width="550px" align="center"><script>dwn(IDC_STATUSINFO);//信息</script></td>
                          </tr>
                      </table>
                </td>
              </tr>
            </table>
    </div>
</body>
</html>