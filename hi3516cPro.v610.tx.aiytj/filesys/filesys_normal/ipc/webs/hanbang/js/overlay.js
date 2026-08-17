$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
        showVideo();
        $("#tbExtend").hide();
        GetJCP({cmd: "osdcfg -act list", ParseJCP: ParseOSDCfg});
        GetJCP({cmd: "osdstylecfg -act list", ParseJCP: ParseOSDStyleCfg});
        GetJCPList({cmd: "osdstrcfg -act list", ParseJCP: ParseOsdStrCfg});
    }
})

var show = 0;
var wndWidth = 350; //视频宽度
var wndHeight = 250; //视频高度
var veWidth=1920, veHeight=1080; //用插件获取到的尺寸
function showVideo(){
    if(g_is_msie){
      $("#objects").html('<object id="IPCamera" name="IPCamera" CLASSID="CLSID:2319F6E6-ABD3-4b68-BADF-05D8796FA072" width="'+wndWidth+'" height="'+wndHeight+'">></object>');
    }else{
      $("#objects").html('<object id="IPCamera" name="IPCamera" type="application/npipcam" width="'+wndWidth+'" height="'+wndHeight+'">></object>');
    }
    document.IPCamera.IPCSetWindowMode(1);
    var type = GetCookieByKey("ljtypes");
    type = type== -1?1:type;
    document.IPCamera.IPCStartPreviewEx(0, GetCookieByKey("url"), 0, parseInt(type), GetCookieByKey("rtspport"),GetCookieByKey("user"), GetCookieByKey("passwd"), GetCookieByKey("stream"),"V2.00");
}

function showTab(s){
  if(show == s){
    return;
  }
  show = s;
  if(s == 0){
    $("#divExtend").removeClass('spanTabActive');
    $("#divBasic").addClass('spanTabActive');
    $("#tbExtend").hide();
    $("#tbBasic").show();
  }else if(s == 1){
    $("#divBasic").removeClass('spanTabActive');
    $("#divExtend").addClass('spanTabActive');
    $("#tbBasic").hide();
    $("#tbExtend").show();
  }
}

function ParseOSDStyleCfg(jcpstr){
    try{
        $("#colorOption").val(parseInt(jcpstr.colormode));//宽高暂时为实现
    }
    catch(e){}
}


function ParseOSDCfg(jcpstr)
{
    try
    {
        // 名称
        $("#name").val(jcpstr.name);
        $("input[name='namechk'][value=" + parseInt(jcpstr.nameen) + "]").attr("checked",true);
        position[0][0] = jcpstr.nameleft;
        position[0][1] = jcpstr.nametop;;

        // 星期
        $("input[name='weekchk'][value=" + parseInt(jcpstr.osdweek) + "]").attr("checked",true);
        //日期格式
        $("#dateformat").val(jcpstr.dateformat);
        
        // 时间
        $("input[name='timechk'][value=" + parseInt(jcpstr.timeen) + "]").attr("checked",true);
        position[1][0] = jcpstr.timeleft;
        position[1][1] = jcpstr.timetop;

        // 码率
        $("input[name='bpschk'][value=" + parseInt(jcpstr.bpsen) + "]").attr("checked",true);
        position[2][0] = jcpstr.bpsleft;
        position[2][1] = jcpstr.bpstop;

        document.IPCamera.IPCSetMDModeEx(0, true);
        if(parseInt(jcpstr.nameen)==1){
            document.IPCamera.IPCSetMDAreaRectEx(0,
                                                    position[0][0] * wndWidth / veWidth,
                                                    position[0][1] * wndHeight / veHeight,
                                                    position[0][0] * wndWidth / veWidth,
                                                    position[0][1] * wndHeight / veHeight);
            document.IPCamera.IPCShowMDAreaEx(0, true);
            document.IPCamera.IPCSetMDAreaTitleEx(0, "Name");
        }
        else
        {
            document.IPCamera.IPCShowMDAreaEx(0, false);
        }

        if(parseInt(jcpstr.timeen)==1){
            document.IPCamera.IPCSetMDAreaRectEx(1,
                                                    position[1][0] * wndWidth / veWidth,
                                                    position[1][1] * wndHeight / veHeight,
                                                    position[1][0] * wndWidth / veWidth,
                                                    position[1][1] * wndHeight / veHeight);
            document.IPCamera.IPCShowMDAreaEx(1, true);
            document.IPCamera.IPCSetMDAreaTitleEx(1, "Time");
        }
        else
        {
            document.IPCamera.IPCShowMDAreaEx(1, false);
        }

        if(parseInt(jcpstr.bpsen)==1){
            document.IPCamera.IPCSetMDAreaRectEx(2,
                                                    position[2][0] * wndWidth / veWidth,
                                                    position[2][1] * wndHeight / veHeight,
                                                    position[2][0] * wndWidth / veWidth,
                                                    position[2][1] * wndHeight / veHeight);
            document.IPCamera.IPCShowMDAreaEx(2, true);
            document.IPCamera.IPCSetMDAreaTitleEx(2, "bps");
        }
        else
        {
            document.IPCamera.IPCShowMDAreaEx(2, false);
        }
    }
    catch(e){return e;}
}

var maxOsdName = 8;//最大叠加的OSD个数
function ParseOsdStrCfg(jcpstr)
{
    try
    {
        var extArr = jcpstr.split("#");
        for(var i=0; i<maxOsdName;i++){
            var extObj = parse_jcp_content(extArr[i]);
            $("#check_" + i).prop("checked", (parseInt(extObj.enable) == 0 ? false : true));
            $("#xpos_" + i).val(extObj.left);
            $("#ypos_" + i).val(extObj.top);
            $("#osd_" + i).val(extObj.content);

            if(i == 0) {
                $("input[name='osdchk'][value=" + parseInt(extObj.enable) + "]").attr("checked",true);
                $("#osdname").val(extObj.content);  
                if(i == 0){
                    $("#osd_font").val(extObj.size);
                }
                position[3][0] = extObj.left;  
                position[3][1] = extObj.top;      

                if(parseInt(extObj.enable)==1){
                    document.IPCamera.IPCSetMDAreaRectEx(3,
                                                        position[3][0] * wndWidth / veWidth,
                                                            position[3][1] * wndHeight / veHeight,
                                                            position[3][0] * wndWidth / veWidth,
                                                            position[3][1] * wndHeight / veHeight);
                    document.IPCamera.IPCShowMDAreaEx(3, true);
                    document.IPCamera.IPCSetMDAreaTitleEx(3, "osd");
                }
                else
                {
                    document.IPCamera.IPCShowMDAreaEx(3, false);
                } 
            }
        }
    }catch(E){}
}


function SetOSDPosition(type)
{
    if(type==0)
    {
        CommonSetOSDPosition("namechk", 0, "Name");
    }
    else if (type==1)
    {
        CommonSetOSDPosition("timechk", 1, "Time");
    }
    else if (type==2)
    {
       CommonSetOSDPosition("bpschk", 2, "bps");
    }
    else if (type==3)
    {
        CommonSetOSDPosition("osdchk", 3, "ods");
    }
    else if(type==-1){
        CommonSetOSDPosition("namechk", 0, "Name");
        CommonSetOSDPosition("timechk", 1, "Time");
        CommonSetOSDPosition("bpschk", 2, "bps");
        CommonSetOSDPosition("osdchk", 3, "ods");
    }
}

// SetOSDPosition 方法中提取公用方法
function CommonSetOSDPosition(inputName, positionItem, AreaTitleEx){
    if (parseInt($('input:radio[name="' + inputName + '"]:checked').val())==1)
    {
        document.IPCamera.IPCSetMDModeEx(positionItem, true);
        document.IPCamera.IPCSetMDAreaRectEx(positionItem,
                                                position[positionItem][0] * wndWidth / veWidth,
                                                position[positionItem][1] * wndHeight / veHeight,
                                                position[positionItem][0] * wndWidth / veWidth,
                                                position[positionItem][1] * wndHeight / veHeight);
        document.IPCamera.IPCShowMDAreaEx(positionItem, true);
        document.IPCamera.IPCSetMDAreaTitleEx(positionItem, AreaTitleEx);
    }
    else
    {
        document.IPCamera.IPCShowMDAreaEx(positionItem, false);
    }
}

/*====================================================================
    保存页面参数 functions 
====================================================================*/
function UpdatePosition()
{
    var ret = 0;

    if(!CommonUpdatePosition("namechk", 0, 0))
    {
        return false;
    }

    if(!CommonUpdatePosition("timechk", 1, 1))
    {
        return false;
    }

    if(!CommonUpdatePosition("bpschk", 2, 2))
    {
        return false;
    }

    
    if(!CommonUpdatePosition("osdchk", 3, 3))
    {
        return false;
    }

    return true;
}

function CommonUpdatePosition(inputName, positionItem, wndRect)
{
    if($('input:radio[name="'+inputName+'"]:checked').val())
    {
        winPosStr = document.IPCamera.IPCGetVideoWndRect(wndRect);
        ret = GetRtspKeyStr(winPosStr, "left");
        if (ret == -2)
        {
            return false;
        }
        position[positionItem][0] = parseInt(parseInt(ret) * veWidth / wndWidth);
        position[positionItem][0] = position[positionItem][0] > 1920 ? 1920 : position[positionItem][0];

        ret = GetRtspKeyStr(winPosStr, "top");
        if (ret == -2)
        {
            return false;
        }
        if(parseInt(ret) > parseInt(wndHeight / 2))
        {
            ret = parseInt(ret) + 30;
        }
        position[positionItem][1] = parseInt(parseInt(ret) * veHeight / wndHeight);
        position[positionItem][1] = position[positionItem][1] > 1080 ? 1080 : position[positionItem][1];
        return true;
    }
}


function SaveBasic()
{
    var myRegExp = new RegExp("[\\u4e00-\\u9fa5]+\\s+[\\u4e00-\\u9fa5]+|[\\w]+\\b\\s+[\\w]+");  //匹配name中是否含有空格并替换为''
    var nameen = parseInt($('input:radio[name="namechk"]:checked').val());
    var osdweek = parseInt($('input:radio[name="weekchk"]:checked').val());
    var dateformat = $("#dateformat").val();
    var timeen = parseInt($('input:radio[name="timechk"]:checked').val());
    var bpsen = parseInt($('input:radio[name="bpschk"]:checked').val());
    var color = parseInt($("#colorOption").find("option:selected").val());

    var name = $("#name").val();

    var myRegExp = /["'<>%;)(&+“”\[\]!！？?\\#￥$^；【】。]/;
    if(myRegExp.test(name)){
          parent.paramFailTip(IDC_NAME+IDC_NOT_CONTAIN+"\"'<>%;)(&+“”[]!！？?\\#￥$^；【】。");
          window.focus();
          return;
    }

    if (0 >= name.length || 36 < name.length)
    {
        parent.paramFailTip(IDC_NAME_MSG + IDC_NAME_CONTENT_36);
        window.focus();
        return false;
    }

    var osd = $("#osdname").val();
    if(myRegExp.test(osd)){
          parent.paramFailTip(IDC_OSD_NAME+IDC_NOT_CONTAIN+"\"'<>%;)(&+“”[]!！？?\\#￥$^；【】。");
          window.focus();
          return;
    }

    if(12 < osd.length)
    {
        parent.paramFailTip(IDC_NAME_DIS_DIEJIA + IDC_NAME_CONTENT);
        window.focus();
        return;
    }
    var osd_font = $("#osd_font").val();
    var osdenable = parseInt($('input:radio[name="osdchk"]:checked').val());

    
    //获取OSD位置信息
     UpdatePosition();
     var jcpstr = "osdcfg -act set ";
     jcpstr += " -nameen " + nameen + " -nameleft " + position[0][0] + " -nametop " + position[0][1] + " -name \"" + name+"\"";
     jcpstr += " -osdweek " + osdweek;
     jcpstr += " -dateformat " + dateformat;
   
     GetJCPList({cmd: jcpstr});

     var jcpstr2 = "osdcfg -act set ";
     jcpstr2 += " -timeen " + timeen + " -timeleft " + position[1][0] + " -timetop " + position[1][1];
     GetJCPList({cmd: jcpstr2});

     var jcpstr3 = "osdcfg -act set ";
     jcpstr3 += " -bpsen " + bpsen + " -bpsleft " + position[2][0] + " -bpstop " + position[2][1];
     GetJCPList({cmd: jcpstr3});

     var jcpstr4 = "osdstylecfg -act set ";
     jcpstr4 += " -colormode " + color;
     GetJCPList({cmd: jcpstr4});

     var jcpstr5 = "osdstrcfg -act set -size " + osd_font + " -index 0 -enable " + osdenable + " -content \"" + osd + "\" -left " + position[3][0] + " -top " + position[3][1];
     GetJCPList({cmd: jcpstr5});

     parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
     window.focus();
     return 0;
}

var position = [[0, 0], [0, 0], [0, 0], [0,0]];    //name, time, bps,osd

function SaveOsdStr(index)
{  
    
    if (index < 0 || index > maxOsdName)
    {
        return;
    }
    var osd = $("#osd_" + index).val();

    var myRegExp = /["'<>%;)(&+“”\[\]!！？?\\#￥$^；【】。]/;
    if(myRegExp.test(osd)){
          alert(IDC_OSD_NAME+IDC_NOT_CONTAIN+"\"'<>%;)(&+“”[]!！？?\\#￥$^；【】。");
          window.focus();
          return;
    }

    if(12 < osd.length)
    {
        alert(IDC_OSD_TILTE);
        window.focus();
        return;
    }

    var enable = $("#check_"+index).is(":checked") ? 1 : 0;
    var x = $("#xpos_" + index).val();
    var y = $("#ypos_" + index).val();

    var chnsX= 1919, chnsY = 1079;

    if(x > chnsX || y > chnsY)
    {
        alert(IDC_GET_REC_PROMPT+IDC_X+chnsX);
        window.focus();
    }
    else if(isNaN(x)|| isNaN(y))
    {
        alert(IDC_DIEJIA);
        window.focus();
    }
    else
    {
        var jcpstr = "osdstrcfg -act set -index " + index + " -enable " + enable + " -content \"" + osd + "\" -left " + x + " -top " + y;
        
        GetJCPList({cmd: jcpstr,ParseJCP:function(result){
            if(result != "Error")
            {
                alert(IDC_MSGBOX_SAVEOK);
                window.focus();
            }
            else{
                alert(IDC_MSGBOX_SAVEFAIL);
                window.focus();
            }
        },async:false});
    }
}
