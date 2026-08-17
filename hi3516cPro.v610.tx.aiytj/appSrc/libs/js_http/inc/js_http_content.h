/*
 * File Name    :
 * Created Time : 2024-06-13
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */


#ifndef _js_http_content_h_
#define _js_http_content_h_

#include <string>
#include <map>

/**************multipart/form-data*************************************
--boundary
Content-Disposition: form-data; name="user"

content
--boundary
Content-Disposition: form-data; name="avatar"; filename="user.jpg"
Content-Type: image/jpeg

content
--boundary--
***********************************************************************/
struct JHttpFormData {
    std::string     filename;
    std::string     content;

    JHttpFormData(const char* content, int contentsize, const char* filename = NULL)
    {
        if (content && contentsize > 0) {
            this->content.resize(contentsize);
            memcpy((void *)this->content.data(), (void *)content, contentsize);
        }

        if (filename) {
            this->filename = filename;
        }
    }

    JHttpFormData(const char* content = NULL, const char* filename = NULL)
    {
        if (content) {
            this->content = content;
        }

        if (filename) {
            this->filename = filename;
        }
    }
};

typedef std::map<std::string, std::string>      JHttpQuerys;
typedef std::map<std::string, std::string>      JHttpHeaders;
typedef std::map<std::string, JHttpFormData>    JHttpMultiParts;

std::string dump_query_params(JHttpQuerys& querys);
int parse_query_params(const char* querystring, JHttpQuerys& querys);

std::string dump_multipart(JHttpMultiParts& multparts, const char* boundary);
int parse_multipart(const std::string& str, JHttpMultiParts& multparts, const char* boundary);

#endif
