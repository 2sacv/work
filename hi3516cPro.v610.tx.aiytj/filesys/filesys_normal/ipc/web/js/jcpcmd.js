GetJCP = function()
{
    if (arguments.length == 0)
    {
        return;
    }
    var ajax = new Array;

    ajax["url"] = "http://"+ (document.URL.split('//')[1]).split('/')[0] +"?jcpcmd=" + encodeURIComponent(arguments[0].cmd);

    ajax["async"] = true;
    if (typeof arguments[0].async == "boolean")
    {
        ajax["async"] = arguments[0].async;
    }
    var _timeout = arguments[0].timeout || 1900;
    
    ajax["ParseJCP"] = arguments[0].ParseJCP;
    var der = $.when($.ajax({type: "GET", url: ajax["url"], dataType: "script", async: ajax["async"],cache:false,
            timeout: _timeout}));
    der.done(function(a1){
        eval(a1);
        if (typeof ajax["ParseJCP"] == "function")
        {
            var arr = szJcpResult.split("[Success]");
            var szResult;
            if (1 >= arr.length)
            {
                ajax["ParseJCP"]("Error");
            }
            else
            {
                szResult = arr[1];
            }
            if(szResult.indexOf(";#") > 0)
            {
                      var j = szResult.split("#")
                      var content = []
                      for(var i = 0; i< j.length;i++){
                          if(j[i].length < 3){ continue; }
                          content.push(parsejcp_array_content(j[i]))
                      }
                      ajax["ParseJCP"](content);
            }
            else{
                ajax["ParseJCP"](parse_jcp_content(szResult));
            }
        }
    }).fail(function(a2){
        ajax["ParseJCP"]("Error");
    });
}

parsejcp_array_content = function(content,options){
    var key, value, _i, _len;
    var options = $.extend({
      spliter1: '=',
      spliter2: ';'
    }, options);

    var result = {};
    var regS = new RegExp(options.spliter1, "gi");
    var place = content.replace(regS, options.spliter2);
    var info = place.split(options.spliter2);

    if (info.length < 3) { return;}
    for (var i = _i = 0, _len = info.length; _i < _len; i = _i += 2) 
    {
      key = $.trim(info[i]);
      value = $.trim(info[i + 1]);
      if (key !== '') {
        result[key] = value;
      }
    }
    return result;
}

parse_jcp_content = function(content, options){
    if(content == "Error"){ return content;}
    
    var options = $.extend({spliter1: '=', spliter2: ';'},options);
    var result = {}
    var regS = new RegExp(options.spliter1, "gi");
    var place = content.replace(regS, options.spliter2);
    var info = place.split(options.spliter2);

    var key, value, _i, _len;
    for (var i = _i = 0, _len = info.length; _i < _len; i = _i += 2){
      key = $.trim(info[i]);  
      value = $.trim(info[i + 1]);  
      if (key !== '')
      {    
          result[key] = value;  
      }
    }
    return result;
}


function GetJCPList()
{
    if (arguments.length == 0)
    {
        return;
    }
    var ajax = new Array;

    ajax["url"] = "http://"+ (document.URL.split('//')[1]).split('/')[0] +"?jcpcmd=" + encodeURIComponent(arguments[0].cmd);

    ajax["async"] = true;
    if (typeof arguments[0].async == "boolean")
    {
        ajax["async"] = arguments[0].async;
    }
    
    var _timeout = arguments[0].timeout || 1900;
    var logType = encodeURIComponent(arguments[0].logType)||0;
    
    ajax["ParseJCP"] = arguments[0].ParseJCP;
    
    var der = $.when($.ajax({type: "GET", url: ajax["url"], dataType: "script", async: ajax["async"],cache:false,timeout: _timeout}));
    
    der.done(function(a1){
        eval(a1);
        if (typeof ajax["ParseJCP"] == "function")
        {
            var arr = szJcpResult.split("[Success]");
            var szResult;
            if (1 >= arr.length)
            {
                if(logType==1){
                    szResult = szJcpResult.split("[Error]")[1];
                }else{
                    ajax["ParseJCP"]("Error");
                }
            }
            else
            {
                szResult = arr[1];
            }
            ajax["ParseJCP"](szResult);
        }
    }).fail(function(a2){
        ajax["ParseJCP"]("Error");
    });;
}