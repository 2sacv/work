/*
 *       Filename:  coordinate_system.h
 *    Description:  
 *        Version:  1.0
 *        Created:  07/03/2025 08:29:52 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xuyx (), 
 *   Organization:  
 */

#ifndef _COORDINATE_SYSTEM_H_
#define _COORDINATE_SYSTEM_H_

void wgs84_to_gcj02(double wgs_lng, double wgs_lat, double *gcj_lng, double *gcj_lat);
void gcj02_to_wgs84(double gcj_lng, double gcj_lat, double *wgs_lng, double *wgs_lat);

#endif  // _COORDINATE_SYSTEM_H_
