#pragma once
#include "common.h"

struct RuntimeState
{
    std::string language;
    bool hasLanguage;
};

void RuntimeStateDefaults(RuntimeState& st);

bool LoadRuntimeState(
    const char* path,
    RuntimeState& st,
    std::string& err
);

bool SaveRuntimeState(
    const char* path,
    const RuntimeState& st,
    std::string& err
);