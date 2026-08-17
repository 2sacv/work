(function($){
	var SelectTime;
	SelectTime = (function () {
		function SelectTime (element, options) {
			this.wrap = element;
			this.wrap.addClass("plug-timer-wrap");
			this.cellW = parseInt(this.wrap.innerWidth() / 25, 10);
			this.cellH = parseInt(this.wrap.innerHeight() / 8, 10);
			this.grid = null;
			this.cover = null;
			this.status = false;
			this.setting = $.extend({}, $.fn.selectTime.defaults, options);
		}

		SelectTime.prototype.init = function () {
			this.create();
			this.bind();

			if (this.setting.data) {
				this.setData(this.setting.data);
			}
		}

		SelectTime.prototype.create = function () {
			var strArr = [],
				i = 0,
				j = 0;

			this.wrap.children().remove();

			strArr.push('<table>');
			for (i = 0; i <= 7; i++) {
				strArr.push("<tr>");
				for (j = 0; j <= 24; j++) {
					if(i == 0){
						if (j > 0) {
							strArr.push("<td>" + (j - 1) + "</td>");
						} else {
							strArr.push("<td>"+IDC_TIME_ALL+"</td>");
						}
					} else {
						if (i > 0 && j == 0) {
							strArr.push("<td>" + this.setting.weekStr[i-1] + "</td>");
						} else {
							strArr.push("<td></td>");
						}
					}
				}
				strArr.push("</tr>");
			}
			strArr.push("</table>");

			this.grid = this.wrap.append(strArr.join("")).find(" > table")
			this.grid.addClass("plug-timer-grid");
			this.canvas = this.wrap.append("<div class='plug-timer-canvas'></div>").find("> .plug-timer-canvas");
			this.cover = this.wrap.append("<div class='plug-timer-cover'></div>").find("> .plug-timer-cover");

			if(/msie 6/i.test(navigator.userAgent)){
				this.grid.css("height", this.wrap.innerHeight() + "px");
				this.canvas.css("height",this.wrap.innerHeight() + "px");
			}
		};

		SelectTime.prototype.bind = function () {
			var me = this,
				cls = this.setting.cls,
				canvas = this.canvas,
				cover = this.cover,
				grid = this.grid,
				cellW = this.cellW,
				cellH = this.cellH,
				row,
				cols,
				tmpRow,
				tmpCols,
				x,
				y;
            var draged = false;
			canvas.click(function(e){
				var pos = getPos.call(this, e);//alert(pos.x);
				if (pos.x < cellW && pos.y < cellH) {  //全部
					if (grid.find("." + cls).length < 1) {  
						grid.find("tr:gt(0)").find("td:gt(0)").addClass(cls);
					} else {
						grid.find("." + cls).removeClass(cls);
					}
				} else if (pos.x < cellW && pos.y > cellH){ //周日-周六
					var cell = parseInt(pos.y/cellH);
					var flag = false;
					for(var i=1;i<=24;i++){
						flag = grid.find("tr:eq("+cell+")").find("td:eq("+i+")").hasClass(cls);
						if(flag)break;
					}
                    if (!flag) {  
						grid.find("tr:eq("+cell+")").find("td:gt(0)").addClass(cls);
					} else {
						grid.find("tr:eq("+cell+")").find("td:gt(0)").removeClass(cls);
					}
				} else if (pos.x > cellW && pos.y < cellH){ //0-23
					var cell = parseInt(pos.x/cellW);
					var flag = false;
					for(var i=1;i<=7;i++){
						flag = grid.find("tr:eq("+i+")").find("td:eq("+cell+")").hasClass(cls);
						if(flag)break;
					}
                    if (!flag) {  
						grid.find("tr:gt(0)").find("td:eq("+cell+")").addClass(cls);
					} else {
						grid.find("tr:gt(0)").find("td:eq("+cell+")").removeClass(cls);
					}
				}else{
                    if (draged) {
					    var x0 = parseInt(pos.y/cellH);
					    var y0 = parseInt(pos.x/cellW);
                        grid.find("tr:eq("+x0+")").find("td:eq("+y0+")").toggleClass(cls);
                    }

				}
			});

			canvas.mousedown(function(e){
				var pos = getPos.call(this, e);

				if (pos.x > cellW && pos.y > cellH) {
					me.status = true;
					this.setCapture && this.setCapture();
					x = pos.x;
					y = pos.y;
					cols = parseInt(x / cellW, 10);
					row = parseInt(y / cellH, 10);
					cover.show();
					if (x > cellW || y > cellH) {
						cover.css({
							"left": cols * cellW + "px",
							"top": row * cellH + "px",
							"height": "0px",
							"width": "0px"
						});
					}
				} else {
					me.status = false;
				}
			});

			canvas.mousemove(function (e){
				var pos = getPos.call(this, e),
					tx,
					ty;
                draged = true;

				if (me.status) {
					tx = pos.x;
					ty = pos.y;

					if(tx > x) {
						cover.css({
							"left": cols * cellW + "px",
							"right": ""
						});
					} else {
						cover.css({
							"left": "",
							"right": (25 - cols - 1) * cellW + "px"
						})
					}
					if(ty > y) {
						cover.css({
							"top": row * cellH + "px",
							"bottom": ""
						});
					} else {
						cover.css({
							"top": "",
							"bottom": (8 - row - 1) * cellH + "px"
						});
					}
					tmpCols = parseInt(tx / cellW, 10) <= 0 ? 1 : parseInt(tx / cellW, 10);
					tmpRow = parseInt(ty / cellH, 10) <= 0 ? 1 : parseInt(ty / cellH, 10);
					cover.css({
						"width":(Math.max(tmpCols, cols) - Math.min(tmpCols, cols) + 1) * cellW + "px",
						"height": (Math.max(tmpRow, row) - Math.min(tmpRow, row) + 1) * cellH + "px"
					});
				}
			});

			canvas.mouseleave(function(e){
				clear();
			});

			canvas.mouseup(function(e){
				clear();
			});

			function getPos (e) {
				return {
					x: e.offsetX ? e.offsetX : e.pageX - $(this).offset().left,
					y: e.offsetY ? e.offsetY : e.pageY - $(this).offset().top
				}
			};

			function clear(){
				var trs, tds, i, j, o;

				if (me.status) {
					me.status = false;
					if (!row || !cols || !tmpRow || !tmpCols) {
						return false;
					}

					trs = grid.find("tr");
					for (i = Math.min(row, tmpRow); i <= Math.max(row, tmpRow); i++) {
						tds = trs[i].children;
						for (j = Math.min(cols, tmpCols); j <= Math.max(cols, tmpCols); j++) {
							$(tds[j]).toggleClass(cls);
						}
					}
					
					o = canvas[0];
					o.releaseCapture && o.releaseCapture();
					canvas.blur();
					cover.hide();
					me.status = false;
					row = null;
					cols = null;
					tmpRow = null;
					tmpCols = null;
                    draged = false;
				}else{
					o = canvas[0];
					o.releaseCapture && o.releaseCapture();
					canvas.blur();
					cover.hide();
				}
			};
		};

		SelectTime.prototype.getData = function () {
			var grid = this.wrap.find("table").eq(0),
				cls = this.setting.cls,
				data = [],
				tempStr = [];

			grid.find("tr:gt(0)").each(function(index){
				tempStr = [];
				$(this).find("td:gt(0)").each(function () {
					if ($(this).hasClass(cls)) {
						tempStr.push("1");
					} else {
						tempStr.push("0");
					}
				});
				str = parseInt(tempStr.reverse().join(""),2).toString(10)
				if(str > 0)
				{
					str = parseInt(str) + 2147483648
				}
				// data.push(index + ":" + parseInt(tempStr.reverse().join(""),2).toString(10));
				data.push(index + ":" + str);
			});
			return data.join(",") + ",";
		};

		SelectTime.prototype.setData = function (str) {
			var grid = this.wrap.find("table").eq(0),
				cls = this.setting.cls,
				trs = grid.find("tr:gt(0)"),
				tds = null,
				arr = [],
				tempArr = [],
				i,
				j;

			if (str) {
				arr = str.split(",");
				for (i = 0; i < 8; i++) {
					if (arr[i]) {
						tds = trs.eq(i).find("td:gt(0)");
						tempArr = (Math.abs(arr[i].split(":")[1])).toString(2).split("").reverse();
						for (j = 0; j < 24; j++ ) {
							if (tempArr[j] === "1") {
								tds.eq(j).addClass(cls);
							} else {
								tds.eq(j).removeClass(cls);
							}
						}
					}
				}
			}
		};

		return SelectTime;
	})();

	$.fn.selectTime = function (options) {
		var selectTime;

		if (typeof options === 'object' || !options) {
			this.each(function (key, value) {
				selectTime = new SelectTime($(this), options);
				selectTime.init();
			});
		} else if (typeof options === "string") {
			selectTime = new SelectTime($(this));
			return selectTime[options].apply(selectTime, Array.prototype.slice.call(arguments, 1));
		}
	}

	$.fn.selectTime.defaults = {
		//weekStr: ["日", "一", "二", "三", "四", "五", "六"],
		weekStr: [IDC_SUNDAY, 
		          IDC_MONDAY, 
		          IDC_TUESDAY, 
		          IDC_WEDNESDAY, 
		          IDC_THURSDAY, 
		          IDC_FRIDAY, 
		          IDC_SATURDAY],
		cls: "selected",
		data: ""
	}
})(jQuery);
