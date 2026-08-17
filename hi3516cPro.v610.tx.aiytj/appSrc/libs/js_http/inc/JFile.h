/*
 * File Name    :
 * Created Time : 2024-03-10
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#ifndef _JFILE_H_
#define _JFILE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <sys/stat.h>
#include <unistd.h>
#endif


#include <string>

#ifndef MAX_PATH
#define MAX_PATH    256
#endif

class JFile
{
public:
    JFile()
    {
        filepath[0] = '\0';
        fp = NULL;
    }

    ~JFile()
    {
        close();
    }

    int open(const char* filepath, const char* mode)
    {
        close();
        strncpy(this->filepath, filepath, MAX_PATH - 1);
        fp = fopen(filepath, mode);
        return fp ? 0 : -1;
    }

    void close()
    {
        if (fp) {
            fclose(fp);
            fp = NULL;
        }
    }

    bool isopen()
    {
        return fp != NULL;
    }

    int remove()
    {
        close();
        return ::remove(filepath);
    }

    int rename(const char* newpath)
    {
        close();
        return ::rename(filepath, newpath);
    }

    size_t read(void* ptr, size_t len)
    {
        return fread(ptr, 1, len, fp);
    }

    size_t write(const void* ptr, size_t len)
    {
        return fwrite(ptr, 1, len, fp);
    }

    size_t write(const std::string& str)
    {
        return write(str.c_str(), str.length());
    }

    int seek(size_t offset, int whence = SEEK_SET)
    {
        return fseek(fp, offset, whence);
    }

    int tell()
    {
        return ftell(fp);
    }

    int flush()
    {
        return fflush(fp);
    }

    static size_t size(const char* filepath)
    {
        struct stat st;
        memset(&st, 0, sizeof(st));
        stat(filepath, &st);
        return st.st_size;
    }

    size_t size()
    {
        return JFile::size(filepath);
    }

    size_t readall(std::string& str)
    {
        size_t filesize = size();
        if (filesize == 0) return 0;
        str.resize(filesize);
        return fread((void*)str.data(), 1, filesize, fp);
    }

    int readrange(std::string& str, size_t from = 0, size_t to = 0)
    {
        size_t filesize = size();
        if (filesize == 0) return 0;
        if (to == 0 || to >= filesize) to = filesize - 1;
        size_t readbytes = to - from + 1;
        str.resize(readbytes);
        fseek(fp, from, SEEK_SET);
        return fread((void*)str.data(), 1, readbytes, fp);
    }

public:
    char  filepath[MAX_PATH];
    FILE* fp;
};

#endif

