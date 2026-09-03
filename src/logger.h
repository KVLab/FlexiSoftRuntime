#pragma once
#include "common.h"

class Logger {
public:
    Logger();
    ~Logger();

    void configure(bool enabled, const std::string& file, bool newestFirst, DWORD maxBytes);
    void log(const char* fmt, ...);

private:
    void writeLineNormal(const char* line);
    void writeLineNewestFirst(const char* line);
    DWORD fileSize(const std::string& path);

    bool enabled_;
    bool newestFirst_;
    DWORD maxBytes_;
    std::string file_;
    CRITICAL_SECTION cs_;
};

extern Logger gLog;