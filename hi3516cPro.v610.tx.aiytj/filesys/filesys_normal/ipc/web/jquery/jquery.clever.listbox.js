/* jQuery ListBox Plugin
*  Version: 1.1.0
*  Author: Joseph Chan(think8848)
*  Contact: QQ: 166156888 Blog: http://think8848.cnblogs.com
*  Company: http://www.cleversoft.com */
(function ($) {
    $.fn.listbox = function (options) {
        var self = this;
        var el = self.get(0);
        if (typeof options == 'string') {
            var list = el.list;
            if (options in list) {
                var args = $.makeArray(arguments).slice(1);
                return list[options].apply(list, args);
            } else {
                throw 'clever listbox not contains such function';
            }
        }
        function build() {
            return $.extend({
                removeItem: el.list ? $.proxy(el.list.removeItem, el.list) : null,
                addRange: el.list ? $.proxy(el.list.addRange, el.list) : null,
                addItem: el.list ? $.proxy(el.list.addItem, el.list) : null,
                removeRange: el.list ? $.proxy(el.list.removeRange, el.list) : null,
                getDatas: el.list ? $.proxy(el.list.getDatas, el.list) : null,
                getData: el.list ? $.proxy(el.list.getData, el.list) : null,
                clear: el.list ? $.proxy(el.list.clear, el.list) : null,
                getSelected: el.list ? $.proxy(el.list.getSelected, el.list) : null,
                getNext: el.list ? $.proxy(el.list.getNext, el.list) : null,
                getLength: el.list ? $.proxy(el.list.getLength, el.list) : null,
                setSelection: el.list ? $.proxy(el.list.setSelection, el.list) : null,
                reload: el.list ? $.proxy(el.list.reload, el.list) : null,
                updateText: el.list ? $.proxy(el.list.updateText, el.list) : null,
                queryData: el.list ? $.proxy(el.list.queryData, el.list) : null
            }, self);
        }
        function fillData(data, ajaxsettings) {
            if (data) {
                if (typeof data == 'string') {
                    if (data.indexOf("?") != -1) {
                        data = data + "&" + new Date().getTime();
                    } else {
                        data = data + "?" + new Date().getTime();
                    }
                    $.ajax(data, $.extend({
                        type: 'GET',
                        dataType: 'json',
                        success: function (data) {
                            var list = el.list;
                            list.addRange(data);
                        }
                    }, ajaxsettings));
                } else {
                    list.addRange(data);
                }
            }
        }
        if (el.list) { return build(); }
        var defaults = {
            dnd: false,
            dndscope: new Date().getTime(),
            height: 'auto',
            width: 'auto',
            data: [],
            multiselect: false,
            ajaxsettings: {},
            dblclick: null,
            click: null,
            selectchange: function (data) { return false; }
        };
        var options = $.extend(defaults, options || {});
        var ts = $(el);
        var list = {
            p: options,
            addRange: function (data) {
                for (var i = 0; i < data.length; i++) {
                    this.addItem(data[i]);
                }
            },
            selected: function () { return list.p.selectchange; },
            addItem: function (data) {
                var finallyData = data;
                if (data instanceof Array || (!('value' in data) || (!'text' in data))) {
                    finallyData.value = data[0];
                    finallyData.text = data[1];
                }
                var item = $('<li class="ui-menu-item ui-corner-all"><a id="' + finallyData.value + '" class="ui-corner-all" tabindex="-1">' + finallyData.text + '</a>');
                ts.append(item);
                $('a', item).click(function () {
                    var a = $(this);
                    var e = arguments[0] || window.event;
                    var parent = item.parent();
                    var items = $('>li', parent);
                    if (list.p.multiselect && e.ctrlKey) {
                        if (a.hasClass('ui-state-active')) {
                            a.removeClass('ui-state-active');
                        } else {
                            a.addClass('ui-state-active');
                        }
                    } else {
                        var selectedItem = $('a.ui-state-active', parent).parent();
                        $('a', parent).removeClass('ui-state-active');
                        if (selectedItem.size() == 0 || items.index(selectedItem) != items.index(item)) {
                            a.addClass('ui-state-active');
                        }
                    }
                    a.blur();
                    var current = $(this).parents('ul:first').get(0).list;
                    current.selected()(current.getSelected());
                    if (options.click)
                        options.click(finallyData.value,$(this).text());
                })
                .mouseover(function () {
                    if (!$(this).hasClass('ui-state-active')) {
                        $(this).addClass('ui-state-hover');
                    }
                }).mouseout(function () {
                    $(this).removeClass('ui-state-hover');
                }).dblclick(function () {
                	if (options.dblclick)
                		options.dblclick($(this).attr("id"));
                });
                if (list.p.dnd) {
                    item.draggable({
                        opacity: .5,
                        addClasses: false,
                        helper: 'clone',
                        cursor: 'move',
                        scope: list.p.dndscope,
                        drag: function (event, ui) {
                            $(this).parent().find('a').removeClass('ui-state-active');
                            ui.helper.width($(this).width());
                            $('a', ui.helper).attr('class', '').css('border-style', 'dashed').css('border-width', 'thin').css('font-weight', 'normal');
                        }
                    });
                    item.droppable({
                        scope: list.p.dndscope,
                        tolerance: 'pointer',
                        over: function (event, ui) {
                            var li = $(this);
                            if (li.find('.ui-icon-arrow-1-e').size() == 0) {
                                $('>a:first', li).prepend('<span class="ui-icon ui-icon-arrow-1-e"></span>');
                            }
                        },
                        out: function (event, ui) {
                            $('.ui-icon-arrow-1-e').remove();
                        },
                        drop: function (event, ui) {
                            $(this).before(ui.draggable);
                        },
                        deactivate: function (event, ui) {
                            $('.ui-icon-arrow-1-e').remove();
                        }
                    });
                }
            },
            removeRange: function (data) {
                if (data && data instanceof Array) {
                    for (var i = 0; i < data.length; i++) {
                        this.removeItem(data[i]);
                    }
                }
            },
            removeItem: function (data) {
                var key = data;
                if (data instanceof jQuery) {
                    key = this.getData(data).value;
                }
                else if (typeof data == 'object') {
                    key = data.value;
                }
                ts.find('a[id="' + key + '"]').remove();
            },
            getData: function (a) {
                if (a) {
                    if (!(a instanceof jQuery)) {
                        a = $(a);
                    }
                    return { value: a.attr('id'), text: a.text() };
                } else {
                    return null;
                }
            },
            getSelected: function () {
                var selected = $('a.ui-state-active', ts);
                var result = [];
                selected.each(function () {
                    result.push(list.getData(this));
                });
                return result;
            },
            getDatas: function () {
                var result = [];
                $('>li>a:first-child', ts).each(function () {
                    result.push(list.getData(this));
                });
                return result;
            },
            getNext: function() {
                if ($(ts).find("a").hasClass("ui-state-active") == false)
                {
                    return null;
                }
                var _this = $("a.ui-state-active");
                var nextall = $("a.ui-state-active").parent("li").nextAll();
                var nextitem = null;
                if (nextall.length == 0)
                {
                    nextitem = $(_this).parent().parent().children().children();
                }
                else
                {
                    nextitem = $("a.ui-state-active").parent("li").next().children();
                }
                $(nextitem[0]).addClass("ui-state-active");
                _this.removeClass("ui-state-active");
                return { value: $(nextitem[0]).attr('id'), text: $(nextitem[0]).text() };

            },
            getLength: function () {
                return $(ts).children("li").length;
            },

            setSelection: function (value) {
                $('a', ts).removeClass('ui-state-active').parent().css('margin-top', '');
                $('a[id="' + value + '"]', ts).addClass('ui-state-active');
            },
            clear: function () {
                ts.empty();
            },
            reload: function (options) {
                list.p = $.extend(list.p, options || {});
                list.clear();
                fillData(list.p.data, list.p.ajaxsettings);
            },
            updateText: function(options){
                $('a[id="' + options.value + '"]', ts).html(options.text);
            },
            queryData: function(id){
                var v = $('a[id="' + id + '"]', ts) && $('a[id="' + id + '"]', ts).text();
                return v!='' && v.length>0;
            }
        }
        el.list = list;
        self.addClass('cleverlistbox ui-menu ui-widget ui-widget-content ui-corner-all');
        if (typeof list.p.height == 'number' || (typeof list.p.height != 'string' && list.p.height.toLowerCase() != 'auto')) {
            self.css('overflow-y', 'auto');
            self.css('overflow-x', 'hidden');
            self.height(list.p.height);
        }
        if (typeof list.p.width == 'number' || (typeof list.p.width != 'string' && list.p.width.toLowerCase() != 'auto')) {
            self.width(list.p.width);
        } else {
            self.width(self.parent().width() - 2);
        }
        fillData(list.p.data, list.p.ajaxsettings);
        
        return build();
    }
})(jQuery);