/*
 * File Name    :
 * Created Time : 2024-03-09
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */


#ifndef _URL_ESCAPE_H_
#define _URL_ESCAPE_H_

#include <string>

std::string escape(const std::string& str, const char* unescaped_chars);
std::string unescape(const std::string& str);
std::string escape_url(const std::string& url);

#endif 

