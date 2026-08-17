/* 
 *       Filename:  coordinate_system.c
 *    Description:  
 *        Version:  1.0
 *        Created:  07/03/2025 08:28:55 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xuyx (), 
 *   Organization:  
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

const double PI = 3.1415926535897932384626;
const double A = 6378245.0;                 // 卫星椭球长半轴
const double EE = 0.00669342162296594323;   // 椭球偏心率

// 转换角度为弧度
double deg2rad(double deg)
{
    return deg * PI / 180.0;
}

// 转换弧度为角度
double rad2deg(double rad)
{
    return rad * 180.0 / PI;
}

// 判断坐标是否在中国境外（无需偏移）
bool out_of_china(double lng, double lat)
{
    return (lng < 72.004 || lng > 137.8347) || (lat < 0.8293 || lat > 55.8271);
}

// 纬度偏移计算
double transform_lat(double x, double y)
{
    double ret = -100.0 + 2.0 * x + 3.0 * y + 0.2 * y * y + 0.1 * x * y + 0.2 * sqrt(fabs(x));
    ret += (20.0 * sin(6.0 * x * PI) + 20.0 * sin(2.0 * x * PI)) * 2.0 / 3.0;
    ret += (20.0 * sin(y * PI) + 40.0 * sin(y / 3.0 * PI)) * 2.0 / 3.0;
    ret += (160.0 * sin(y / 12.0 * PI) + 320.0 * sin(y * PI / 30.0)) * 2.0 / 3.0;
    return ret;
}

// 经度偏移计算
double transform_lng(double x, double y)
{
    double ret = 300.0 + x + 2.0 * y + 0.1 * x * x + 0.1 * x * y + 0.1 * sqrt(fabs(x));
    ret += (20.0 * sin(6.0 * x * PI) + 20.0 * sin(2.0 * x * PI)) * 2.0 / 3.0;
    ret += (20.0 * sin(x * PI) + 40.0 * sin(x / 3.0 * PI)) * 2.0 / 3.0;
    ret += (150.0 * sin(x / 12.0 * PI) + 300.0 * sin(x / 30.0 * PI)) * 2.0 / 3.0;
    return ret;
}

// WGS-84 → GCJ-02
void wgs84_to_gcj02(double wgs_lng, double wgs_lat, double *gcj_lng, double *gcj_lat)
{
    if (out_of_china(wgs_lng, wgs_lat)) {
        *gcj_lng = wgs_lng;
        *gcj_lat = wgs_lat;
        return;
    }

    double dlat = transform_lat(wgs_lng - 105.0, wgs_lat - 35.0);
    double dlng = transform_lng(wgs_lng - 105.0, wgs_lat - 35.0);
    double radlat = wgs_lat / 180.0 * PI;
    double magic = sin(radlat);
    magic = 1 - EE * magic * magic;
    double sqrt_magic = sqrt(magic);
    dlat = (dlat * 180.0) / ((A * (1 - EE)) / (magic * sqrt_magic) * PI);
    dlng = (dlng * 180.0) / (A / sqrt_magic * cos(radlat) * PI);
    *gcj_lat = wgs_lat + dlat;
    *gcj_lng = wgs_lng + dlng;
}

// GCJ-02 → WGS-84（逆向转换，精度约±10米）
void gcj02_to_wgs84(double gcj_lng, double gcj_lat, double *wgs_lng, double *wgs_lat)
{
    if (out_of_china(gcj_lng, gcj_lat)) {
        *wgs_lng = gcj_lng;
        *wgs_lat = gcj_lat;
        return;
    }
    double dlat = transform_lat(gcj_lng - 105.0, gcj_lat - 35.0);
    double dlng = transform_lng(gcj_lng - 105.0, gcj_lat - 35.0);
    double radlat = gcj_lat / 180.0 * PI;
    double magic = sin(radlat);
    magic = 1 - EE * magic * magic;
    double sqrt_magic = sqrt(magic);
    dlat = (dlat * 180.0) / ((A * (1 - EE)) / (magic * sqrt_magic) * PI);
    dlng = (dlng * 180.0) / (A / sqrt_magic * cos(radlat) * PI);
    *wgs_lat = gcj_lat - dlat;
    *wgs_lng = gcj_lng - dlng;
}
