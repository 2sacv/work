var stream = [
    'QCIF','CIF','D1',
    '720P','UVGA','1080P',
    'QVGA','VGA','960P','3M',
    '180p','360p','4M','5M','4M_Dahua','8M','4M_1'
    ]

var stream_size = [
    [144,176],[352,288],[720,576],
    [1280,720],[1600,1200],[1920,1080],
    [320,240],[640,480],[1280,960],[2304,1296],
    [240,180],[480,360],[2560, 1440],[2880,1620],[2592,1944],[3840,2160],[2688,1512]];

StreamCfg = function(){
    Devcfg();
}

Devcfg = function(){
    Set_cookie("stream", "stream1");
    GetJCP({cmd:"devvecfg -act list", ParseJCP: function(result){
        if(result[0]){
            Set_cookie("master_stream",result[0]['vencsize']);
            Set_cookie("master_enb",result[0]['enable']);
        }
        if(result[1]){
            Set_cookie("slave_stream",result[1]['vencsize']);
            Set_cookie("slave_enb",result[1]['enable']);
        }else{
            deleteCookie("slave_stream");
            deleteCookie("slave_enb");
        }
        play_stream();
    }})
}

play_stream = function(){
      "stream2" == $.cookie("main_stream") ? player_slave_size():player_master_size();
}

player_master_size = function(){
    Set_cookie("playsize_width",stream_size[$.cookie("master_stream")][0])
      Set_cookie("playsize_height",stream_size[$.cookie("master_stream")][1])
}

player_slave_size = function(){
    Set_cookie("playsize_width",stream_size[$.cookie("slave_stream")][0])
      Set_cookie("playsize_height",stream_size[$.cookie("slave_stream")][1])
}
