$(document).ready(function(){
    _init_load()
    _init_player()
    _init_click()
})

var roi_index = 0;
_init_click = function(){
    $("#roi_option").change(function(){
        roi_index = parseInt($("#roi_option").val())
    })

    $("#roi_select").click(function(){
        document.IPCamera.IPCSetMDModeEx(0, true)
        document.IPCamera.IPCSetMDAreaRectEx(parseInt(roi_index),100,100,300,180)
        document.IPCamera.IPCShowMDAreaEx(parseInt(roi_index), true)
        document.IPCamera.IPCSetMDAreaTitleEx(parseInt(roi_index),IDC_MENU_ROIAREA)
    })

    $("#roi_seting").click(function(){
        var a = _get_roiarea()
        var jcpstr = "roicfg -act set -id " + roi_index + " -enable 1 -left " + a.left + " -top " + a.top + " -right " + a.right + " -bottom " + a.bottom;
        GetJCP({cmd: jcpstr,ParseJCP: function(result){
            var prompt = IDC_SET + IDC_FAIL;
            if(result != "Error"){
                prompt =  IDC_SET + IDC_SUCCESS;
                document.IPCamera.IPCShowMDAreaEx(roi_index,false)
            }
            alert(prompt);
        }})
    })

    $("#roi_delete").click(function(){
        var jcpstr = "roicfg -act set -id " + roi_index + " -enable 0";
        GetJCP({cmd: jcpstr,ParseJCP: function(result){
            var prompt = IDC_DEL + IDC_FAIL;
            if(result != "Error"){
                prompt =  IDC_DEL + IDC_SUCCESS;
                document.IPCamera.IPCShowMDAreaEx(roi_index,false)
            }
            alert(prompt);
        }})
    })
}

_get_roiarea = function(){
    var rect = document.IPCamera.IPCGetVideoWndRect(roi_index)
    var a = parse_jcp_content(rect)

    var left = parseInt(a.left * 1920 / $("#roi_player").width())
    left == 1920 ? 1919 : left
    var top = parseInt(a.top * 1080 / $("#roi_player").height())
    top == 1080 ? 1079 : top
    var right = parseInt(a.right * 1920 / $("#roi_player").width())
    right == 1920 ? 1919 : right
    var bottom = parseInt(a.bottom * 1080 / $("#roi_player").height())
    bottom == 1080 ? 1079 : bottom

    var b = {"left":left,"top":top,"right":right,"bottom":bottom}
    return b
}

_init_load = function(){
        $("#roi_tab").html(IDC_MENU_ROIAREA);
        $("#roi_select").html(IDC_SELECT_TABLE)
        $("#roi_seting").html(IDC_SET)
        $("#roi_delete").html(IDC_DEL)

        $("#roi_tabs").tabs()
        var s  = "";
        for(var i = 0 ;i < 8 ; i++){
            s += "<option value="+i+">"+(i+1)+ "</option>"
        }
        $("#roi_option").append(s)
}

_init_player = function(){
    $("#roi_player").html('<object id="IPCamera" name="IPCamera" CLASSID="CLSID:2319F6E6-ABD3-4b68-BADF-05D8796FA072" width="500" height="370"></object>');
    document.IPCamera.IPCSetWindowMode(1);
    document.IPCamera.IPCStartPreviewEx(0, GetCookieByKey("url"), 0, 1, GetCookieByKey("rtspport"),GetCookieByKey("user"), GetCookieByKey("passwd"), "stream1","V2.00");
}
