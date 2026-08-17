//折线图
function initZhexianChart(title, chartBox, timeList, nameList, valueList) {
    //var myChart = echarts.init(document.getElementById(chartBox));

	var cid = document.getElementById(chartBox);
    if (cid.hasAttribute("_echarts_instance_")) {
       cid.removeAttribute("_echarts_instance_");
	}
	var myChart = echarts.init(cid);

    var dataList = [];
    for (var i = 0; i < nameList.length; i++) {
        dataList[i] = {
            type: 'line',
            // stack:'总量',//堆叠
            smooth: false,//平滑
            // areaStyle: {normal: {}},//区域颜色
            label: {
                normal: {
                    show: true,
                    position: 'top'
                }
            },
            name: nameList[i],
            data: valueList[i]
        };
    }

    var option = {
        // color: ["rgba(55,162,218)","rgba(255,159,127)"],
        title: {
            text: title
        },
        tooltip: {
            trigger: 'axis'
        },
        legend: {
            data: nameList
        },
        grid: {
            left: '3%',
            right: '4%',
            bottom: '3%',
            containLabel: true
        },
        // toolbox: {
        //     feature: {
        //         saveAsImage: {}
        //     }
        // },
        xAxis: {
            type: 'category',
            boundaryGap: false,
            data: timeList
        },
        yAxis: {
            type: 'value'
        },
        series:dataList
    };
    // 使用刚指定的配置项和数据显示图表。
    myChart.setOption(option);
}

//饼图
function initPieChart(chartBox, title, subtext, fieldName, valueList, nameList) {
    // 基于准备好的dom，初始化echarts实例
    var myChart = echarts.init(document.getElementById(chartBox));
    // 指定图表的配置项和数据
    option = {
        title: {
            text: title,
            subtext: subtext,
            x: 'center'
        },
        tooltip: {
            trigger: 'item',
            formatter: "{a} <br/>{b} : {c} ({d}%)"
        },
        legend: {
            orient: 'vertical',
            left: 'left',
            data: nameList
        },
        series: [
            {
                name: fieldName,
                type: 'pie',
                radius: '55%',
                center: ['50%', '60%'],
                data: valueList,
                itemStyle: {
                    emphasis: {
                        shadowBlur: 10,
                        shadowOffsetX: 0,
                        shadowColor: 'rgba(0, 0, 0, 0.5)'
                    }
                }
            }
        ]
    };
    // 使用刚指定的配置项和数据显示图表。
    myChart.setOption(option);
}

//柱形图
function initBarChart(title, chartBox, subtext, fieldName, valueList, nameList) {
    // 基于准备好的dom，初始化echarts实例
    var myChart = echarts.init(document.getElementById(chartBox));
    // 指定图表的配置项和数据
    var option = {
        color: ['#50BBB2', '#749F83', '#2F4554', '#D48265', '#91C7AD', '#61A0A8'],
        title: {
            text: title,
            subtext: subtext,
            x: 'center'
        },
        tooltip: {},
        legend: {
            orient: 'vertical',
            left: 'left',
            data: [fieldName]
        },
        xAxis: {
            data: nameList
        },
        yAxis: {},
        series: [{
            // barWidth: '40%',
            name: fieldName,
            type: 'bar',
            data: valueList
        }]
    };
    // alert(valueList);

    // 使用刚指定的配置项和数据显示图表。
    myChart.setOption(option);
}






  //柱形图
function initMulitBarChart(chartBox, nameList, valueList) {
    //var myChart = echarts.init(document.getElementById(chartBox));
   
	var cid = document.getElementById(chartBox);
    if (cid.hasAttribute("_echarts_instance_")) {
       cid.removeAttribute("_echarts_instance_");
	}
	var myChart = echarts.init(cid);

	var dimension_arr = [];
	dimension_arr[0] = 'product';
	var series_arr = [];
	for (var i = 0; i < nameList.length; ++i)
	{
		dimension_arr[i + 1] = nameList[i];

		series_arr[i] = { type: 'bar' };
	}

	var source_arr = [];
	for (var i = 0; i < valueList.length; ++i)
	{
		source_arr[i] = [];
		source_arr[i][0] = valueList[i]["timeinfo"];
		source_arr[i][1] = valueList[i]["in"];
		source_arr[i][2] = valueList[i]["out"];
	}

    var option = {
	  legend: {},
	  tooltip: {},
	  dataset: {
        dimensions: dimension_arr,
		source: source_arr
	  },
	  xAxis: { type: 'category' },
	  yAxis: {},
	  series: series_arr
	};

    // 使用刚指定的配置项和数据显示图表。
    myChart.setOption(option);
}