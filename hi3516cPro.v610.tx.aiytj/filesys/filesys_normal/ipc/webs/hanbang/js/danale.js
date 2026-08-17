$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
        initDanale();
    }
})

function switchDanale(){
    try{
        var enable = $('input:radio[name="danale_switch"]:checked').val();
        var jcpstr = "danalecfg -act set -enable " + enable ;
        GetJCP({cmd: jcpstr});
    }catch(e){};
}

function initDanale(){
    try{
        GetJCP({cmd: "danalecfg -act list", ParseJCP: function(jcpObj){
            if(jcpObj !== 'Error'){
                $("#danale_devid").html(jcpObj.danale_id);
                $("#danale_status").html(jcpObj.status==0?IDC_HXHT_CONNECT_STATUS_OFFLINE:IDC_HXHT_CONNECT_STATUS_ONLINE);
				$("#danale_path").attr("src","/"+jcpObj.bmp_path + "?" + Date.parse(new Date()));
                $("input[name='danale_switch'][value=" + parseInt(jcpObj.enable) + "]").prop("checked",true);
                
            }
        }});
        window.focus();
    }catch(e){};
}