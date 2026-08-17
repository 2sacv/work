<!DOCTYPE html
  PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>

<head>
  <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
  <meta http-equiv="pragma" content="no-cache" />
  <meta http-equiv="Cache-Control" content="no-cache, must-revalidate" />
  <meta http-equiv="expires" content="Thu, 1 Jan 1970 00:00:01 GMT" />

  <link href="/css/bootstrap.css" type="text/css" rel="stylesheet" />
  <link href="/css/jquery-ui-1.10.4.css" type="text/css" rel="stylesheet" />
  <link href="/css/index.css" type="text/css" rel="stylesheet" />

  <script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
  <script type="text/javascript" src="/jquery/jquery-ui-1.10.4.custom-min.js"></script>
  <script type="text/javascript" src="/jquery/jquery-ui-min.js"></script>

  <script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
  <script type="text/javascript" src="/jquery/bootstrap-min.js"></script>
  <script type="text/javascript" src="/js/public_function.js"></script>
  <script type="text/javascript" src="/js/jcpcmd.js"></script>
  <script type="text/javascript" src="/js/base64.js"></script>
  <script type="text/javascript" src="/js/audiovideo.js"></script>
  <style type="text/css">
    #colorSetTb tr {
      height: 37px;
    }
  </style>
</head>

<body style="background: #2C2C2C;width:99%;height:100%">
  <div style='background: #3C3D3D;'>
    <div id="tabs" class='tabs ui-tabs ui-widget ui-widget-content ui-corner-all' style='width: 100%;'>
      <ul>
        <li><a href="#tabs-1" id="tab_videochannel">
            <script>dwn(IDC_CHANNELSETTING);//视频通道</script>
          </a></li>
        <li><a href="#tabs-2" id="tab_videoparams">
            <script>dwn(IDC_SUBTITLE_OVERLAY);//字幕叠加</script>
          </a></li>
        <li><a href="#tabs-2" id="tab_colorsetting">
            <script>dwn(IDC_VIDEOSETTING);//图像设置</script>
          </a></li>
        <li id="liPrivacyMask" style="display:none;"><a href="#tabs-2" id="tab_videomask">
            <script>dwn(IDC_MENU_PRIVACY_MASK);//隐私遮挡</script>
          </a></li>
        <li><a href="#tabs-2" style="display:none;" id="tab_roisetting">
            <script>dwn(IDC_ROI_SETTING);//ROI设置</script>
          </a></li>
        <li id="liAudioSet" style="display:none;"><a href="#tabs-3" id="tab_audioset">
            <script>dwn(IDC_AUDIOSET);//音频设置</script>
          </a></li>
        <li id="liInfraredSet"><a href="#tabs-4" id="tab_infraredsetting">
            <script>dwn(IDC_FILL_LIGHT_SETTING);//补光设置</script>
          </a></li>
        <li><a href="#tabs-5" id="tab_entendconfig">
            <script>dwn(IDC_EXTEND_DISPOSE);//扩展设置</script>
          </a></li>
      </ul>

      <div id="tabs-1">
        <table style='width: 90%;margin-left: 30px;'>
          <tr>
            <td>
              <table style='width: 80%;'>
                <tr>
                  <td colspan="2" align="center">
                    <script>dwn(IDC_STREAM_MASTER);//主码流</script>
                    <div style="margin:5px 5px 5px 0px;border:1px solid #666"></div>
                  </td>
                </tr>
                <tr>
                  <td height="25" width="200" align="right">
                    <script>dwn(IDC_STREAM_MASTER_ENABLE);//主码流开关</script>
                  </td>
                  <td height="25" width="300" align="left">&nbsp;&nbsp;
                    <input type="radio" id="m_open" name="rdMaster" onclick="DisableMaster(0);ChgOpen(0);"
                      checked='checked' value='1'>
                    <label for="m_open">
                      <script>dwn(IDC_GEN_SWITCH_OPEN);//开</script>
                    </label>
                    <!--  <input type="radio" id="m_close" name="rdMaster" onclick="DisableMaster(1);ChgOpen(0);" value='0' disabled="disabled">
                                <label for="m_close"><script>dwn(IDC_GEN_SWITCH_CLOSE);//关</script></label> -->
                  </td>
                </tr>
                <tr>
                  <td height="25" align="right">
                    <script>dwn(IDC_STREAM_SELECT);//码流选择</script>
                  </td>
                  <td height="25" align="left">
                    <select id="selStreamMaster" class="sysinput" style="width:100px;" onchange="ChgStream(0);">
                      <option value="15">8M</option>
                      <option value="13">5M</option>
                      <option value="12">4M</option>
                      <option value="9">3M</option>
                      <option value="5">1080P</option>
                      <option value="8">960p</option>
                      <option value="3">720P</option>
                      <option value="2">D1</option>
                      <option value="7">VGA</option>
                    </select>
                  </td>
                </tr>
                <tr height='25'>
                  <td align="right">
                    <script>dwn(IDC_FRAMERATE);//帧率</script>
                  </td>
                  <td width="300" align="left">
                    <input id="frmrateMaster" type="text" class="sysinput" style="width:100px;" maxlength="2"
                      onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">
                    &nbsp;&nbsp;
                    <font id="frmrateMasterTip"></font>
                  </td>
                </tr>
                <tr height='25'>
                  <td align="right">
                    <script>dwn(IDC_BPS);//码率</script>
                  </td>
                  <td align="left">
                    <input id="bitrateMaster" type="text" class="sysinput" style="width:100px;" maxlength="4"
                      onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">
                    &nbsp;&nbsp;
                    <font>
                      <span id="spanbitrateMaster"></span>
                    </font>
                  </td>
                </tr>
                <tr height="25">
                  <td align="right">
                    <script>dwn(IDC_INTERVAL);//帧间隔</script>
                  </td>
                  <td align="left" width="200px">
                    <input id="frmintrMaster" type="text" class="sysinput" style="width:100px;" maxlength="3"
                      onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">
                    &nbsp;&nbsp;
                    <font id="spanfrmintrMaster"></font>
                  </td>
                </tr>
                <tr height="25">
                  <td align="right">
                    <script>dwn(IDC_BPSCONTROL);//码率控制</script>
                  </td>
                  <td align="left">
                    <select id="selRateCtrlMaster" class="sysinput" style="width:100px">
                      <option value="1">
                        <script>dwn(IDC_FIXEDRATE);//定码率</script>
                      </option>
                      <option value="0">
                        <script>dwn(IDC_VBR);//变码率</script>
                      </option>
                    </select>
                  </td>
                </tr>
                <!--  <tr>
                            <td height="25" align="right">
                                <script>dwn(IDC_CODEMODEL);//编码模式</script></td>
                            <td height="25" align="left">
                                &nbsp;
                                <select id="selFirstMaster" class="sysinput" style="width:100px">
                                    <option value="0" selected="selected"><script>dwn(IDC_QUALITYFIRST);//质量优先</script></option>
                                    <option value="1"><script>dwn(IDC_RATEFIRST);//码率优先</script></option>
                                </select>
                            </td>
                        </tr> -->

                <tr height="25">
                  <td align="right">
                    <script>dwn(IDC_COMPRESSTYPE);//压缩格式</script>
                  </td>
                  <td align="left">
                    <select id="selVeEncMaster" class="sysinput" style="width:100px;" onchange="changeVeEncMaster()">
                      <option value="2">H264</option>
                      <option value="7">265+</option>
                    </select>
                  </td>
                </tr>
              </table>
            </td>
            <td>
              <table style='width: 80%;margin-left: 10px;'>
                <tr>
                  <td colspan="2" align="center">
                    <script>dwn(IDC_STREAM_SLAVE);//从码流</script>
                    <div style="margin:5px 5px 5px 0px;border:1px solid #666"></div>
                  </td>
                </tr>
                <tr height="25">
                  <td width="200" align="right">
                    <script>dwn(IDC_STREAM_SLAVE_ENABLE);//从码流开关</script>
                  </td>
                  <td width="300" align="left">
                    <input type="radio" id='s_open' name="rdSlave" value="1" style='margin-left: 25px;'
                      onclick="DisableSlave(0);ChgOpen(1);">
                    <label for="s_open">
                      <script>dwn(IDC_GEN_SWITCH_OPEN);//开</script>
                    </label>
                    <!--    
                                <input type="radio" id='s_close' name="rdSlave" value="0" onclick="DisableSlave(1);ChgOpen(1);">
                                <label for="s_close"><script>dwn(IDC_GEN_SWITCH_CLOSE);//关</script></label>
                            -->
                  </td>
                </tr>
                <tr height="25">
                  <td align="right">
                    <script>dwn(IDC_STREAM_SELECT);//码流选择</script>
                  </td>
                  <td align="left">
                    &nbsp;
                    <select id="selStreamSlave" class="sysinput" style="width:100px;" onchange="ChgStream(1)">
                      <option value="2">D1</option>
                      <option value="7">VGA</option>
                      <option value="1">CIF</option>
                      <option value="6">QVGA</option>
                      <option value="11">360P</option>
                    </select>
                  </td>
                </tr>
                <tr height="25">
                  <td align="right">
                    <script>dwn(IDC_FRAMERATE);//帧率</script>
                  </td>
                  <td align="left">
                    &nbsp;
                    <input id="frmrateSlave" type="text" class="sysinput" style="width:100px;" value="10" maxlength="2"
                      onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">
                    &nbsp;&nbsp;
                    <font id="frmrateSlaveTip"></font>
                  </td>
                </tr>
                <tr height="25">
                  <td align="right">
                    <script>dwn(IDC_BPS);//码率</script>
                  </td>
                  <td align="left">
                    &nbsp;
                    <input id="bitrateSlave" type="text" class="sysinput" style="width:100px;" value="384" maxlength="4"
                      onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">
                    &nbsp;&nbsp;
                    <font>
                      <span id="spanbitrateSlave"></span>
                    </font>
                  </td>
                </tr>
                <tr height="25">
                  <td align="right">
                    <script>dwn(IDC_INTERVAL);//帧间隔</script>
                  </td>
                  <td align="left">
                    &nbsp;
                    <input id="frmintrSlave" type="text" class="sysinput" style="width:100px;" value="50" maxlength="3"
                      onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">
                    &nbsp;&nbsp;
                    <font id="spanfrmintrSlave"></font>
                  </td>
                </tr>
                <tr height="25">
                  <td align="right">
                    <script>dwn(IDC_BPSCONTROL);//码率控制</script>
                  </td>
                  <td align="left">
                    &nbsp;
                    <select id="selRateCtrlSlave" class="sysinput" style="width:100px">
                      <option value="1">
                        <script>dwn(IDC_FIXEDRATE);//定码率</script>
                      </option>
                      <option value="0">
                        <script>dwn(IDC_VBR);//变码率</script>
                      </option>
                    </select>
                  </td>
                </tr>
                <!-- <tr>
                            <td height="25" align="right">
                                <script>dwn(IDC_CODEMODEL);//编码模式</script></td>
                            <td height="25" align="left">
                                &nbsp;
                                <select id="selFirstSlave" class="sysinput" style="width:100px">
                                    <option value="0" selected="selected"><script>dwn(IDC_QUALITYFIRST);//质量优先</script></option>
                                    <option value="1"><script>dwn(IDC_RATEFIRST);//码率优先</script></option>
                                </select>
                            </td>
                        </tr> -->
                <tr height="25">
                  <td align="right">
                    <script>dwn(IDC_COMPRESSTYPE);//压缩格式</script>
                  </td>
                  <td align="left">&nbsp;
                    <select id="selVeEncSlave" class="sysinput" style="width:100px;" onchange="changeVeEncSlave()">
                      <option value="2">H264</option>
                      <option value="7">265+</option>
                    </select>
                  </td>
                </tr>
              </table>
            </td>
          </tr>
          <tr>
            <td colspan="2" align="center">
              <button style="width:65px;margin-right: 120px;" class="btn btn-inverse btn-black button_test index_btn"
                onclick="SaveStream();">
                <script>dwn(IDC_SAVE);</script>
              </button>
            </td>
          </tr>
        </table>
      </div>
      <div id="tabs-2">
        <table style='width: 99%;margin-left: 10px;'>
          <tr>
            <td style="width:50%;">
              <div id="objects" align="center" valign="middle"></div>
            </td>
            <td style="width:60px;">
              &nbsp;
            </td>
            <td style="width:45%;">
              <div id="subtabs_mask" class='tabs ui-tabs ui-widget ui-widget-content ui-corner-all'
                style='width: 500px;height:470px;display:none'>
                <ul>
                  <li><a href="#subtabs_mask-1">
                      <script>dwn(IDC_MENU_PRIVACY_MASK)</script>
                    </a></li>
                </ul>
                <div id="subtabs_mask-1">
                  <table width="100%" style="margin-top:0px;" border="0" id="tbDome">
                    <tr>
                      <td align="right">
                        <script>dwn(IDC_BLOCKCOLOR); //遮挡颜色</script>
                      </td>
                      <td>
                        <select id="ShieldingColor" style="width:50px;" onchange="colorChange()" class="sysinput2">
                          <option value="1">1</option>
                        </select>
                      </td>
                      <td></td>
                      <td colspan="4" rowspan="3">
                        <table cellpadding="0" cellspacing="0" border="0">
                          <tr height="23px">
                            <td align="center">
                              <img hidefocus="true" id="direction_bar" src="/image/ptz/ptz.png"
                                usemap="#direction_bar_map" /><map id="direction_bar_map" name="direction_bar_map">
                                <area coords="45,7,69,54,92,54,116,9,79,2" data-direction="up" href="#"
                                  shape="poly" /><area coords="70,108,47,152,82,162,115,153,93,108"
                                  data-direction="down" href="#" shape="poly" />
                                <area coords="10,44,52,68,53,93,11,118,1,82" data-direction="left" href="#"
                                  shape="poly" />
                                <area coords="108,69,110,92,154,116,163,80,153,45" data-direction="right" href="#"
                                  shape="poly" />
                                <area coords="113,17,134,32,145,47,108,68,94,53" data-direction="right_up" href="#"
                                  shape="poly" />
                                <area coords="48,17,69,53,53,68,18,48" data-direction="left_up" href="#" shape="poly" />
                                <area coords="53,93,68,107,49,147,17,114" data-direction="left_down" href="#"
                                  shape="poly" />
                                <area coords="110,91,147,113,113,146,93,107" data-direction="right_down" href="#"
                                  shape="poly" />
                                <area coords="81,79,29" data-direction="center" href="#" shape="circle" />
                                <area nohref="nohref" shape="default" /></map>
                            </td>
                          </tr>
                        </table>
                      </td>
                    </tr>
                    <tr>
                      <td align="right">
                        <script>dwn(IDC_SHIELDING_INDEXS); //遮挡索引</script>
                      </td>
                      <td>
                        <select id="ShieldingIndex" style="width:50px;" class="sysinput2">
                        </select>
                      </td>
                      <td></td>
                    </tr>
                    <tr>
                      <td width="65">
                        <button class="btn btn-inverse btn-black button_test index_btn" style="width:65px;"
                          onclick="SetRegion();" id="btnSetRegion">
                          <script>dwn(IDC_SET); //设置</script>
                        </button>
                      </td>
                      <td width="65">
                        <button class="btn btn-inverse btn-black button_test index_btn" style="width:65px;"
                          onclick="Areas();" id="btnSelRegion">
                          <script>dwn(IDC_AREA); //区域</script>
                        </button>
                      </td>
                      <td></td>
                    </tr>
                    <tr>
                      <td width="65">
                        <button class="btn btn-inverse btn-black button_test index_btn" style="width:65px;"
                          onclick="Display();" id="btnShowMask">
                          <script>dwn(IDC_SHOW); //显示</script>
                        </button>
                      </td>
                      <td width="65">
                        <button class="btn btn-inverse btn-black button_test index_btn" style="width:65px;"
                          onclick="HideRegion();" id="btnHideMask">
                          <script>dwn(IDC_SHIELDING_HIDE); //隐藏</script>
                        </button>
                      </td>
                      <td width="65">
                        <button class="btn btn-inverse btn-black button_test index_btn" style="width:65px;"
                          onclick="DeleteRegion();" id="btnDelMask">
                          <script>dwn(IDC_DEL); //删除</script>
                        </button>
                      </td>
                    </tr>
                  </table>
                  <table width="100%" style="margin-top:0px;display:none;" border="0" id="tbQJ">
                    <tr>
                      <td width="20%" align="left">
                        <script>dwn(IDC_BLOCKCOLOR);</script>
                        <select id="selMaskColor" style="width:190px;" onchange="SetColorChg();">
                          <option value="0" bgvalue="RGB(8,6,7)" style="background: RGB(8,6,7)"></option>
                          <option value="1" bgvalue="RGB(32,34,33)" style="background: RGB(32,34,33)"></option>
                          <option value="2" bgvalue="RGB(61,59,60)" style="background: RGB(61,59,60)"></option>
                          <option value="3" bgvalue="RGB(88,87,86)" style="background: RGB(88,87,86)"></option>
                          <option value="4" bgvalue="RGB(113,111,112)" style="background: RGB(113,111,112)"></option>
                          <option value="5" bgvalue="RGB(139,137,138)" style="background: RGB(139,137,138)"></option>
                          <option value="6" bgvalue="RGB(169,167,168)" style="background: RGB(169,167,168)"></option>
                          <option value="7" bgvalue="RGB(195,193,194)" style="background: RGB(195,193,194)"></option>
                          <option value="8" bgvalue="RGB(178,44,51)" style="background: RGB(178,44,51)"></option>
                          <option value="9" bgvalue="RGB(24,170,0)" style="background: RGB(24,170,0)"></option>
                          <option value="10" bgvalue="RGB(18,66,250)" style="background: RGB(18,66,250)"></option>
                          <option value="11" bgvalue="RGB(33,175,221)" style="background: RGB(33,175,221)"></option>
                          <option value="12" bgvalue="RGB(189,185,0)" style="background: RGB(189,185,0)"></option>
                          <option value="13" bgvalue="RGB(180,37,163)" style="background: RGB(180,37,163)"></option>
                        </select>
                      </td>
                    </tr>
                    <tr>
                      <td width="20%" align="left">
                        <script>dwn(IDC_BLOCKSET_AREA);</script>
                        <select style="width:65px;" onchange="AreaMaskSelect()" id="selAreaMask" class="sysinput2">
                          <option value="0">1</option>
                          <option value="1">2</option>
                          <option value="2">3</option>
                          <option value="3">4</option>
                        </select>
                      </td>
                    </tr>
                    <tr>
                      <td>
                        <label for='checkMask'>
                          <script>dwn(IDC_BLOCKSET_EN);</script>
                        </label>
                        <input type="checkbox" id="checkMask" onclick="MaskEnableSelect();" />
                      </td>
                    </tr>
                    <tr>
                      <td width="20%" align="left">
                        <button class="btn btn-inverse btn-black button_test index_btn" style="width:65px;"
                          onclick="SaveVideoMask();">
                          <script>dwn(IDC_SAVE);</script>
                        </button>
                        <button class="btn btn-inverse btn-black button_test index_btn" style="width:65px;"
                          onclick="DelVideoMask();">
                          <script>dwn(IDC_DEL);</script>
                        </button>
                      </td>
                    </tr>
                  </table>
                </div>
              </div>
              <div id="subtabs_roisetting" class='tabs ui-tabs ui-widget ui-widget-content ui-corner-all'
                style='width: 500px;height:470px;display:none'>
                <ul>
                  <li><a href="#subtabs_roisetting-1">
                      <script>dwn(IDC_ROI_SETTING)</script>
                    </a></li>
                </ul>
                <div id="subtabs_roisetting-1">
                  <table width="100%" style="margin-top:0px;" border="0">

                    <tr>
                      <td width="20%" align="left">
                        <script>dwn(IDC_GEN_AREA);</script>
                        <select style="width:65px;" onchange="RoiSelect()" id="selRoi" class="sysinput2">
                          <option value="0">1</option>
                          <option value="1">2</option>
                          <option value="2">3</option>
                          <option value="3">4</option>
                          <option value="4">5</option>
                          <option value="5">6</option>
                          <option value="6">7</option>
                          <option value="7">8</option>
                        </select>
                      </td>
                    </tr>
                    <tr>
                      <td>
                        <label for='checkRoi'>
                          <script>dwn(IDC_GEN_ENABLE);</script>
                        </label>
                        <input type="checkbox" id="checkRoi" onclick="RoiEnableSelect();" />
                      </td>
                    </tr>
                    <tr>
                      <td width="20%" align="left">
                        <button class="btn btn-inverse btn-black button_test index_btn" style="width:65px;"
                          onclick="SaveRoi();">
                          <script>dwn(IDC_SAVE);</script>
                        </button>
                        <button class="btn btn-inverse btn-black button_test index_btn" style="width:65px;"
                          onclick="DelRoi();">
                          <script>dwn(IDC_DEL);</script>
                        </button>
                      </td>
                    </tr>
                  </table>
                </div>
              </div>
              <div id="subtabs_color" class='tabs ui-tabs ui-widget ui-widget-content ui-corner-all'
                style='width: 500px;height:470px;display:none'>
                <ul>
                  <li id="liColorParamSet"><a href="#subtabs_color-1">
                      <script>dwn(IDC_COLORPARAMETER)</script>
                    </a></li>
                  <li><a href="#subtabs_color-2" onclick="showVideoControl()" id="liVideoControl">
                      <script>dwn(IDC_VIDEO_REVERSE)</script>
                    </a></li>
                  <li id="liAdvanceSet"><a href="#subtabs_color-3">
                      <script>dwn(IDC_AE)</script>
                    </a></li>
                </ul>
                <div id="subtabs_color-1" style="display:none">
                  <table width="95%" style="margin-top:0px;" border="0" id="colorSetTb">
                    <!--
                                  <tr>
                                    <td  align="right">
                                        <script>dwn(IDC_CONTRAST_AGAIN_ENABLE);</script>
                                    </td>
                                    <td  align="left">
                                        <input type="checkbox" id="cbContrastAgain" style="vertical-aglin:middle;margin-left:10px;" onclick="checkContrastAgain(1);"/>
                                        <span id="contrastAgainTip"><script>dwn(IDC_CONTRAST_AGAIN_TIP);</script></span>
                                    </td>
                                    <td  align="left">
                                    </td>
                                </tr>
                                  <tr id="trContrastAgain">
                                    <td  align="right">
                                        <script>dwn(IDC_BRIGHT);//对比度增强</script>
                                    </td>
                                    <td  align="center">
                                        <div id="contrastAgain" style="width:140px;height:8px;">
                                        </div>
                                    </td>
                                    <td    align="left" id="tdContrastAgain">
                                    </td>
                                </tr>
                                 -->
                    <tr id="trBright" style="height:30px;">
                      <td align="right">
                        <script>dwn(IDC_BRIGHT);//亮度</script>
                      </td>
                      <td width="170" align="center">
                        <div id="bright" style="width:140px;height:8px;">
                        </div>
                      </td>
                      <td id="tdBright" align="left">64</td>
                    </tr>
                    <tr id="trContrast">
                      <td align="right">
                        <script>dwn(IDC_CONTRAST);//对比度</script>
                      </td>
                      <td align="center">
                        <div id="contrast" style="width:140px;height:8px;">
                        </div>
                      </td>
                      <td id="tdContrast" align="left">64</td>
                    </tr>
                    <tr>
                      <td align="right">
                        <script>dwn(IDC_SATURATION);//饱和度</script>
                      </td>
                      <td align="center">
                        <div id="saturation" style="width:140px;height:8px;">
                        </div>
                      </td>
                      <td name="tdSaturation" id="tdSaturation" align="left">64</td>
                    </tr>
                    <tr>
                      <td align="right">
                        <script>dwn(IDC_SHRPNESS);//锐度</script>
                      </td>
                      <td align="center">
                        <div id="sharpness" style="width:140px;height:8px;">
                        </div>
                      </td>
                      <td height="25" name="tdSharpness" id="tdSharpness" align="left"></td>
                    </tr>
                    <tr>
                      <td width="160" align="right">
                        <script>dwn(IDC_NIGHTLUMA);//夜视亮度</script>
                      </td>
                      <td width="170" align="center">
                        <div id="nightluma" style="width:140px;height:8px;">
                        </div>
                      </td>
                      <td id="tdNightluma" align="left">64</td>
                    </tr>
                    <tr>
                      <td width="160" align="right">
                        <script>dwn(IDC_HIGH_LIGHT_SUPPRESS);//强光抑制</script>
                      </td>
                      <td width="170" align="center">
                        <div id="highLightSuppress" style="width:140px;height:8px;">
                        </div>
                      </td>
                      <td id="tdHighLightSuppress" align="left">64</td>
                    </tr>
                    <tr>
                      <td align="right">
                        <script>dwn(IDC_GAIN);//增益</script>
                      </td>
                      <td align="center">
                        <div id="gain" style="width:140px;height:8px;">
                        </div>
                      </td>
                      <td name="tdGain" id="tdGain" align="left">64</td>
                    </tr>
                    <tr>
                      <td height="25" align="center" colspan="3">
                        <span>
                          <script>dwn(IDC_NOTE_GAIN);//光源频率与帧率相关</script>
                        </span>
                        <div style="margin:5px 5px 5px 0px;border:1px solid #666"></div>
                      </td>
                    </tr>

                    <tr id="trLamp">
                      <td height="25" align="right">
                        <script>dwn(IDC_LAMP);//光源频率</script>
                      </td>
                      <td height="25" align="left" colspan="2">&nbsp;&nbsp;
                        <input type="radio" name="lampchk" onclick="lamchkSave()" value="1" id="lampchkOpen">
                        <label for="lampchkOpen">50Hz</label>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
                        <input type="radio" name="lampchk" onclick="lamchkSave()" value="0" id="lampchkClose">
                        <label for="lampchkClose">60Hz</label>
                      </td>
                    </tr>
                    <tr>
                      <td height="25" align="center" colspan="3">
                        <span>
                          <script>dwn(IDC_LAMP_RATE);//光源频率与帧率相关</script>
                        </span>
                        <div style="margin:5px 5px 5px 0px;border:1px solid #666"></div>
                      </td>
                    </tr>
                    <tr>
                      <td height="25" align="center" colspan="3">
                        <button class="btn btn-inverse btn-black button_test index_btn" onclick="DefaultColor();"
                          style="width:80px;">
                          <script>dwn(IDC_DEFAULT);//默认值</script>
                        </button>
                      </td>
                    </tr>
                  </table>

                </div>
                <div id="subtabs_color-2">
                  <table width="40%" style="margin-top:5px;margin-left:30px;" align="left">
                    <tr>
                      <td width="140" align="left">
                        <input type="radio" id='reverse_0' name="videoreverse" value="0" onclick="PreviewReverse()">
                        <label for="reverse_0">
                          <script>dwn(IDC_VIDEO_NORMAL);//正常</script>
                        </label>
                      </td>
                    </tr>
                    <tr>
                      <td width="140" align="left">
                        <input type="radio" id='reverse_1' name="videoreverse" value="1" onclick="PreviewReverse()">
                        <label for="reverse_1">
                          <script>dwn(IDC_VIDEO_HORIZONTAL);//水平镜像</script>
                        </label>
                      </td>
                    </tr>
                    <tr>
                      <td width="140" align="left">
                        <input type="radio" id='reverse_2' name="videoreverse" value="2" onclick="PreviewReverse()">
                        <label for="reverse_2">
                          <script>dwn(IDC_VIDEO_VERTICAL);//垂直镜像</script>
                        </label>
                      </td>
                    </tr>
                    <tr>
                      <td width="140" align="left">
                        <input type="radio" id='reverse_3' name="videoreverse" value="3" onclick="PreviewReverse()">
                        <label for="reverse_3">
                          <script>dwn(IDC_VIDEO_DIAGONAL);//对角镜像</script>
                        </label>
                      </td>
                    </tr>

                  </table>

                </div>
                <div id="subtabs_color-3" style="display:none">
                  <table width="80%" style="margin-top:0px;margin-left:0px;" border="0" align="left">
                    <tr>
                      <td align="right">
                        <script>dwn(IDC_AES)//电子快门</script>：
                      </td>
                      <td align="left">
                        <select id="AESelect" class="sysinput" style="width:140px;">
                          <option value="0" selected>
                            <script>dwn(IDC_PTZ_AUTO)</script>
                          </option>
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
                      <td align="left">&nbsp;</td>

                    </tr>
                    <tr>
                      <td align="right">
                        <script>dwn(IDC_AWB)//白平衡控制模式</script>：
                      </td>
                      <td align="left">
                        <select id="AWBSelect" onchange="changeAWBSelect();" class="sysinput" style="width:140px;">
                          <option value="0" selected>
                            <script>dwn(IDC_PTZ_AUTO)</script>
                          </option>
                          <option value="1">
                            <script>dwn(IDC_WEATHER_SUNNY)</script>
                          </option>
                          <option value="2">
                            <script>dwn(IDC_WEATHED_CLOUDY)</script>
                          </option>
                          <option value="3">
                            <script>dwn(IDC_WEATHED_LAMPS)</script>
                          </option>
                          <option value="4">
                            <script>dwn(IDC_WEATHED_DAYLIGHT)</script>
                          </option>
                        </select>
                      </td>
                      <td align="left">&nbsp;</td>
                    </tr>
                    <tr>
                      <td align="right">
                        <script>dwn(IDC_REDGain)//红增益</script>：
                      </td>
                      <td align="left"><input type="text" id="redgain" class="sysinput" style="width:140px;"
                          onblur="redgainBlur()" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);"
                          maxlength="3"></td>
                      <td align="left">(1~255)</td>
                    </tr>
                    <tr>
                      <td align="right">
                        <script>dwn(IDC_BLUEGain)//蓝增益</script>：
                      </td>
                      <td align="left"><input type="text" id="blurgain" class="sysinput" style="width:140px;"
                          onblur="blurgainBlur();" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);"
                          maxlength="3">
                      </td>
                      <td align="left">(1~255)</td>
                    </tr>
                    <tr>
                      <td align="right">
                        <script>dwn(IDC_LOWLIGHTENHANCE)//低照亮度增強</script>：
                      </td>
                      <td align="left">
                        <div id="lowlightenhance" style="width:140px;height:8px;">
                        </div>
                      </td>
                      <td align="left" id="tdlowlightenhance">&nbsp;</td>
                    </tr>

                    <tr>
                      <td align="right">
                        <script>dwn(IDC_NIGHT_FACE_ENHANCE)//夜视人脸增强</script>：
                      </td>
                      <td align="left">
                        <select id="nightfacemode" class="sysinput" style="width:140px;">
                          <option value="0">
                            <script>dwn(IDC_GEN_SWITCH_CLOSE)</script>
                          </option>
                          <option value="1">
                            <script>dwn(IDC_NIGHT_FACE_ENHANCE_LEVEL1)</script>
                          </option>
                          <option value="2">
                            <script>dwn(IDC_NIGHT_FACE_ENHANCE_LEVEL2)</script>
                          </option>
                          <option value="3">
                            <script>dwn(IDC_NIGHT_FACE_ENHANCE_LEVEL3)</script>
                          </option>
                        </select>
                      </td>
                      <td align="left">&nbsp;</td>
                    </tr>

                    <!-- <tr>
                                    <td align="right"><script>dwn(IDC_SHADOW_CORRECTION)//阴影矫正</script>：</td>
                                    <td align="left"><input type="checkbox"  id="shadowCorrection" style="margin-left:15px;"><script>dwn(IDC_CONTRAST_AGAIN_TIP)//夜晚有效</script>
                                    </td>
                                </tr> -->
                    <tr>
                      <td height="25" align="center" colspan="3">
                        <div style="margin:5px 5px 5px 0px;border:1px solid #666"></div>
                        <button class="btn btn-inverse btn-black button_test index_btn" onclick="AWBSave();"
                          style="width:65px;">
                          <script>dwn(IDC_SAVE);//保存</script>
                        </button>
                    </tr>
                  </table>

                </div>
              </div>
              <div id="subtabs_param" class='tabs ui-tabs ui-widget ui-widget-content ui-corner-all'
                style='width: 500px;height:470px;display:none;'>
                <ul>
                  <li><a href="#subtabs-1" onclick="showBaseTitle()">
                      <script>dwn(IDC_NAME_ENABLE)//基础字幕</script>
                    </a></li>
                  <li><a href="#subtabs-2" onclick="showExtendTitle()">
                      <script>dwn(IDC_NAME_DIEJIA)//扩展字幕</script>
                    </a></li>
                </ul>
                <div id="subtabs-1">
                  <table width="100%" style="margin-top:0px;" border="0">
                    <tr>
                      <td width="20%" align="left">
                        <input type="radio" id='name_enb' name="namechk" value="1" onclick="SetOSDPosition(0);">
                        <label for="name_enb" style="font-size:10pt;">
                          <script>dwn(IDC_SHOWNAME);//显示</script>
                        </label>
                      </td>
                      <td width="20%" align="left">
                        <input type="radio" id='name_dis' name="namechk" value="0" onclick="SetOSDPosition(0);">
                        <label for="name_dis" style="font-size:10pt;">
                          <script>dwn(IDC_HIDENAME);//隐藏</script>
                        </label>
                      </td>
                      <td align="right" width="20%">
                        <font style="font-size:10pt;">
                          <script>dwn(IDC_NAME)//名称</script>
                        </font>
                      </td>
                      <td align="left">
                        <input type="text" id="name" class="sysinput" style="width:110px;" maxlength="12">
                        <font>
                          <script>dwn(IDC_NAME_CONTENT);//(1~12)个字符</script>
                        </font>
                      </td>
                    </tr>
                    <tr>
                      <td align="left">
                        <input type="radio" id='time_enb' name="timechk" value="1" onclick="SetOSDPosition(1);">
                        <label for="time_enb" style="font-size:10pt;">
                          <script>dwn(IDC_SHOWTIME);//显示时间</script>
                        </label>
                      </td>
                      <td align="left">
                        <input type="radio" id='time_dis' name="timechk" value="0" onclick="SetOSDPosition(1);">
                        <label for="time_dis" style="font-size:10pt;">
                          <script>dwn(IDC_HIDETIME);//隐藏时间</script>
                        </label>
                      </td>
                      <td align="right">
                        <font style="font-size:10pt;">
                          <script>dwn(IDC_COLORSETTING);//颜色设置</script>：
                        </font>
                      </td>
                      <td align="left">
                        <select class="sysinput" style="width:110px;" id="colorOption">
                          <option value="1">
                            <script>dwn(IDC_OSD_COLOR1)</script>
                          </option>
                          <option value="2">
                            <script>dwn(IDC_OSD_COLOR2)</script>
                          </option>
                          <option value="0">
                            <script>dwn(IDC_HDR_AUTO)</script>
                          </option>
                        </select>
                      </td>
                    </tr>
                    <tr>
                      <td width="20%" align="left">
                        <input type="radio" id='osd_enb' name="osdchk" value="1" onclick="SetOSDPosition(3);">
                        <label for="osd_enb" style="font-size:10pt;">
                          <script>dwn(IDC_SHOWOSD);//显示</script>
                        </label>
                      </td>
                      <td width="20%" align="left">
                        <input type="radio" id='osd_dis' name="osdchk" value="0" onclick="SetOSDPosition(3);">
                        <label for="osd_dis" style="font-size:10pt;">
                          <script>dwn(IDC_HIDEOSD);//隐藏</script>
                        </label>
                      </td>
                      <td align="right" width="20%">
                        <font style="font-size:10pt;">
                          <script>dwn(IDC_NAME)//名称</script>
                        </font>
                      </td>
                      <td align="left">
                        <input type="text" id="osdname" class="sysinput" style="width:110px;" maxlength="12">
                        <font>
                          <script>dwn(IDC_NAME_CONTENT);//(1~12)个字符</script>
                        </font>
                      </td>
                    </tr>
                    <tr>
                      <td align="left">
                        <input type="radio" id='bps_enb' name="bpschk" value="1" onclick="SetOSDPosition(2);">
                        <label for="bps_enb" style="font-size:10pt;">
                          <script>dwn(IDC_SHOWBPS);//显示码率</script>
                        </label>
                      </td>
                      <td align="left">
                        <input type="radio" id='bps_dis' name="bpschk" value="0" onclick="SetOSDPosition(2);">
                        <label for="bps_dis" style="font-size:10pt;">
                          <script>dwn(IDC_HIDEBPS);//隐藏码率</script>
                        </label>
                      </td>
                      <td align="right" width="20%">
                        <font style="font-size:10pt;">
                          <script>dwn(IDC_OSD_FONT)//OSD字体</script>
                        </font>
                      </td>
                      <td align="left">
                        <select class="sysinput" style="width:110px;" id="osd_font">
                          <option value="2">
                            <script>dwn(IDC_FONT_BIG)</script>
                          </option>
                          <option value="1">
                            <script>dwn(IDC_FONT_MIDDLE)</script>
                          </option>
                          <option value="0">
                            <script>dwn(IDC_FONT_SMALL)</script>
                          </option>
                        </select>
                      </td>
                    </tr>
                    <tr>
                      <td align="center" colspan="4">
                        <button onclick="SaveBasic();" class="btn btn-inverse btn-black button_test index_btn"
                          style="width:65px;">
                          <script>dwn(IDC_SAVE);//保存</script>
                        </button>
                      </td>
                    </tr>
                  </table>
                </div>
                <div id="subtabs-2">
                  <table width="100%" style="margin-top:0px;">
                    <tr>
                      <td width="30" align="left">
                        <input type="checkbox" id="check_1" />
                      </td>
                      <!-- <td align="left">X:
                        <input type="text" id="xpos_1" class="sysinput2" style="width:45px;" maxlength="5" />
                      </td>
                      <td align="right">Y:
                        <input type="text" id="ypos_1" class="sysinput2" style="width:45px;" maxlength="5" />
                      </td> -->
                      <td align="left">
                        <script>dwn(IDC_OSD_NAME);//字符</script>
                        <input type="text" id="osd_1" class="sysinput2" style="width:120px;" maxlength="12" />
                      </td>
                      <td align="left">
                        <button class="btn btn-inverse btn-black button_test index_btn"
                          style="width:65px;margin:0px 10px 0px 10px;" onclick="SaveOsdStr(1)">
                          <script>dwn(IDC_SAVE);//保存</script>
                        </button>
                      </td>
                    </tr>
                    <tr>
                      <td width="30" align="left">
                        <input type="checkbox" id="check_2" />
                      </td>
                      <!-- <td align="left">X:
                        <input type="text" id="xpos_2" class="sysinput2" style="width:45px;" maxlength="5">
                      </td>
                      <td align="right">Y:
                        <input type="text" id="ypos_2" class="sysinput2" style="width:45px;" maxlength="5">
                      </td> -->
                      <td align="left">
                        <script>dwn(IDC_OSD_NAME);//字符</script>
                        <input type="text" id="osd_2" class="sysinput2" style="width:120px;" maxlength="12">
                      </td>
                      <td align="left">
                        <button class="btn btn-inverse btn-black button_test index_btn"
                          style="width:65px;margin:0px 10px 0px 10px;" onclick="SaveOsdStr(2)">
                          <script>dwn(IDC_SAVE);//保存</script>
                        </button>
                      </td>
                    </tr>
                    <tr>
                      <td width="30" align="left">
                        <input type="checkbox" id="check_3" />
                      </td>
                      <!-- <td align="left">X:
                        <input type="text" id="xpos_3" class="sysinput2" style="width:45px;" maxlength="5">
                      </td>
                      <td align="right">Y:
                        <input type="text" id="ypos_3" class="sysinput2" style="width:45px;" maxlength="5">
                      </td> -->
                      <td align="left">
                        <script>dwn(IDC_OSD_NAME);//字符</script>
                        <input type="text" id="osd_3" class="sysinput2" style="width:120px;" maxlength="12">
                      </td>
                      <td align="left">
                        <button class="btn btn-inverse btn-black button_test index_btn"
                          style="width:65px;margin:0px 10px 0px 10px;" onclick="SaveOsdStr(3)">
                          <script>dwn(IDC_SAVE);//保存</script>
                        </button>
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
                      <td align="center">
                        <script>dwn(IDC_OSD_NAME);//字符</script>
                        <input type="text" id="osd_5" class="sysinput2" style="width:120px;" maxlength="12">
                      </td>
                      <td align="center">
                        <button class="btn btn-inverse btn-black button_test index_btn"
                          style="width:65px;margin:0px 10px 0px 10px;" onclick="SaveOsdStr(5)">
                          <script>dwn(IDC_SAVE);//保存</script>
                        </button>
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
                      <td align="center">
                        <script>dwn(IDC_OSD_NAME);//字符</script>
                        <input type="text" id="osd_6" class="sysinput2" style="width:120px;" maxlength="12">
                      </td>
                      <td align="center">
                        <button class="btn btn-inverse btn-black button_test index_btn"
                          style="width:65px;margin:0px 10px 0px 10px;" onclick="SaveOsdStr(6)">
                          <script>dwn(IDC_SAVE);//保存</script>
                        </button>
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
                      <td align="center">
                        <script>dwn(IDC_OSD_NAME);//字符</script>
                        <input type="text" id="osd_7" class="sysinput2" style="width:120px;" maxlength="12">
                      </td>
                      <td align="center">
                        <button class="btn btn-inverse btn-black button_test index_btn"
                          style="width:65px;margin:0px 10px 0px 10px;" onclick="SaveOsdStr(7)">
                          <script>dwn(IDC_SAVE);//保存</script>
                        </button>
                      </td>
                    </tr>
                    <!-- <tr>
                      <td colspan="5" align="center">
                        <font>
                          (
                          <script>dwn(IDC_X + "1919," + IDC_Y + "1079")</script>)
                        </font>
                      </td>
                    </tr> -->
                  </table>
                </div>
              </div>
            </td>
          </tr>
        </table>
      </div>
      <div id="tabs-3">
        <table style='width: 500px;margin-left: 0px;'>
          <tr>
            <td height="25" width="200" align="right">
              <script>
                dwn(IDC_AUDIOINPUTENABLE);//音频输入开关
              </script>
            </td>
            <td height="25" width="260" align="left">
              <input type="radio" name="rdAudioIN" value="1" onclick="SetDisableEnable(1)">
              <script>dwn(IDC_GEN_SWITCH_OPEN);//开</script>
              <input type="radio" name="rdAudioIN" value="0" onclick="SetDisableEnable(0)">
              <script>dwn(IDC_GEN_SWITCH_CLOSE);//关</script>
            </td>
            <td height="25" width="30" align="left"></td>
          </tr>
          <tr>
            <td height="25" align="right">
              <script>
                dwn(IDC_AUDIO_INPUTTYPE);//音频输入方式
              </script>
            </td>
            <td height="25">
              <select id="inputtype" style="width:80px;margin-left:0px;" class="sysinput2">
                <option value="0">Mic</option>
                <option value="1">Line-In</option>
              </select>
            </td>
            <td height="25" align="left"></td>
          </tr>
          <tr>
            <td height="25" align="right">
              <script>
                dwn(IDC_AUDIO_CODETYPE);//音频输入编码格式
              </script>
            </td>
            <td height="25">
              <select id="codetype" style="width:80px;" class="sysinput2">
                <option value="1">G711A</option>
                <option value="2">G711U</option>
              </select>
            </td>
            <td height="25" align="left"></td>
          </tr>
          <tr style="display:none">
            <td height="25" align="right">
              <script>dwn(IDC_BPS);//码率</script>
            </td>
            <td height="25">
              <select id="amrbps" style="width:80px;" class="sysinput2">
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
            <td height="25" align="left"></td>
          </tr>
          <tr>
            <td height="25" align="right">
              <script>dwn(IDC_AUDIOINPUT);//音频输入音量</script>
            </td>
            <td height="25" align="left">
              <div id="divInvolume" style="display:none;"></div>
              <div id="involume" style="width:250px;height:5px;" />
            </td>
            <td height="25" align="left" id="tdInvolume">90</td>
          </tr>
          <tr>
            <td height="25" align="right">
              <script>dwn(IDC_AUDIOOUTPUT);//音频输出音量</script>
            </td>
            <td height="25" align="left">
              <div id="divOutvolume" style="display:none;"></div>
              <div id="outvolume" style="width:250px;height:5px;" />
            </td>
            <td height="25" align="left" id="tdOutvolume">50</td>
          </tr>
          <tr>
            <td height="25" align="right">
              <button class="btn btn-inverse btn-black button_test index_btn" style="width:75px;margin-top:0px;"
                onclick="DefauleVolume();">
                <script>dwn(IDC_DEFAULT);//默认值</script>
              </button>
            </td>
            <td height="25" align="left">
              <button class="btn btn-inverse btn-black button_test index_btn" style="width:75px;margin-top:0px;"
                onclick="SaveAudioSet();">
                <script>dwn(IDC_SAVE);//保存</script>
              </button>
            </td>
            <td height="25" align="left"></td>

          </tr>
        </table>
      </div>
      <div id="tabs-4">
        <table style='width: 500px;margin-left: 0px;'>
          <tr id="tr_dbl">
            <td height="25" align="right" width="150">
              <script>
                dwn(IDC_INFRARED_MODE);//灯光模式
              </script>
            </td>
            <td height="25" width="350" align="left">
              <select id="lightmode" style="width:150px;margin-left:0px;" class="sysinput2"
                onchange="changeSwitchMode()">
                <option value="2" id="dev_dbl">
                  <script>dwn(IDC_DBLIGHT)</script>
                </option>
                <option value="1" id="dev_white">
                  <script>dwn(IDC_STARLIGHT)</script>
                </option>
                <option value="0" id="dev_ir">
                  <script>dwn(IDC_INFREDLIGHT)</script>
                </option>
              </select>
            </td>
            <td height="25" align="left" width="30"></td>
          </tr>
          <tr id="tr_white">
            <td height="25" align="right" width="150">
              <script>
                dwn(IDC_INFRARED_MODE);//灯光模式
              </script>
            </td>
            <td height="25" width="350" align="left">
              <select id="lightmode" style="width:150px;margin-left:0px;" class="sysinput2"
                onchange="changeSwitchMode()">
                <option value="0" id="dev_white">
                  <script>dwn(IDC_STARLIGHT)</script>
                </option>
              </select>
            </td>
            <td height="25" align="left" width="30"></td>
          </tr>
          <tr id="tr_ir">
            <td height="25" align="right" width="150">
              <script>
                dwn(IDC_INFRARED_MODE);//灯光模式
              </script>
            </td>
            <td height="25" width="350" align="left">
              <select id="lightmode" style="width:150px;margin-left:0px;" class="sysinput2"
                onchange="changeSwitchMode()">
                <option value="1" id="dev_ir">
                  <script>dwn(IDC_INFREDLIGHT)</script>
                </option>
              </select>
            </td>
            <td height="25" align="left" width="30"></td>
          </tr>
          <tr id="tr_light_onoff">
            <td height="25" align="right" width="150">
              <script>
                dwn(IDC_CONTROLMODE);//灯光开关
              </script>
            </td>
            <td height="25" width="180" align="left">
              <select id="lightonoff" style="width:150px;margin-left:0px;" class="sysinput2"
                onchange="changeLightOnOff()">
                <option value="2">
                  <script>dwn(IDC_AUTO_CONTROL)</script>
                </option>
                <option value="3">
                  <script>dwn(IDC_DAY_AND_NIGHT)</script>
                </option>
              </select>
            </td>
            <td height="25" align="left" width="30"></td>
          </tr>
          <tr id="tr_night_time">
            <td height="25" align="right" width="100">
              <script>
                dwn(IDC_TIME_OF_NIGHT);//夜间时间
              </script>
            </td>
            <td height="25" width="350" align="left" id="tdNightTime"></td>
            <td height="25" align="left" width="30"></td>
          </tr>
          <tr id="tr_white_light_bright">
            <td height="25" align="right" width="150">
              <script>
                dwn(IDC_WHITE_LIGHT_BRIGHT);//白光灯亮度
              </script>
            <td height="25" width="350" align="left">
              <select id="brightness" style="width:150px;margin-left:0px;" class="sysinput2">
                <option value="1">100%</option>
                <option value="2">75%</option>
                <option value="3">50%</option>
                <option value="4">25%</option>
              </select>
            </td>
            <td height="25" align="left" width="30"></td>
            </td>
          </tr>
          <tr id="tr_sensitivity">
            <td height="25" align="right">
              <script>dwn(IDC_SENSITIVITY);//灵敏度</script>
            </td>
            <td height="25" align="left">
              <div id="sensitivity" style="width:340px;height:5px;" />
            </td>
            <td height="25" align="left" id="tdSensitivity"></td>
          </tr>
          <tr id="tr_shine_mode">
            <td height="25" align="right" width="100">
              <script>
                dwn(IDC_SHINE_MODE);//闪灯模式
              </script>
            </td>
            <td height="25" width="350" align="left">
              <select id="shinemode" style="width:150px;margin-left:0px;" class="sysinput2"
                onchange="changeShineMode()">
                <option value="0">
                  <script>dwn(IDC_SHINE_FULL_COLOR)</script>
                </option>
                <option value="1">
                  <script>dwn(IDC_SHINE_BLACK_WHITE)</script>
                </option>
                <option value="2">
                  <script>dwn(IDC_SHINE_CAPACITY)</script>
                </option>
              </select>
            </td>
            <td height="25" align="left" width="30"></td>
          </tr>
          <tr id="tr_white_light_control">
            <td height="25" align="right" width="100">
              <script>
                dwn(IDC_WHITE_LIGHT_CONTROL);//白光灯控制
              </script>
            </td>
            <td height="25" width="350" align="left">
              <select id="white_reverse" style="width:150px;margin-left:0px;" class="sysinput2">
                <option value="0">
                  <script>dwn(IDC_HIGH_OPEN)</script>
                </option>
                <option value="1">
                  <script>dwn(IDC_LOW_OPEN)</script>
                </option>
              </select>
            </td>
            <td height="25" align="left" width="30"></td>
          </tr>
          <tr id="tr_inf_light_control">
            <td height="25" align="right" width="150">
              <script>
                dwn(IDC_INFRARED_CONTROL);//红外灯控制
              </script>
            </td>
            <td height="25" width="350" align="left">
              <select id="infrared_reverse" style="width:150px;margin-left:0px;" class="sysinput2">
                <option value="0">
                  <script>dwn(IDC_HIGH_OPEN)</script>
                </option>
                <option value="1">
                  <script>dwn(IDC_LOW_OPEN)</script>
                </option>
              </select>
            </td>
            <td height="25" align="left" width="30"></td>
          </tr>
          <tr>
            <td height="25" align="left" colspan="3">
              <button class="btn btn-inverse btn-black button_test index_btn"
                style="width:75px;margin-top:0px;margin-left:20px;" onclick="SaveInfraredSet();">
                <script>dwn(IDC_SAVE);//保存</script>
              </button>
            </td>
          </tr>
          <!-- <tr>
            <td align="align" height="25" colspan="3">
              <label for='gpiohighz'>
                <script>dwn(IDC_HIGHZ_OPEN);//启用高阻态</script>
              </label>
              <input type="checkbox" name="gpiohighz" id="openhightz" onchange="SaveHighZ();">
            <td>
          </tr> -->
          <tr>
            <td align="left">
              <script>dwn(IDC_OTHER_INFO);</script>
            </td>
          </tr>
          <tr>
            <td align="left">
              <span>
                <script>dwn(IDC_sensitive_TYPE)</script>
              </span>
              <label id="sensType"></label>
            </td>
          </tr>
          <tr>
            <td align="left">
              <span>
                <script>dwn(IDC_sensitive_value)</script>
              </span>
              <label id="sensRead"></label>
            </td>
          </tr>
        </table>
      </div>
      <div id="tabs-5">
        <table style='width: 99%;margin-left: 30px;'>
          <tr>
            <td align="left">
              <script>dwn(IDC_3D_NOISE_REDUCTION);</script>
            </td>
          </tr>
          <tr>
            <td align="left">
              <script>dwn(IDC_NOISE_REDUCTION_ENABLE);</script>
              <input type="radio" id='denoise_open' name="denoise" value="1">
              <label for="denoise_open">
                <script>dwn(IDC_GEN_SWITCH_OPEN);//开</script>
              </label>
              <input type="radio" id='denoise_close' name="denoise" value="0">
              <label for="denoise_close">
                <script>dwn(IDC_GEN_SWITCH_CLOSE);//关</script>
              </label>
            </td>
          </tr>
          <tr>
            <td align="left">
              <table>
                <tr>
                  <td align="left">
                    <script>dwn(IDC_NOISE_REDUCTION_STRENGTH);</script>
                  </td>
                  <td height="25" align="left">
                    <div id="StrengthSlider" style="margin-left:10px;width:400px;height:5px;" />
                  </td>
                  <td height="25" align="left">
                    <span id="tdStrengthValue" style="margin-left:10px;">0</span>
                  </td>
                </tr>
              </table>
            </td>
          </tr>
          <tr>
            <td align="left">
              <button class="btn btn-inverse btn-black button_test index_btn" style="width:65px;margin-top:0px;"
                onclick="SaveExtCfgNoise();">
                <script>dwn(IDC_SAVE);//保存</script>
              </button>
              <div style="margin:5px 5px 5px 0px;border:1px solid #666"></div>
              </br>
            </td>
          </tr>
          <tr>
            <td align="left">
              <script>dwn(IDC_VIDEO_ENCODE_SETTINGS);</script>
            </td>
          </tr>
          <tr>
            <td align="left">
              <script>dwn(IDC_VIDEO_ENCODE_TYPE);//编码尺寸</script>
              <select id="selEncodeSize" class="sysinput2" style="width:70px;" onchange="changeEncodeSize(this.value)">
                <option value="6">QVGA</option>
                <option value="1">CIF</option>
                <option value="7">VGA</option>
                <option value="2">D1</option>
                <option value="3">720P</option>
                <option value="8">960p</option>
                <option value="5">1080P</option>
                <option value="9">3M</option>
              </select>
              &nbsp;&nbsp;
              <script>dwn(IDC_VIDEO_ENCODE_CHILD_TYPE);//编码profile</script>
              <select id="selChildEncodeType" class="sysinput2" style="width:70px">
                <option value="0">HIGH</option>
                <option value="1">MAIN</option>
                <option value="2">BASE</option>
              </select>
              <!-- &nbsp;&nbsp;
                        <script>dwn(IDC_VIDEO_ENCODE_LEVEL);//编码level</script>
                        <select id="selEncodeLevel" class="sysinput2"  vstyle="width:70px;">
                            <option value="10">10</option>
                            <option value="20">20</option>
                            <option value="30">30</option>
                            <option value="40">40</option>
                            <option value="50">50</option>
                        </select>-->
              &nbsp;&nbsp;
              <button class="btn btn-inverse btn-black button_test index_btn" style="width:65px;margin-top:0px;"
                onclick="SaveExtCfgEncode();">
                <script>dwn(IDC_SAVE);//保存</script>
              </button>
              <div style="margin:5px 5px 5px 0px;border:1px solid #666"></div>
            </td>
          </tr>
          <tr style="display:none;">
            <td align="left">
              <script>dwn(IDC_HIKVISON_NVR_ENABLE);</script>
              <input type="radio" id='nvr_open' name="nvroise" value="1" onclick="SaveHikvisonNVR(1);">
              <label for="nvr_open">
                <script>dwn(IDC_GEN_SWITCH_OPEN);//开</script>
              </label>
              <input type="radio" id='nvr_close' name="nvroise" value="0" onclick="SaveHikvisonNVR(0);">
              <label for="nvr_close">
                <script>dwn(IDC_GEN_SWITCH_CLOSE);//关</script>
              </label>
              <div style="margin:5px 5px 5px 0px;border:1px solid #666"></div>
            </td>
          </tr>
          <tr>
            <td style="height:100px;"></td>
          </tr>
        </table>
      </div>
    </div>
  </div>


</body>

</html>