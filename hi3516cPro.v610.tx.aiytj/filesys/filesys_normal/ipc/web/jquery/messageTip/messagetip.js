(function( $, undefined ) {
    $.messageTip = {
        show: function(config){
            config = $.extend({
                title : '',
                content : '&#160;',
                type : 'alert'
            },config);
            var html = '<div class="messageTip corner-all" tabindex="-1">'+
                    '<div class="messageTip-content widget-content corner-bottom">'+
                        '<div class="messageTip-content-body">'+config.content+'</div>' +
                    '</div>'+
                '</div>';
            var messageTip = $(html).appendTo(document.body).css('z-index', 99000).hide();
            var result = {d:messageTip,l:config.onClose};
            
            messageTip.slideDown('slow');
            
            var timer;
            function timeout(time){
            	timer = setTimeout(function(){
                    $.messageTip._close(result);
                },time);
            }
            if(config.timeout){ //定时关闭
              timeout(config.timeout);
            }
            
            messageTip.bind('mouseover', function(){
            		clearTimeout(timer);
            }).bind('mouseout', function(){
            	if(timer){
            		timeout(config.timeout);
            	}
            });
            return messageTip;
        },
        _close : function(result){
            result.d.slideUp('slow');
            if(result.l){
                result.l();
            }
            setTimeout(function(){
                result.d.remove();
            },1000);
        }
    };
}(jQuery));