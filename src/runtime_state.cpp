#include "runtime_state.h"
#include "json.h"

#include <algorithm>
#include <cctype>
#include <sstream>

static void toLower(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
}

static bool isValidLanguageCode(const std::string& s)
{
    if (s.empty())
        return false;

    if (s.size() > 16)
        return false;

    for (size_t i = 0; i < s.size(); i++)
    {
        unsigned char c = (unsigned char)s[i];

        if ((c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '_' ||
            c == '-')
        {
            continue;
        }

        return false;
    }

    return true;
}

static void writeEscaped(std::ostringstream& out, const std::string& s)
{
    for (size_t i = 0; i < s.size(); i++)
    {
        unsigned char c = (unsigned char)s[i];

        switch (c)
        {
        case '\\':
            out << "\\\\";
            break;

        case '"':
            out << "\\\"";
            break;

        case '\n':
            out << "\\n";
            break;

        case '\r':
            out << "\\r";
            break;

        case '\t':
            out << "\\t";
            break;

        default:
            out << s[i];
            break;
        }
    }
}

void RuntimeStateDefaults(RuntimeState& st)
{
    st.language.clear();
    st.hasLanguage = false;
}

bool LoadRuntimeState(
    const char* path,
    RuntimeState& st,
    std::string& err
)
{
    RuntimeStateDefaults(st);
    err.clear();

    std::string text = JsonReadFile(path);

    /*
        Missing or empty runtime_state.json is not an error.
        config.json remains the source of defaults.
    */
    if (text.empty())
        return true;

    JVal root;

    if (!JsonParseText(text, root, err))
    {
        err = "runtime_state.json: " + err;
        return false;
    }

    if (root.t != JVal::OBJ)
    {
        err = "runtime_state.json: root must be an object";
        return false;
    }

    std::string lang = JsonString(root, "language", "");

    if (lang.empty())
        return true;

    toLower(lang);

    if (!isValidLanguageCode(lang))
    {
        err = "runtime_state.json: invalid language code";
        return false;
    }

    st.language = lang;
    st.hasLanguage = true;

    return true;
}

bool SaveRuntimeState(
    const char* path,
    const RuntimeState& st,
    std::string& err
)
{
    err.clear();

    if (!path || !path[0])
    {
        err = "cannot write runtime_state.json, empty path";
        return false;
    }

    std::string lang = st.language;
    toLower(lang);

    if (!st.hasLanguage || !isValidLanguageCode(lang))
    {
        err = "cannot write runtime_state.json, invalid language code";
        return false;
    }

    std::ostringstream out;

    out << "{\r\n";
    out << "  \"language\": \"";
    writeEscaped(out, lang);
    out << "\"\r\n";
    out << "}\r\n";

    std::string data = out.str();

    std::string tmpPath = std::string(path) + ".tmp";

    std::wstring wtmp = s2ws(tmpPath);
    std::wstring wpath = s2ws(path);

    FILE* f = _wfopen(wtmp.c_str(), L"wb");

    if (!f)
    {
        err = "cannot write runtime_state.json.tmp";
        return false;
    }

    size_t written = fwrite(data.c_str(), 1, data.size(), f);

    fclose(f);

    if (written != data.size())
    {
        DeleteFileW(wtmp.c_str());
        err = "cannot write complete runtime_state.json.tmp";
        return false;
    }

    if (!MoveFileExW(
            wtmp.c_str(),
            wpath.c_str(),
            MOVEFILE_REPLACE_EXISTING))
    {
        DWORD e = GetLastError();
        DeleteFileW(wtmp.c_str());

        err = "cannot replace runtime_state.json, winerr=" + std::to_string((long long)e);
        return false;
    }

    return true;
}