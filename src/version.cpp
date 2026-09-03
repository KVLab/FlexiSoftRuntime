#include "version.h"
#include "json.h"
#include "about.h"

bool LoadVersion(VersionInfo& v, std::string& err)
{
    v = VersionInfo();
    err.clear();

    JVal root;

    if (!JsonParseText(ABOUT_JSON, root, err))
        return false;

    v.product = JsonString(root, "product", "");
    v.version = JsonString(root, "version", "");

    std::map<std::string, JVal>::const_iterator it;

    for (it = root.o.begin(); it != root.o.end(); ++it)
    {
        const std::string prefix = "description_";

        if (it->first.compare(0, prefix.size(), prefix) != 0)
            continue;

        if (it->second.t != JVal::STR)
            continue;

        std::string code = it->first.substr(prefix.size());

        if (!code.empty())
            v.description[code] = it->second.s;
    }

    v.buildDate = JsonString(root, "build_date", "");
    v.buildType = JsonString(root, "build_type", "");
    v.buildToolset = JsonString(root, "build_toolset", "");
    v.target = JsonString(root, "target", "");

    v.author = JsonString(root, "author", "");
    v.email = JsonString(root, "email", "");
    v.homepage = JsonString(root, "homepage", "");

    v.copyrightText = JsonString(root, "copyright", "");

    return true;
}

std::string VersionDescription(const VersionInfo& v, const std::string& languageCode)
{
    if (!languageCode.empty())
    {
        std::map<std::string, std::string>::const_iterator it =
            v.description.find(languageCode);

        if (it != v.description.end() && !it->second.empty())
            return it->second;
    }

    {
        std::map<std::string, std::string>::const_iterator it =
            v.description.find("en");

        if (it != v.description.end() && !it->second.empty())
            return it->second;
    }

    if (!v.description.empty())
        return v.description.begin()->second;

    return "";
}
