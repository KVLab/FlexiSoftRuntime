#pragma once

#include "common.h"
#include <map>
#include <vector>

struct LanguageInfo
{
    std::string code;
    std::string name;

    std::string fontFace;
    std::string fontFile;

    std::map<std::string, std::string> strings;
};

struct AppLanguages
{
    std::string requestedCode;
    std::string activeCode;

    LanguageInfo fallback;
    LanguageInfo active;
    std::vector<LanguageInfo> available;

    bool loadedFromFile;
    bool fallbackLoadedFromFile;
    bool activeLoadedFromFile;
};

void SetLanguagesDefaults(AppLanguages& lang);

bool LoadLanguages(
    const char* path,
    const std::string& requestedCode,
    AppLanguages& lang,
    std::string& err
);

bool SaveLanguagesTemplate(
    const char* path,
    const AppLanguages& lang,
    std::string& err
);

std::string LangText(const AppLanguages& lang, const char* key);
std::wstring LangTextW(const AppLanguages& lang, const char* key);

std::string LangTextReplace(
    const AppLanguages& lang,
    const char* key,
    const char* token,
    const std::string& value
);