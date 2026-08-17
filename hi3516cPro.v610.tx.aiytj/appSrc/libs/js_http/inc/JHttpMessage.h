/*
 * File Name    :
 * Created Time : 2024-06-13
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */


#ifndef _JHTTPMESSAGE_H_
#define _JHTTPMESSAGE_H_

#include <map>
#include <string>

#include <string.h>

#include "js_http_content.h"
class JHttpMessage
{
public:
    int                 message_type;   //http_parser_type
    int                 http_major;
    int                 http_minor;

    JHttpHeaders        headers;
    std::string         body;

    void *              content;
    int                 content_length;
    std::string         content_type;

    // Content-Type: application/json
    // Content-Type: multipart/form-data
    // Content-Type: application/x-www-form-urlencoded
    JHttpMultiParts     multiparts;
    JHttpQuerys         keyvals;

public:
    JHttpMessage();
    virtual ~JHttpMessage();

    void Init();

    virtual std::string Dump()
    {
        return std::string();
    }

    void FillContentType();

    void FillContentLength();

    bool IsKeepAlive();

    void setMajorMinor(int major, int minor)
    {
        http_major = major;
        http_minor = minor;
    }

    void SetHeader(const char* key, const char* value)
    {
        headers[key] = value;
    }

    std::string GetHeader(const char* key)
    {
        std::map<std::string, std::string>::iterator it = headers.find(key);
        if(it == headers.end()) {
            return std::string();
        }

        return it->second;
    }

    void SetBody(const char * bodymsg,  int bodysize)
    {
        if (bodymsg && bodysize > 0) {
            body.resize(bodysize);
            memcpy((void *)body.data(), (void *)bodymsg, bodysize);
        }
    }

    void SetBody(const char * bodystr)
    {
        body = bodystr;
    }

    void DumpHeaders(std::string& str);
    void DumpBody();
    void DumpBody(std::string& str);
    int ParseBody();

    void* Content()
    {
        if (content == NULL && body.size() != 0) {
            content = (void*)body.data();
        }

        return content;
    }

    int ContentLength()
    {
        if (content_length == 0) {
            FillContentLength();
        }

        return content_length;
    }

public:
    void SetFormData(const char* name, const char *filename, char *content, int contentlen)
    {
        multiparts[name] = JHttpFormData(content, contentlen, filename);
    }

    std::string GetFormData(char* name)
    {
        if (multiparts.empty()) {
            ParseBody();
        }

        std::map<std::string, JHttpFormData>::iterator iter = multiparts.find(name);
        return iter == multiparts.end() ? std::string() : iter->second.content;
    }

    void SetURLEncoded(char *key, const char *value)
    {
        keyvals[key] = value;
    }

    std::string GetURLEncoded(char *key)
    {
        std::map<std::string, std::string>::iterator it = keyvals.find(key);
        if(it == keyvals.end()) {
            return std::string();
        }

        return it->second;
    }

};


class JHttpRequest : public JHttpMessage
{
public:
    int                 method; //http_method

    // scheme:[//[user[:password]@]host[:port]][/path][?query][#fragment]
    //http://www.mywebsite.com/sj/test/test.aspx?name=sviergn&x=true#stuff
    //Schema:        http
    //host:          www.mywebsite.com
    //path:          /sj/test/test.aspx
    //Query String:  name=sviergn&x=true
    //Anchor:        stuff

    std::string         scheme;
    std::string         host;
    int                 port;
    std::string         path;
    JHttpQuerys         querys;

public:
    JHttpRequest();
    virtual ~JHttpRequest();

    void                Init();
    virtual std::string Dump();

    void SetMethod(int emethod)
    {
        method = emethod;
    }

    void SetPath(const char *pathstr)
    {
        path = pathstr;
    }

    std::string Path()
    {
        return path;
    }

    void ParseURL(const char *url, int urllen);
    void ParseFullURL(const char *url, int urllen);

    bool IsHttps()
    {
        return strncasecmp(scheme.c_str(), "https", 5) == 0 ;
    }

    void SetParam(const char *key, const char *value)
    {
        querys[key] = value;
    }

    std::string GetParm(char *key)
    {
        std::map<std::string, std::string>::iterator it = querys.find(key);
        if(it == querys.end()) {
            return std::string();
        }

        return it->second;
    }

    void SetHost(const char* host, int port);

    // Auth
    void SetAuth(const std::string& auth);
    void SetBasicAuth(const std::string& username, const std::string& password);
    void SetBearerTokenAuth(const std::string& token);

};


class JHttpResponse : public JHttpMessage
{
public :
    int     status_code;  //http_status

public:
    JHttpResponse();
    virtual ~JHttpResponse();

    void Init();
    virtual std::string Dump();

    void setStatusCode(int statuscode)
    {
        this->status_code = statuscode;
    }
};

#endif

