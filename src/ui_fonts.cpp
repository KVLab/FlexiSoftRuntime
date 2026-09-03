#include "ui_fonts.h"
#include "path.h"

static std::string gUiFontFaceA = "Tahoma";
static std::string gUiFontFileA;

static std::wstring gUiFontFaceW = L"Tahoma";
static std::wstring gLoadedPrivateFontPathW;
static bool gPrivateFontLoaded = false;

static void UiFontsUnloadPrivateFont()
{
    if(gPrivateFontLoaded && !gLoadedPrivateFontPathW.empty())
    {
        RemoveFontResourceExW(
            gLoadedPrivateFontPathW.c_str(),
            FR_PRIVATE,
            NULL
        );
    }

    gPrivateFontLoaded = false;
    gLoadedPrivateFontPathW.clear();
}

bool UiFontsConfigure(
    const std::string& fontFace,
    const std::string& fontFile,
    std::string& err
)
{
    err.clear();

    /*
        Caller must destroy existing HFONT objects before calling this.
        This function unloads previous private font file and loads the new one.
    */
    UiFontsUnloadPrivateFont();

    gUiFontFaceA = fontFace.empty() ? "Tahoma" : fontFace;
    gUiFontFileA = fontFile;
    gUiFontFaceW = s2ws(gUiFontFaceA);

    if(!fontFile.empty())
    {
        std::wstring fontPath = ResolveAppPathW(s2ws(fontFile));

        int added = AddFontResourceExW(
            fontPath.c_str(),
            FR_PRIVATE,
            NULL
        );

        if (added <= 0)
        {
            err = "AddFontResourceExW failed for font_file=" + fontFile + ", using Tahoma fallback";

            gUiFontFaceA = "Tahoma";
            gUiFontFaceW = L"Tahoma";
            gUiFontFileA.clear();

            return false;
        }

        gPrivateFontLoaded = true;
        gLoadedPrivateFontPathW = fontPath;
    }

    return true;
}

void UiFontsShutdown()
{
    UiFontsUnloadPrivateFont();

    gUiFontFaceA = "Tahoma";
    gUiFontFileA.clear();
    gUiFontFaceW = L"Tahoma";
}

HFONT UiFontsCreateFont(int height, int weight)
{
    return CreateFontW(
        height,
        0,
        0,
        0,
        weight,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        gUiFontFaceW.c_str()
    );
}

const std::string& UiFontsActiveFace()
{
    return gUiFontFaceA;
}

const std::string& UiFontsActiveFile()
{
    return gUiFontFileA;
}