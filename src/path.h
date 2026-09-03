#pragma once

#include "common.h"

bool InitAppPath();

std::wstring GetExePathW();
std::wstring GetExeDirW();

std::wstring AppPathW(const wchar_t* file);
std::string AppPath(const char* file);

bool IsAbsolutePathW(const std::wstring& path);
bool IsAbsolutePath(const std::string& path);

std::wstring ResolveAppPathW(const std::wstring& path);
std::string ResolveAppPath(const std::string& path);

bool PathFileExistsW(const std::wstring & path);
bool PathFileExists(const std::string & path);