#pragma once
#ifndef WINVER
#define WINVER 0x0501
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#ifndef _WIN32_IE
#define _WIN32_IE 0x0501
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdarg>
#include <stdint.h>
#include <limits.h>

static inline std::string ws2s(const std::wstring& ws)
{
    if (ws.empty())
        return std::string();

    if (ws.size() > (size_t)INT_MAX)
        return std::string();

    int inLen = (int)ws.size();

    int len = WideCharToMultiByte(
        CP_UTF8,
        0,
        ws.c_str(),
        inLen,
        NULL,
        0,
        NULL,
        NULL
    );

    if (len <= 0)
        return std::string();

    std::string out((size_t)len, 0);

    int written = WideCharToMultiByte(
        CP_UTF8,
        0,
        ws.c_str(),
        inLen,
        &out[0],
        len,
        NULL,
        NULL
    );

    if (written <= 0)
        return std::string();

    if (written < len)
        out.resize((size_t)written);

    return out;
}

static inline std::wstring s2ws(const std::string& s)
{
    if (s.empty())
        return std::wstring();

    if (s.size() > (size_t)INT_MAX)
        return std::wstring();

    int inLen = (int)s.size();

    int len = MultiByteToWideChar(
        CP_UTF8,
        0,
        s.c_str(),
        inLen,
        NULL,
        0
    );

    if (len <= 0)
        return std::wstring();

    std::wstring out((size_t)len, 0);

    int written = MultiByteToWideChar(
        CP_UTF8,
        0,
        s.c_str(),
        inLen,
        &out[0],
        len
    );

    if (written <= 0)
        return std::wstring();

    if (written < len)
        out.resize((size_t)written);

    return out;
}