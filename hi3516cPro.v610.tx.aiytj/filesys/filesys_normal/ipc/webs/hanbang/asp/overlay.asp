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
<script type="text/javascript" src="/js/overlay.js"></script>
</head>
<body>
    <div class="left">
        <table border=0 cellPadding=0 cellSpacing=0 class="mainTable">
             <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_SUBTITLE_OVERLAY)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
             <tr>
               <td  colspan="2" align="left">
                 <div id="objects" valign="middle"></div>
               </td>
             </tr>
              <tr><td colspan="2" align="left">
                <div class="spanTab spanTabActive" onclick="showTab(0)" id="divBasic"><script>dwn(IDC_NAME_ENABLE)//基础字幕</script></div>
                <div class="spanTab" onclick="showTab(1)"  id="divExtend"><script>dwn(IDC_NAME_DIEJIA)//扩展字幕</script></div>
              </td>
            </tr>
            <tr><td colspan="2" align="left" style="border:1px solid gray">
                <table border=0 cellPadding=3 cellSpacing=1 style="width:600px;" id="tbBasic">
                  <tr>
                        <td width="20%" align="left">
                            <input type="radio" id='name_enb' name="namechk" value="1" onclick="SetOSDPosition(0);">
                            <label for="name_enb"><script>dwn(IDC_SHOWNAME);//显示</script></label>
                        </td>
                        <td width="20%" align="left">
                            <input type="radio" id='name_dis' name="namechk" value="0" onclick="SetOSDPosition(0);">
                            <label for="name_dis"><script>dwn(IDC_HIDENAME);//隐藏</script></label>
                        </td>
                        <td align="right" width="20%">
                            <font style="font-size:10pt;"><script>dwn(IDC_NAME)//名称</script></font>
                        </td>
                        <td align="left">
                            <input type="text" id="name" class="sysinput" style="width:180px;" maxlength="36">
                            <font><script>dwn(IDC_NAME_CONTENT_36);//(1~36)个字符</script></font>
                        </td>
                    </tr>
                    <tr>
                        <td align="left">
                            <input type="radio" id='time_enb' name="timechk" value="1" onclick="SetOSDPosition(1);">
                            <label for="time_enb"><script>dwn(IDC_SHOWTIME);//显示时间</script></label>
                        </td>
                        <td align="left">
                            <input type="radio" id='time_dis' name="timechk" value="0" onclick="SetOSDPosition(1);">
                            <label for="time_dis"><script>dwn(IDC_HIDETIME);//隐藏时间</script></label>
                        </td>
                        <td align="right">
                            <font style="font-size:10pt;"><script>dwn(IDC_COLORSETTING);//颜色设置</script>：</font>
                        </td>
                        <td  align="left">
                            <select class="sysinput" style="width:110px;" id="colorOption">
                                <option value="1"><script>dwn(IDC_OSD_COLOR1)</script></option>
                                <option value="2"><script>dwn(IDC_OSD_COLOR2)</script></option>
                                <option value="0"><script>dwn(IDC_HDR_AUTO)</script></option>
                            </select>
                        </td>
                    </tr>
                    <tr>
                        <td width="20%" align="left">
                            <input type="radio" id='osd_enb' name="osdchk" value="1" onclick="SetOSDPosition(3);">
                            <label for="osd_enb" style="font-size:10pt;"><script>dwn(IDC_SHOWOSD);//显示</script></label>
                        </td>
                        <td width="20%" align="left">
                            <input type="radio" id='osd_dis' name="osdchk" value="0" onclick="SetOSDPosition(3);">
                            <label for="osd_dis" style="font-size:10pt;"><script>dwn(IDC_HIDEOSD);//隐藏</script></label>
                        </td>
                        <td align="right" width="20%">
                            <font style="font-size:10pt;"><script>dwn(IDC_NAME)//名称</script></font>
                        </td>
                        <td align="left">
                            <input type="text" id="osdname" class="sysinput" style="width:110px;" maxlength="12">
                            <font><script>dwn(IDC_NAME_CONTENT);//(1~12)个字符</script></font>
                        </td>
                    </tr>
                    <tr>
                        <td align="left">
                            <input type="radio" id='bps_enb' name="bpschk" value="1" onclick="SetOSDPosition(2);">
                            <label for="bps_enb"><script>dwn(IDC_SHOWBPS);//显示码率</script></label>
                        </td>
                        <td align="left">
                            <input type="radio" id='bps_dis' name="bpschk" value="0" onclick="SetOSDPosition(2);">
                            <label for="bps_dis"><script>dwn(IDC_HIDEBPS);//隐藏码率</script></label>
                        </td>
                        <td align="right" width="20%">
                            <font style="font-size:10pt;"><script>dwn(IDC_OSD_FONT)//OSD字体</script></font>
                        </td>
                        <td align="left">
                             <select class="sysinput" style="width:110px;" id="osd_font">
                                <option value="2"><script>dwn(IDC_FONT_BIG)</script></option>
                                <option value="1"><script>dwn(IDC_FONT_MIDDLE)</script></option>
                                <option value="0"><script>dwn(IDC_FONT_SMALL)</script></option>
                            </select>
                        </td>
                    </tr>
                    <tr>
                        <td width="20%" align="left">
                            <input type="radio" id='week_enb' name="weekchk" value="1">
                            <label for="week_enb"><script>dwn(IDC_SHOWWEEK);//显示星期</script></label>
                        </td>
                        <td width="20%" align="left">
                            <input type="radio" id='week_dis' name="weekchk" value="0">
                            <label for="week_dis"><script>dwn(IDC_HIDEWEEK);//隐藏星期</script></label>
                        </td>
                        <td align="right" width="20%">
                            <font style="font-size:10pt;"><script>dwn(IDC_DATEFORMAT)//日期格式</script></font>
                        </td>
                        <td align="left">
                            <select id="dateformat" class="sysinput" style="width:160px;">
                                <option value="2"><script>dwn(IDC_DATE_2)</script></option>
                                <option value="3"><script>dwn(IDC_DATE_3)</script></option>
                                <option value="4"><script>dwn(IDC_DATE_4)</script></option>
                                <option value="5"><script>dwn(IDC_DATE_5)</script></option>
                                <option value="7"><script>dwn(IDC_DATE_7)</script></option>
                                <option value="8"><script>dwn(IDC_DATE_8)</script></option>
                        </td>
                    </tr>
                    <tr>
                        <td colspan="4" align="left">
                            <button  class="BtnConfig" onclick="SaveBasic();" ><script>dwn(IDC_SAVE)</script></button>
                        </td>
                    </tr> 
                </table>
                <table border=0 cellPadding=3 cellSpacing=1 style="width:500px;" id="tbExtend" style="display:none;">
                  <tr>
                        <td width="30" align="left">
                            <input type="checkbox" id="check_1"/>
                        </td>
                        <td align="left">X:
                            <input type="text" id="xpos_1" class="sysinput2" style="width:45px;" maxlength="5"/>
                        </td>
                        <td align="right"  style="display:none;">Y:
                            <input type="text" id="ypos_1"  style="display:none;" class="sysinput2" style="width:45px;" maxlength="5"/>
                        </td>
                        <td align="center"><script>dwn(IDC_OSD_NAME);//字符</script>
                            <input type="text" id="osd_1" class="sysinput2" style="width:120px;" maxlength="12" />
                        </td>
                        <td align="center">
                            <button class="btn btn-inverse btn-black button_test index_btn"  style="width:65px;margin:0px 10px 0px 10px;" onclick="SaveOsdStr(1)"><script>dwn(IDC_SAVE);//保存</script></button>
                        </td>
                    </tr>
                    <tr>
                        <td width="30" align="left">
                            <input type="checkbox" id="check_2"/>
                        </td>
                        <td align="left">X:
                            <input type="text" id="xpos_2" class="sysinput2" style="width:45px;" maxlength="5">
                        </td>
                        <td align="right"  style="display:none;">Y:
                            <input type="text" id="ypos_2"  style="display:none;" class="sysinput2" style="width:45px;" maxlength="5">
                        </td>
                        <td align="center"><script>dwn(IDC_OSD_NAME);//字符</script>
                            <input type="text" id="osd_2" class="sysinput2" style="width:120px;" maxlength="12" >
                        </td>
                        <td align="center">
                            <button class="btn btn-inverse btn-black button_test index_btn" style="width:65px;margin:0px 10px 0px 10px;" onclick="SaveOsdStr(2)"><script>dwn(IDC_SAVE);//保存</script></button>
                        </td>
                    </tr>
                    <tr>
                        <td width="30" align="left">
                            <input type="checkbox" id="check_3"/>
                        </td>
                        <td align="left">X:
                            <input type="text" id="xpos_3" class="sysinput2" style="width:45px;" maxlength="5">
                        </td>
                        <td align="right"  style="display:none;">Y:
                            <input type="text" id="ypos_3"  style="display:none;" class="sysinput2" style="width:45px;" maxlength="5">
                        </td>
                        <td align="center"><script>dwn(IDC_OSD_NAME);//字符</script>
                            <input type="text" id="osd_3" class="sysinput2" style="width:120px;" maxlength="12" >
                        </td>
                        <td align="center">
                            <button class="btn btn-inverse btn-black button_test index_btn" style="width:65px;margin:0px 10px 0px 10px;" onclick="SaveOsdStr(3)"><script>dwn(IDC_SAVE);//保存</script></button>
                        </td>
                    </tr>
                    <tr style="display:none;">
                        <td width="30" align="left">
                            <input type="checkbox" id="check_4">
                        </td>
                        <td align="left">X:
                            <input type="text" id="xpos_4" class="sysinput2" style="width:45px;" maxlength="5">
                        </td>
                        <td align="right">Y:
                            <input type="text" id="ypos_4" class="sysinput2" style="width:45px;" maxlength="5">
                        </td>
                        <td align="center"><script>dwn(IDC_OSD_NAME);//字符</script>
                            <input type="text" id="osd_4" class="sysinput2" style="width:120px;"maxlength="12" >
                        </td>
                        <td align="center">
                            <button class="btn btn-inverse btn-black button_test index_btn" style="width:65px;margin:0px 10px 0px 10px;" onclick="SaveOsdStr(4)"><script>dwn(IDC_SAVE);//保存</script></button>
                        </td>
                    </tr>
                    <tr style="display:none;">
                        <td width="30" align="left">
                            <input type="checkbox" id="check_5">
                        </td>
                        <td align="left">X:
                            <input type="text" id="xpos_5" class="sysinput2" style="width:45px;" maxlength="5">
                        </td>
                        <td align="right">Y:
                            <input type="text" id="ypos_5" class="sysinput2" style="width:45px;" maxlength="5">
                        </td>
                        <td align="center"><script>dwn(IDC_OSD_NAME);//字符</script>
                            <input type="text" id="osd_5" class="sysinput2" style="width:120px;"maxlength="12" >
                        </td>
                        <td align="center">
                            <button class="btn btn-inverse btn-black button_test index_btn" style="width:65px;margin:0px 10px 0px 10px;" onclick="SaveOsdStr(5)"><script>dwn(IDC_SAVE);//保存</script></button>
                        </td>
                    </tr>
                    <tr style="display:none;">
                        <td width="30" align="left">
                            <input type="checkbox" id="check_6">
                        </td>
                        <td align="left">X:
                            <input type="text" id="xpos_6" class="sysinput2" style="width:45px;" maxlength="5">
                        </td>
                        <td align="right">Y:
                            <input type="text" id="ypos_6" class="sysinput2" style="width:45px;" maxlength="5">
                        </td>
                        <td align="center"><script>dwn(IDC_OSD_NAME);//字符</script>
                            <input type="text" id="osd_6" class="sysinput2" style="width:120px;"maxlength="12" >
                        </td>
                        <td align="center">
                            <button class="btn btn-inverse btn-black button_test index_btn" style="width:65px;margin:0px 10px 0px 10px;" onclick="SaveOsdStr(6)"><script>dwn(IDC_SAVE);//保存</script></button>
                        </td>
                    </tr>
                    <tr style="display:none;">
                        <td width="30" align="left">
                            <input type="checkbox" id="check_7">
                        </td>
                        <td align="left">X:
                            <input type="text" id="xpos_7" class="sysinput2" style="width:45px;" maxlength="5">
                        </td>
                        <td align="right">Y:
                            <input type="text" id="ypos_7" class="sysinput2" style="width:45px;" maxlength="5">
                        </td>
                        <td align="center"><script>dwn(IDC_OSD_NAME);//字符</script>
                            <input type="text" id="osd_7" class="sysinput2" style="width:120px;"maxlength="12" >
                        </td>
                        <td align="center">
                            <button class="btn btn-inverse btn-black button_test index_btn" style="width:65px;margin:0px 10px 0px 10px;" onclick="SaveOsdStr(7)"><script>dwn(IDC_SAVE);//保存</script></button>
                        </td>
                    </tr>
                    <tr>
                        <td colspan="5" align="center">
                            <font >
                                (<script>dwn(IDC_X+"1919,"+IDC_Y+"1079")</script>)
                            </font>
                        </td>
                    </tr>
                </table>
              </td>
            </tr>
            </table>
    </div>
</body>
</html>
