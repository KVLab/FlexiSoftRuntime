#pragma once

#include "common.h"

bool UiFontsConfigure(
    const std::string& fontFace,
    const std::string& fontFile,
    std::string& err
);

void UiFontsShutdown();

HFONT UiFontsCreateFont(int height, int weight);

const std::string& UiFontsActiveFace();
const std::string& UiFontsActiveFile();