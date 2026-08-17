$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
        initAliyun();
    }
})

function switchAliyun(){
    try{
        var enable = $('input:radio[name="aliyun_switch"]:checked').val();
        var jcpstr = "alicfg -act set -enable " + enable ;
        GetJCP({cmd: jcpstr});
    }catch(e){};
}

function initAliyun(){
    try{
        GetJCP({cmd: "aliconfcfg -act list", ParseJCP: function(jcpObj){
            if(jcpObj !== 'Error'){
                $("#aliyun_devid").html(jcpObj.product_key+"@"+jcpObj.device_name);
                $("#aliyun_status").html(jcpObj.connect_status==0?IDC_HXHT_CONNECT_STATUS_OFFLINE:IDC_HXHT_CONNECT_STATUS_ONLINE);
								$("#aliyun_path").attr("src","/"+jcpObj.bmp_path + "?" + Date.parse(new Date())); 
            } 
        }});
       /* 
        GetJCP({cmd: "alicfg -act list", ParseJCP: function(jcpObj){
            if(jcpObj !== 'Error'){
                $("input[name='aliyun_switch'][value=" + parseInt(jcpObj.enable) + "]").prop("checked",true);   
            } 
        }});
        */
        
        window.focus();
    }catch(e){};
}
