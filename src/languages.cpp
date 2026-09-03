#include "languages.h"
#include "json.h"
#include <sstream>
#include <algorithm>
#include <cctype>

static void toLower(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
}

static void setString(LanguageInfo& info, const char* key, const char* value)
{
    info.strings[key] = value;
}

static void setHardcodedEnglish(LanguageInfo& info)
{
    info.code = "en";
    info.name = "English";
    info.fontFace = "Tahoma";
    info.fontFile = "";
    info.strings.clear();

    setString(info, "tray.title", "FlexiSoft Runtime");
    setString(info, "tray.reload_config", "Reload config");
    setString(info, "tray.reconnect", "Reconnect");
    setString(info, "tray.about", "About");
    setString(info, "tray.exit", "Exit");
    setString(info, "tray.language", "Language");
    setString(info, "tray.send_command", "Send {channel} command");

    setString(info, "status.ok", "OK");
    setString(info, "status.error", "ERROR");
    setString(info, "status.on", "ON");
    setString(info, "status.off", "OFF");
    setString(info, "status.disabled", "disabled");

    setString(info, "alert.input.window_title", "FlexiSoft - input error");
    setString(info, "alert.repeat.window_title", "FlexiSoft - check guard");
    setString(info, "alert.command_failed.window_title", "FlexiSoft - command failed");

    setString(info, "alert.input.title", "Active input errors");
    setString(info, "alert.repeat.title", "Check proper guard closing");
    setString(info, "alert.command_failed.title", "Command failed");

    setString(info, "alert.repeat.prefix", "Repeated simultaneity error / guard is not properly closed:");
    setString(info, "alert.active_errors", "Active errors:");
    setString(info, "alert.command_failed.text", "Command failed.");
    setString(info, "alert.affected_channels", "Affected channels:");

    setString(info, "alert.input.footer_yes", "YES = send one common command for all listed errors");
    setString(info, "alert.input.footer_no", "NO = close alert without command");
    setString(info, "alert.command_failed.footer_yes", "YES = send command again");
    setString(info, "alert.command_failed.footer_no", "NO = close alert without retry");

    setString(info, "button.yes", "YES");
    setString(info, "button.no", "NO");
    setString(info, "button.ok", "OK");

    setString(info, "about.title", "About");
    setString(info, "about.version", "Version");
    setString(info, "about.build_date", "Build date");
    setString(info, "about.open_readme", "Open README");
    setString(info, "about.open_manual", "Open MANUAL");
    setString(info, "about.open_log", "Open LOG");
    setString(info, "about.project_page", "Project page");

    setString(info, "error.missing_readme", "README.md was not found.");
    setString(info, "error.missing_manual", "MANUAL.md was not found.");
    setString(info, "error.missing_log", "Log file was not found.");
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

bool SaveLanguagesTemplate(
    const char* path,
    const AppLanguages& lang,
    std::string& err
)
{
    err.clear();

    if (!path || !path[0])
    {
        err = "cannot write languages.json, empty path";
        return false;
    }

    const LanguageInfo& en = lang.fallback;

    std::ostringstream out;

    out << "{\r\n";
    out << "  \"languages\": {\r\n";
    out << "    \"en\": {\r\n";

    out << "      \"name\": \"";
    writeEscaped(out, en.name);
    out << "\",\r\n";

    out << "      \"font_face\": \"";
    writeEscaped(out, en.fontFace);
    out << "\",\r\n";

    out << "      \"font_file\": \"";
    writeEscaped(out, en.fontFile);
    out << "\",\r\n";

    out << "      \"strings\": {\r\n";

    std::map<std::string, std::string>::const_iterator it;
    size_t index = 0;
    size_t count = en.strings.size();

    for (it = en.strings.begin(); it != en.strings.end(); ++it)
    {
        out << "        \"";
        writeEscaped(out, it->first);
        out << "\": \"";
        writeEscaped(out, it->second);
        out << "\"";

        if (index + 1 < count)
            out << ",";

        out << "\r\n";
        index++;
    }

    out << "      }\r\n";
    out << "    }\r\n";
    out << "  }\r\n";
    out << "}\r\n";

    std::string data = out.str();

    std::wstring wpath = s2ws(path);

    FILE* f = _wfopen(wpath.c_str(), L"wb");

    if (!f)
    {
        err = "cannot write languages.json";
        return false;
    }

    size_t written = fwrite(data.c_str(), 1, data.size(), f);

    fclose(f);

    if (written != data.size())
    {
        err = "cannot write complete languages.json";
        return false;
    }

    return true;
}

void SetLanguagesDefaults(AppLanguages& lang)
{
    lang.requestedCode = "en";
    lang.activeCode = "en";

    setHardcodedEnglish(lang.fallback);
    lang.active = lang.fallback;

    lang.available.clear();
    lang.available.push_back(lang.fallback);

    lang.loadedFromFile = false;
    lang.fallbackLoadedFromFile = false;
    lang.activeLoadedFromFile = false;
}

static bool parseLanguageInfo(
    const JVal& languagesObj,
    const std::string& code,
    const LanguageInfo& base,
    LanguageInfo& out
)
{
    const JVal* node = JsonGet(languagesObj, code.c_str());

    if(!node || node->t != JVal::OBJ)
        return false;

    out = base;
    out.code = code;

    out.name = JsonString(*node, "name", out.name);
    out.fontFace = JsonString(*node, "font_face", out.fontFace);
    out.fontFile = JsonString(*node, "font_file", out.fontFile);

    const JVal* strings = JsonGet(*node, "strings");

    if(strings && strings->t == JVal::OBJ)
    {
        std::map<std::string, JVal>::const_iterator it;

        for(it = strings->o.begin(); it != strings->o.end(); ++it)
        {
            if(it->second.t == JVal::STR)
                out.strings[it->first] = it->second.s;
        }
    }

    return true;
}

static bool languageExists(const std::vector<LanguageInfo>& list, const std::string& code)
{
    for (size_t i = 0; i < list.size(); i++)
    {
        if (list[i].code == code)
            return true;
    }

    return false;
}

static void addLanguageIfPresent(
    const JVal& languagesObj,
    const std::string& code,
    const LanguageInfo& fallback,
    std::vector<LanguageInfo>& out
)
{
    if (languageExists(out, code))
        return;

    LanguageInfo info;

    if (parseLanguageInfo(languagesObj, code, fallback, info))
        out.push_back(info);
}

static void loadAvailableLanguages(
    const JVal& languagesObj,
    const LanguageInfo& fallback,
    std::vector<LanguageInfo>& out
)
{
    out.clear();

    /*
        Prefer stable order in tray menu.
        Extra/custom languages are appended alphabetically from std::map.
    */
    addLanguageIfPresent(languagesObj, "en", fallback, out);
    addLanguageIfPresent(languagesObj, "cz", fallback, out);
    addLanguageIfPresent(languagesObj, "uk", fallback, out);

    std::map<std::string, JVal>::const_iterator it;

    for (it = languagesObj.o.begin(); it != languagesObj.o.end(); ++it)
    {
        addLanguageIfPresent(languagesObj, it->first, fallback, out);
    }

    if (out.empty())
        out.push_back(fallback);
}

bool LoadLanguages(
    const char* path,
    const std::string& requestedCode,
    AppLanguages& lang,
    std::string& err
)
{
    SetLanguagesDefaults(lang);
    err.clear();

    std::string req = requestedCode;

    if(req.empty())
        req = "en";

    toLower(req);

    lang.requestedCode = req;

    JVal root;

    if(!JsonParseFile(path, root, err))
    {
        if(err == "JSON file not found or empty")
        {
            std::string saveErr;

            if (SaveLanguagesTemplate(path, lang, saveErr))
            {
                err = "languages.json not found or empty, created new file with built-in English defaults";
            }

            else
            {
                err = "languages.json not found or empty, using built-in English defaults, template save failed: " + saveErr;
            }

            return true;
        }

        return false;
    }

    const JVal* languages = JsonGet(root, "languages");

    if(!languages || languages->t != JVal::OBJ)
    {
        err = "languages.json: missing object 'languages'";
        return false;
    }

    lang.loadedFromFile = true;

    LanguageInfo fileEn;

    if(parseLanguageInfo(*languages, "en", lang.fallback, fileEn))
    {
        lang.fallback = fileEn;
        lang.fallbackLoadedFromFile = true;
    }

    loadAvailableLanguages(*languages, lang.fallback, lang.available);

    if(req == "en")
    {
        lang.active = lang.fallback;
        lang.activeCode = "en";
        lang.activeLoadedFromFile = lang.fallbackLoadedFromFile;
        return true;
    }

    LanguageInfo base;
    base.code = req;
    base.name = req;
    base.fontFace = lang.fallback.fontFace;
    base.fontFile = "";
    base.strings.clear();

    LanguageInfo active;

    if(parseLanguageInfo(*languages, req, base, active))
    {
        lang.active = active;
        lang.activeCode = req;
        lang.activeLoadedFromFile = true;
        return true;
    }

    lang.active = lang.fallback;
    lang.activeCode = "en";
    lang.activeLoadedFromFile = lang.fallbackLoadedFromFile;

    err = "languages.json loaded, language '" + req + "' not found, using en";
    return true;
}

std::string LangText(const AppLanguages& lang, const char* key)
{
    if(!key || !*key)
        return "";

    {
        std::map<std::string, std::string>::const_iterator it =
            lang.active.strings.find(key);

        if(it != lang.active.strings.end())
            return it->second;
    }

    {
        std::map<std::string, std::string>::const_iterator it =
            lang.fallback.strings.find(key);

        if(it != lang.fallback.strings.end())
            return it->second;
    }

    return key;
}

std::wstring LangTextW(const AppLanguages& lang, const char* key)
{
    return s2ws(LangText(lang, key));
}

std::string LangTextReplace(
    const AppLanguages& lang,
    const char* key,
    const char* token,
    const std::string& value
)
{
    std::string text = LangText(lang, key);

    if(!token || !*token)
        return text;

    std::string t = token;

    size_t pos = 0;

    for(;;)
    {
        pos = text.find(t, pos);

        if(pos == std::string::npos)
            break;

        text.replace(pos, t.size(), value);
        pos += value.size();
    }

    return text;
}