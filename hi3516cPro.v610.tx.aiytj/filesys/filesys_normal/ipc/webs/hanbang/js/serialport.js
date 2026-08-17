$(function(){
        var lf = $.cookie("loginflag_"+g_hostname);
        if (null === lf)
        {
           parent.location.href = "/login.asp";
        }else{
            _init_port_setting();
        }
    })

   function _init_port_setting(){
    try{
        var boardmode = parseInt(GetCookieByKey("boardmode"));
        if (5 == parseInt(boardmode) || 18 == parseInt(boardmode)){
            $("#tabs-10 select").attr("disabled", true);
        }
    
        //球机地址初始化
        for (i = 1; i < 256; i++){
            $("#selAddr").append('<option value="' + i + '">' + i + '</option>');
        }

        GetJCPList({cmd: "ptzcfg -act list", ParseJCP: function(jcpstr){
            if(jcpstr != 'Error'){         
                 //$("#comtype").val(GetRtspKeyStr(jcpstr, 'type'));  
                 $("#stopbits").val(GetRtspKeyStr(jcpstr, 'stop')); 
                 $("#databits").val(GetRtspKeyStr(jcpstr, 'data')); 
                 $("#checktype").val(GetRtspKeyStr(jcpstr, 'parity')); 
                 $("#baudrate").val(GetRtspKeyStr(jcpstr, 'baud')); 
                 $("#selAddr").val(GetRtspKeyStr(jcpstr, 'addr')); 
                 /*for (var i = 0; i < 255; i ++) {
                    var ret = GetRtspKeyStr(jcpstr, "ps" + i);
                    if (ret == -2) {
                        continue;
                    }
                    $("#selProtocol").append('<option value="' + ret + '">' + ret + '</option>');
                 }*/
                 $("#selProtocol").append('<option value="PELCO_D">PELCO_D</option>');
                 $("#selProtocol").val(GetRtspKeyStr(jcpstr, 'protocol'));
            }
               
        }});
        
    }
    catch(E){}
}

function SaveSerialPort(){
    var type = $("#comtype").val();
    var stop = $("#stopbits").val();
    var data = $("#databits").val();
    var parity = $("#checktype").val();
    var baud = $("#baudrate").val();
    var addr = $("#selAddr").val();
    var protocol = $("#selProtocol").val();
    
    var jcpstr = "ptzcfg -act set  -stop " + stop + " -data " + data + " -parity " + parity + " -baud " + baud + " -addr " + addr + " -protocol " + protocol;
    GetJCP({cmd: jcpstr});
    
    parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
    window.focus();
    return 0;
}