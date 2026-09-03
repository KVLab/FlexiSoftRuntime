#include "path.h"

static std::wstring gExePathW;
static std::wstring gExeDirW;

static std::wstring TrimTrailingSlashW(const std::wstring& path)
{
    if(path.empty())
        return path;

    std::wstring out = path;

    while(out.size() > 3)
    {
        wchar_t c = out[out.size() - 1];

        if(c != L'\\' && c != L'/')
            break;

        out.erase(out.size() - 1);
    }

    return out;
}

static std::wstring DirNameW(const std::wstring& path)
{
    size_t p1 = path.find_last_of(L'\\');
    size_t p2 = path.find_last_of(L'/');

    size_t p;

    if(p1 == std::wstring::npos)
        p = p2;
    else if(p2 == std::wstring::npos)
        p = p1;
    else
        p = (p1 > p2) ? p1 : p2;

    if(p == std::wstring::npos)
        return L".";

    return TrimTrailingSlashW(path.substr(0, p));
}

bool InitAppPath()
{
    wchar_t buf[MAX_PATH];
    ZeroMemory(buf, sizeof(buf));

    DWORD n = GetModuleFileNameW(NULL, buf, MAX_PATH);

    if(n == 0 || n >= MAX_PATH)
        return false;

    gExePathW = buf;
    gExeDirW = DirNameW(gExePathW);

    if(gExeDirW.empty())
        return false;

    SetCurrentDirectoryW(gExeDirW.c_str());

    return true;
}

std::wstring GetExePathW()
{
    if(gExePathW.empty())
        InitAppPath();

    return gExePathW;
}

std::wstring GetExeDirW()
{
    if(gExeDirW.empty())
        InitAppPath();

    return gExeDirW;
}

bool IsAbsolutePathW(const std::wstring& path)
{
    if(path.empty())
        return false;

    /*
        C:\...
        C:/...
    */
    if(path.size() >= 3)
    {
        if(((path[0] >= L'A' && path[0] <= L'Z') ||
            (path[0] >= L'a' && path[0] <= L'z')) &&
            path[1] == L':' &&
            (path[2] == L'\\' || path[2] == L'/'))
        {
            return true;
        }
    }

    /*
        \\server\share\...
        \\?\C:\...
    */
    if(path.size() >= 2)
    {
        if((path[0] == L'\\' && path[1] == L'\\') ||
           (path[0] == L'/'  && path[1] == L'/'))
        {
            return true;
        }
    }

    /*
        /absolute/path
        Kvůli kompatibilitě, i když na Windows to běžně nepoužijeme.
    */
    if(path[0] == L'/' || path[0] == L'\\')
        return true;

    return false;
}

bool IsAbsolutePath(const std::string& path)
{
    return IsAbsolutePathW(s2ws(path));
}

std::wstring AppPathW(const wchar_t* file)
{
    std::wstring f = file ? file : L"";

    if(IsAbsolutePathW(f))
        return f;

    std::wstring dir = GetExeDirW();

    if(dir.empty())
        return f;

    if(f.empty())
        return dir;

    wchar_t last = dir[dir.size() - 1];

    if(last == L'\\' || last == L'/')
        return dir + f;

    return dir + L"\\" + f;
}

std::string AppPath(const char* file)
{
    return ws2s(AppPathW(s2ws(file ? file : "").c_str()));
}

std::wstring ResolveAppPathW(const std::wstring& path)
{
    if(path.empty())
        return path;

    if(IsAbsolutePathW(path))
        return path;

    return AppPathW(path.c_str());
}

std::string ResolveAppPath(const std::string& path)
{
    return ws2s(ResolveAppPathW(s2ws(path)));
}

bool PathFileExistsW(const std::wstring& path)
{
    if (path.empty())
        return false;

    DWORD a = GetFileAttributesW(path.c_str());

    if (a == INVALID_FILE_ATTRIBUTES)
        return false;

    return (a & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool PathFileExists(const std::string & path)
{
    return PathFileExistsW(s2ws(path));
}