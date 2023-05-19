#ifndef LPLOG_H
#define LPLOG_H
#include <cstdio>
#include <cstdarg>
#include <string>
#include <stdint.h>
#include <time.h>
#include <sstream>

// Verbose level
enum {
    VERBOSE_NONE,
    VERBOSE_INFO,
    VERBOSE_DEBUG
};

class LpLog
{
    int verboseLevel;

    public:
        static LpLog& getInstance()
        {
            static LpLog instance; // Guaranteed to be destroyed.
            return instance;
        }
        
    private:
        LpLog() {
            verboseLevel = VERBOSE_INFO;
        };

    public:
        LpLog(LpLog const&)   = delete;
        void operator=(LpLog const&)  = delete;

        void setVerbose(int level)
        {
            verboseLevel = level;
        }

        void d(std::string tag, const char* str, ...)
        {
            if (verboseLevel < VERBOSE_DEBUG)
                return;
            va_list a_list;
            va_start(a_list, str);
            if (!tag.empty())
                printf("[ %4s] [ %-8s]: ", "DBUG", tag.c_str());
            vprintf(str, a_list);
            va_end(a_list);
        } 

        void i(std::string tag, const char* str, ...)
        {
            if (verboseLevel < VERBOSE_INFO)
                return;
            va_list a_list;
            va_start(a_list, str);
            if (!tag.empty())
                printf("[ %4s] [ %-8s]: ", "INFO", tag.c_str());
            vprintf(str, a_list);
            va_end(a_list);
        } 

        void e(std::string tag, const char* str, ...)
        {
            va_list a_list;
            va_start(a_list, str);
            if (!tag.empty())
                printf("[ %4s] [ %-8s]: ", "ERR", tag.c_str());
            vprintf(str, a_list);
            va_end(a_list);
        } 
};


#endif