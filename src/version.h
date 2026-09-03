#pragma once
#include "common.h"
#include <map>

struct VersionInfo
{
    std::string product;
    std::string version;

    std::map<std::string, std::string> description;

    std::string buildDate;
    std::string buildType;
    std::string buildToolset;
    std::string target;

    std::string author;
    std::string email;
    std::string homepage;

    std::string copyrightText;
};

bool LoadVersion(VersionInfo& v, std::string& err);

std::string VersionDescription(const VersionInfo& v, const std::string& languageCode);