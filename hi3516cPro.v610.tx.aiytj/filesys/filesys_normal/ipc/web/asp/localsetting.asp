<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta http-equiv="pragma" content="no-cache" />
<meta http-equiv="Cache-Control" content="no-cache, must-revalidate" />
<meta http-equiv="expires" content="Thu, 1 Jan 1970 00:00:01 GMT" />

<link href="/css/bootstrap.css" type="text/css" rel="stylesheet"/>
<link href="/css/jquery-ui-1.10.4.css" type="text/css" rel="stylesheet"/>
<link href="/css/index.css" type="text/css" rel="stylesheet"/>

<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery-ui-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/jquery/bootstrap-min.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
</head>

<body style="background: #2C2C2C;width:99%;height:100%">
    <div >
      <div id="tabs" class='tabs ui-tabs ui-widget ui-widget-content ui-corner-all' style='width: 100%;'>
        <ul>
          <li><a href="#tabs-1"><script>dwn(IDC_MENU_LOCALSET)</script></a></li>
        </ul>
        <div id="tabs-1">
            <table style='width: 60%;' >
              <tr>
                <td colspan="2">
                   <script>dwn(IDC_RECMEMORY_SETTING);//录像存储设置</script>
                   <div style="margin:5px 5px 5px 0px;border:1px solid #666;"></div>
                </td>
              </tr>   
              <tr>
                  <td height="25" width="150" align="right"><script>dwn(IDC_REC_TIME);//手动录像文件打包时间</script></td>
                  <td height="25" width="350" align="left">
                      <select name="RectimeSel" id="RectimeSel"  style="width:150px" class="sysinput2">
                          <option value="1">1</option>
                          <option value="5">5</option>
                          <option value="10" selected="selected">10</option>
                          <option value="15">15</option>
                          <option value="20">20</option>
                          <option value="25">25</option>
                          <option value="30">30</option>
                          <option value="60">60</option>
                      </select>
                      <font ><script>dwn(IDC_GEN_UNIT_MINUTE);//分</script></font>
                  </td>
              </tr>
              <tr>
                  <td height="25" align="right"><script>dwn(IDC_STORAGE_PATH);//录像/抓拍文件存储目录</script></td>
                  <td height="25" align="left"><input type="text" name="RecPath" id="RecPath" value="D:\IPCamera" style="width:250px;" maxlength="50" class="sysinput2" ></td>
              </tr>
              <tr>
                  <td></td>
                  <td height="25" align="left">
                      <script>dwn(IDC_STORAGE_PATH_TITLE);//非必要情况,请保留默认路径[D:\IPCamera]</script>
                  </td>
              </tr>
              <tr>
                <td colspan="2">
                   <script>dwn(IDC_PRE_REC_SETTING);//预录像设置</script>
                   <div style="margin:5px 5px 5px 0px;border:1px solid #666;"></div>
                </td>
              </tr>  
              <tr>
                  <td height="25" align="right"><script>dwn(IDC_PRE_REC_TIME);//预录像时间</script></td>
                  <td height="25" width="350" align="left"><input type="text" name="PreRecTime" id="PreRecTime" value="5" style="width:150px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" maxlength="2" class="sysinput2">
                    <font >(1~10)<script>dwn(IDC_GEN_UNIT_SECOND);//秒</script></font>
                  </td>
              </tr>
              <tr>
                  <td height="25" align="right"><label for='PreRecCk'><script>dwn(IDC_PRE_REC_ENABLE);//预录像启用</script></label></td>
                  <td height="25" align="left"><input type="checkbox" name="PreRecCk" id="PreRecCk"></td>
              </tr>
              <tr>
                <td colspan="2">
                   <script>dwn(IDC_LINK_REC_SETTING);//报警联动录像(录像文件保存到本地PC)</script>
                   <div style="margin:5px 5px 5px 0px;border:1px solid #666;"></div>
                </td>
              </tr>  
              <tr>
                  <td height="25"  align="right"><script>dwn(IDC_LINK_REC_TIME);//报警录像长度</script></td>
                  <td height="25"  align="left"><input type="text" name="AlarmRecTime" id="AlarmRecTime" value="20" style="width:150px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" maxlength="4" class="sysinput2">
                    <font >(1~3600)<script>dwn(IDC_GEN_UNIT_SECOND);</script></font>
                  </td>
              </tr>
              <tr>
                  <td height="25" align="right"><label for='AlarmRecCk'><script>dwn(IDC_LINK_REC_EN);//报警联动录像</script></label></td>
                  <td height="25" align="left"><input type="checkbox" name="AlarmRecCk" id="AlarmRecCk"></td>
              </tr>
              <tr style="height:15px;">
                <td colspan="2">
                   <div style="margin:5px 5px 5px 0px;border:1px solid #666;"></div>
                </td>
              </tr> 
              <tr>
                <td></td>
                <td align="left">
                    <button style="width:65px;" class="btn btn-inverse btn-black index_btn" onclick="SaveLocalSetting();"><script>dwn(IDC_SAVE);</script></button>
                </td>
              </tr> 

            </table>
        </div>
        
    </div>
</div>
<script type="text/javascript" src="/js/localsetting.js"></script>
</body>
</html>