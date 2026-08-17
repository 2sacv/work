var JcpController;

JcpController = (function() {
  JcpController.send = function(message) {
    var jcp;
    jcp = new JcpController(message);
    return jcp.send();
  };

  JcpController.getStr = function(message) {
    var jcp;
    jcp = new JcpController(message);
    return jcp.getStr();
  }

  function JcpController(message) {
    this.message = encodeURIComponent(message);
  }

  JcpController.prototype.send = function() {
    var defer,
      _this = this;
    defer = $.Deferred();

    $.ajax({url:"jcp?jcpcmd=" + this.message,cache:false}).done(function(data) {
      var result;
      result = _this.parse_jcp_result(data);
      if (result.success) {
        return defer.resolve(result.data);
      } else {
        return defer.reject(result.data);
      }
    }).fail(function() {
      return "Error";
    });
    return defer.promise();
  };

  JcpController.prototype.parse_jcp_result = function(result) {
    var jcp_content, jcp_result, result_type;
    result = $.trim(result);
    jcp_result = $.trim(result.substring(13, result.length - 2));
    result_type = 'error';
    if (jcp_result.match(/^\[Success\]/)) {
      result_type = 'success';
    }
    jcp_content = jcp_result.replace(/^\[\w*\]/, '');
    if (result_type === 'success') {
      return {
        success: true,
        data: JcpController.parse_jcp_content(jcp_content)
      };
    } else {
      return {
        success: false,
        data: jcp_content.replace(/;$/, '')
      };
    }
  };

  JcpController.parse_jcp_content = function(content, options) {
    var i, info, key, place, regS, result, value, _i, _len;
    options = $.extend({
      spliter1: '=',
      spliter2: ';'
    }, options);
    result = {};
    regS = new RegExp(options.spliter1, "gi");
    place = content.replace(regS, options.spliter2);
    info = place.split(options.spliter2);
    for (i = _i = 0, _len = info.length; _i < _len; i = _i += 2) {
      value = info[i];
      key = $.trim(value);
      value = $.trim(info[i + 1]);
      if (key !== '') {
        result[key] = value;
      }
    }
    return result;
  };

   JcpController.prototype.getStr = function() {
    var defer,
      _this = this;
    defer = $.Deferred();
    $.ajax({url:"jcp?jcpcmd=" + this.message,cache:false}).done(function(result) {
        result = $.trim(result);
        var jcp_result = $.trim(result.substring(13, result.length - 2));
        if (jcp_result.match(/^\[Success\]/)) {
          return defer.resolve(jcp_result.split("[Success]")[1]);
        }else{
          return defer.reject(jcp_result);
        }
    }).fail(function() {
      "Error";
    });
    return defer.promise();
  };

  return JcpController;

})();
