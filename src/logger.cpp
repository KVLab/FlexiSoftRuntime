#include "logger.h"
#include "common.h"

Logger gLog;

Logger::Logger()
    : enabled_(false),
    newestFirst_(false),
    maxBytes_(65536),
    file_("flexi_runtime.log")
{
    InitializeCriticalSection(&cs_);
}

Logger::~Logger()
{
    DeleteCriticalSection(&cs_);
}

static FILE* fopenUtf8Path(const std::string& path, const wchar_t* mode)
{
    std::wstring wpath = s2ws(path);
    return _wfopen(wpath.c_str(), mode);
}

static bool deleteUtf8Path(const std::string& path)
{
    std::wstring wpath = s2ws(path);
    return DeleteFileW(wpath.c_str()) != FALSE;
}

static bool moveUtf8Path(const std::string& from, const std::string& to, bool replaceExisting)
{
    std::wstring wfrom = s2ws(from);
    std::wstring wto = s2ws(to);

    DWORD flags = replaceExisting ? MOVEFILE_REPLACE_EXISTING : 0;

    return MoveFileExW(
        wfrom.c_str(),
        wto.c_str(),
        flags
    ) != FALSE;
}

void Logger::configure(bool enabled, const std::string& file, bool newestFirst, DWORD maxBytes)
{
    EnterCriticalSection(&cs_);

    enabled_ = enabled;
    newestFirst_ = newestFirst;

    if (!file.empty())
        file_ = file;

    if (maxBytes < 4096)
        maxBytes_ = 4096;
    else
        maxBytes_ = maxBytes;

    LeaveCriticalSection(&cs_);
}

DWORD Logger::fileSize(const std::string& path)
{
    WIN32_FILE_ATTRIBUTE_DATA fad;
    std::wstring wpath = s2ws(path);

    if (!GetFileAttributesExW(wpath.c_str(), GetFileExInfoStandard, &fad))
        return 0;

    if (fad.nFileSizeHigh != 0)
        return 0xFFFFFFFF;

    return fad.nFileSizeLow;
}

void Logger::writeLineNormal(const char* line)
{
    FILE* f = fopenUtf8Path(file_, L"a");
    if (!f)
        return;

    fputs(line, f);
    fclose(f);

    if (fileSize(file_) > maxBytes_)
    {
        std::string oldName = file_ + ".old";

        deleteUtf8Path(oldName);
        moveUtf8Path(file_, oldName, true);
    }
}

void Logger::writeLineNewestFirst(const char* line)
{
    std::string oldData;

    FILE* in = fopenUtf8Path(file_, L"rb");

    if (in)
    {
        char buf[512];
        DWORD kept = (DWORD)strlen(line);

        while (kept < maxBytes_)
        {
            size_t want = sizeof(buf);

            if (kept + want > maxBytes_)
                want = maxBytes_ - kept;

            size_t n = fread(buf, 1, want, in);

            if (n == 0)
                break;

            oldData.append(buf, n);
            kept += (DWORD)n;
        }

        fclose(in);
    }

    FILE* out = fopenUtf8Path(file_, L"wb");

    if (!out)
        return;

    fputs(line, out);

    if (!oldData.empty())
        fwrite(oldData.data(), 1, oldData.size(), out);

    fclose(out);
}

void Logger::log(const char* fmt, ...)
{
    EnterCriticalSection(&cs_);

    if (!enabled_)
    {
        LeaveCriticalSection(&cs_);
        return;
    }

    char body[512];
    va_list ap;
    va_start(ap, fmt);
    _vsnprintf(body, sizeof(body) - 1, fmt, ap);
    va_end(ap);
    body[sizeof(body) - 1] = 0;

    SYSTEMTIME st;
    GetLocalTime(&st);

    char line[768];

    _snprintf(
        line,
        sizeof(line) - 1,
        "%04u-%02u-%02u %02u:%02u:%02u %s\r\n",
        st.wYear,
        st.wMonth,
        st.wDay,
        st.wHour,
        st.wMinute,
        st.wSecond,
        body
    );

    line[sizeof(line) - 1] = 0;

    if (newestFirst_)
        writeLineNewestFirst(line);
    else
        writeLineNormal(line);

    LeaveCriticalSection(&cs_);
}