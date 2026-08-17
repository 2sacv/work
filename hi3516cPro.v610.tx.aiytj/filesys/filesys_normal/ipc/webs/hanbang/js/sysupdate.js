$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
      _init_progress();
    }
})
var sliderProgress;

   _init_progress = function(){
    $("#progress_lab").text("2%");
    sliderProgress = new SliderModules({
        targetId: "progress",
        min: 0,
        max: 100
      }); 
      sliderProgress.create();
      sliderProgress.onchange = function () {
          $('#progress_lab').text(sliderProgress.getValue()+ "%" );
      };
}

function changeFilepath(){
    var filepath = document.getElementById("filepath").value;
    $("#trFileName").show();
    var l = filepath.lastIndexOf("\\");
    $("#fileName").html(filepath.substring(l+1));
}

function IframeUpdate(){
    var file = $("#filepath").val();
    var file_length = file.split(".");
    if(file == "" || file_length[file_length.length-1]!= 'tgz')
    {
        parent.paramFailTip(IDC_UPDATE_TYPE_ERROR);
        return false;
    }
    else
    {
        if(confirm(IDC_MSGBOX_CONFIRM+IDC_UPDATE)){
            document.frmUpdate.action="/webs/updateCfg";
            frmUpdate.submit();
            $("#progress_div").show();
            $("#restart_prompt_div").show();
            sliderProgress.wsetValue(1);
            $('#progress_lab').text("1%" );
            //$("body").append('<div style="z-index:1;background-color:#FFF;filter: alpha(opacity=50);-moz-opacity:0.5;-khtml-opacity: 0.5;opacity: 0.5;width:100%;height:100%; position:absolute;left:0px;top:0px; "></div>'); 
            window.setTimeout(get_progress,3000);
        }
        window.focus();
    }
}

var update_time = "";
get_progress = function(){
    GetJCP({cmd: "update -act list",ParseJCP: function(result){
        $("#progress_div").show();
      
        if('undefined' === typeof(result.progressbar) || 100 == parseInt(result.progressbar)){
            _to_be_continueted_pregress();
        }
        else if(100 < parseInt(result.progressbar) && 110 > parseInt(result.progressbar)){
            $('#progress_div').hide()
            clearTimeout(update_time)
            $("#restart_prompt").html(IDC_PACKAGE_ERROR+", " +IDC_SERVER_RESTART_PROMPT)
            _error_progress()
        }
        else if(parseInt(result.progressbar) == 111){
          $('#progress_div').hide()
          clearTimeout(update_time)
          $("#restart_prompt").html(IDC_SCRIPT_UPGRADE_ERROR+", " +IDC_SERVER_RESTART_PROMPT)
          _error_progress()
        }
        else{
            if (parseInt(result.progressbar) >= 0 && parseInt(result.progressbar) < 100) {
                var p = parseInt(result.progressbar/10,10)+1;
                sliderProgress.wsetValue(p);
                $('#progress_lab').text(p+ "%" );
            }
            update_time = setTimeout(get_progress,3000)  
        }
    }})
}

var error_p = 0, error_progress = 0, success_p = 0, success_progress = 0;
_error_progress = function(){
    $("#progress_div").show();
    sliderProgress.wsetValue(error_p);
    $('#progress_lab').text(error_p+ "%" );
    if(error_p == 100){
        $("#progress_div").hide()
        clearTimeout(error_progress)
        $("#restart_prompt_div").hide();
        deleteCookie("lastclickmenu");
        deleteCookie("loginflag_"+g_hostname);
        parent.location.href = "../login.asp";
    }
    else{
        error_p++;
        error_progress = setTimeout(_error_progress,350)    
    }
}

var be_continueted_pregress = 0, be_continueted = 10;
_to_be_continueted_pregress = function(){
    if(be_continueted == 100){
        $('#progress_div').hide()
        clearTimeout(be_continueted_pregress)
        $("#restart_prompt").html(IDC_UPDATE+IDC_SUCCESS+IDC_SERVER_RESTART_PROMPT)
        _success_progress()
    }else{
        be_continueted++;
        sliderProgress.wsetValue(be_continueted);
        $('#progress_lab').text(be_continueted+ "%" );
        be_continueted_pregress = setTimeout(_to_be_continueted_pregress,1100)
    }
}

_success_progress = function(){
    $("#progress_div").show();
    sliderProgress.wsetValue(parseInt(success_p));
    $('#progress_lab').text(success_p+ "%" );
    if(success_p == 100){
        $("#progress_div").hide()
        clearTimeout(success_progress)
        $("#restart_prompt_div").hide()
        deleteCookie("lastclickmenu");
        deleteCookie("loginflag_"+g_hostname);
        parent.location.href = "../login.asp";
    }
    else{
        success_p++;
        success_progress = setTimeout(_success_progress,810)
    }
}