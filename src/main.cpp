#include "common.h"
#include "path.h"
#include "config.h"
#include "runtime.h"
#include "logger.h"
#include "version.h"
#include "languages.h"
#include "ui_fonts.h"
#include "runtime_state.h"

#define WM_TRAY (WM_APP+1)
#define ID_TRAY_EXIT 100
#define ID_TRAY_RELOAD 101
#define ID_TRAY_RECONNECT 102
#define ID_TRAY_ABOUT 103
#define ID_SEND_BASE 200
#define ID_LANG_BASE 500

enum TrayColorState
{
    TRAY_STATE_NONE = -1,
    TRAY_STATE_GRAY = 0,
    TRAY_STATE_YELLOW = 1,
    TRAY_STATE_GREEN = 2,
    TRAY_STATE_RED = 3
};

static HINSTANCE gInst;
static HWND gWnd;
static AppConfig gCfg;
static NOTIFYICONDATAW gNid;

static HICON gIconGray = NULL;
static HICON gIconYellow = NULL;
static HICON gIconGreen = NULL;
static HICON gIconRed = NULL;
static int gTrayState = TRAY_STATE_NONE;

#define ID_ALERT_YES 300
#define ID_ALERT_NO 301
#define ALERT_TYPE_NONE 0
#define ALERT_TYPE_INPUT 1
#define ALERT_TYPE_COMMAND_FAILED 2

static HWND gAlertWnd = NULL;
static HWND gAlertTitle = NULL;
static HWND gAlertText = NULL;
static HWND gAlertFooter = NULL;
static HWND gAlertLine = NULL;
static HWND gAlertIcon = NULL;
static HWND gAlertYes = NULL;
static HWND gAlertNo = NULL;

static HFONT gAlertFont = NULL;
static HFONT gAlertFontBold = NULL;
static DWORD gAlertMask = 0;
static DWORD gAlertRepeatMask = 0;
static int gAlertType = ALERT_TYPE_NONE;
static COLORREF gAlertIconColor = RGB(255, 220, 0);
static int gAlertIconKind = 0; // 0=none, 1=warning, 2=error

#define ID_ABOUT_README 400
#define ID_ABOUT_MANUAL 401
#define ID_ABOUT_LOG    402
#define ID_ABOUT_HOMEPAGE 403
#define ID_ABOUT_OK     404

static HWND gAboutWnd = NULL;

static HWND gAboutInfo = NULL;
static HWND gAboutReadme = NULL;
static HWND gAboutManual = NULL;
static HWND gAboutLog = NULL;
static HWND gAboutHomepage = NULL;
static HWND gAboutOk = NULL;

static VersionInfo gVersion;

static AppLanguages gLang;

static RuntimeState gRuntimeState;

static void CreateAlertFonts();
static void DestroyAlertFonts();
static void SetControlFont(HWND h, HFONT font);
static void RebuildUiFontsForActiveLanguage();
static void ApplyLoadedLanguageState();
static void ApplyLanguageToOpenWindows();
static void RefreshAboutWindowText();
static void RefreshAlertWindowText();
static void SwitchLanguage(const std::string& code);
static void LayoutAboutWindow(HWND h);
static void LoadAppRuntimeState();
static void SaveAppRuntimeState();
static void EnsureRuntimeDirectories();

static bool EnsureAppSubdirW(const wchar_t* relDir)
{
    if (!relDir || !relDir[0])
        return false;

    std::wstring dir = AppPathW(relDir);

    DWORD attr = GetFileAttributesW(dir.c_str());

    if (attr != INVALID_FILE_ATTRIBUTES)
    {
        if (attr & FILE_ATTRIBUTE_DIRECTORY)
            return true;

        std::wstring msg;
        msg += L"Path exists but is not a directory:\r\n";
        msg += dir;

        MessageBoxW(
            NULL,
            msg.c_str(),
            L"FlexiSoft Runtime",
            MB_ICONERROR | MB_OK
        );

        return false;
    }

    if (CreateDirectoryW(dir.c_str(), NULL))
        return true;

    DWORD err = GetLastError();

    if (err == ERROR_ALREADY_EXISTS)
    {
        attr = GetFileAttributesW(dir.c_str());

        if (attr != INVALID_FILE_ATTRIBUTES &&
            (attr & FILE_ATTRIBUTE_DIRECTORY))
        {
            return true;
        }
    }

    wchar_t errText[64];

    _snwprintf(
        errText,
        sizeof(errText) / sizeof(errText[0]) - 1,
        L"%lu",
        (unsigned long)err
    );

    errText[sizeof(errText) / sizeof(errText[0]) - 1] = 0;

    std::wstring msg;
    msg += L"Cannot create required directory:\r\n";
    msg += dir;
    msg += L"\r\n\r\nWindows error: ";
    msg += errText;

    MessageBoxW(
        NULL,
        msg.c_str(),
        L"FlexiSoft Runtime",
        MB_ICONERROR | MB_OK
    );

    return false;
}

static void EnsureRuntimeDirectories()
{
    /*
        Required for:
            conf/config.json
            conf/languages.json
            conf/runtime_state.json

        The directory must exist before config/languages default generation
        or runtime_state saving can work.
    */

    EnsureAppSubdirW(L"conf");
}

static void LoadAppConfig()
{
    std::string err;
    std::string cfgPath = AppPath("conf\\config.json");

    bool cfgOk = LoadConfig(cfgPath.c_str(), gCfg, err);
    bool cfgDefaulted = cfgOk && !err.empty();

    if (gCfg.logFile.empty())
        gCfg.logFile = "flexi_runtime.log";

    gCfg.logFile = ResolveAppPath(gCfg.logFile);

    bool logExisted = PathFileExists(gCfg.logFile);

    gLog.configure(
        gCfg.loggingEnabled,
        gCfg.logFile,
        gCfg.loggingNewestFirst,
        gCfg.loggingMaxBytes
    );

    if (gCfg.loggingEnabled)
    {
        gLog.log(
            "log %s: path=%s newest_first=%s max_bytes=%lu",
            logExisted ? "opened" : "created",
            gCfg.logFile.c_str(),
            gCfg.loggingNewestFirst ? "true" : "false",
            (unsigned long)gCfg.loggingMaxBytes
        );

        if (cfgOk && !cfgDefaulted)
        {
            gLog.log(
                "config loaded: path=%s language=%s transport=%s network_mode=%s",
                cfgPath.c_str(),
                gCfg.language.c_str(),
                ConfigTransportName(gCfg.transport),
                ConfigNetModeName(gCfg.network.mode)
             );
        }

        else if (cfgDefaulted)
        {
            gLog.log(
                "config defaulted: path=%s warning=%s",
                cfgPath.c_str(),
                err.c_str()
            );
        }

        else
        {
            gLog.log(
                "config failed: path=%s error=%s using defaults",
                cfgPath.c_str(),
                err.c_str()
            );
        }
    }

    if (!cfgOk)
    {
        MessageBoxW(
            NULL,
            s2ws(err).c_str(),
            L"config.json error",
            MB_ICONERROR | MB_OK
        );
    }
}

static void LoadAppRuntimeState()
{
    std::string err;
    std::string statePath = AppPath("conf\\runtime_state.json");

    RuntimeState st;

    if (!LoadRuntimeState(statePath.c_str(), st, err))
    {
        RuntimeStateDefaults(gRuntimeState);

        gLog.log(
            "runtime_state warning: path=%s warning=%s",
            statePath.c_str(),
            err.c_str()
        );

        return;
    }

    gRuntimeState = st;

    if (gRuntimeState.hasLanguage)
    {
        gCfg.language = gRuntimeState.language;

        gLog.log(
            "runtime_state loaded: path=%s language=%s",
            statePath.c_str(),
            gRuntimeState.language.c_str()
        );
    }
}

static void SaveAppRuntimeState()
{
    std::string err;
    std::string statePath = AppPath("conf\\runtime_state.json");

    if (!SaveRuntimeState(statePath.c_str(), gRuntimeState, err))
    {
        gLog.log(
            "runtime_state warning: path=%s warning=%s",
            statePath.c_str(),
            err.c_str()
        );

        return;
    }

    gLog.log(
        "runtime_state saved: path=%s language=%s",
        statePath.c_str(),
        gRuntimeState.language.c_str()
    );
}

static void LoadAppVersion()
{
    std::string err;

    if (LoadVersion(gVersion, err))
    {
        gLog.log(
            "version loaded: source=embedded_json product=%s version=%s build_date=%s",
            gVersion.product.c_str(),
            gVersion.version.c_str(),
            gVersion.buildDate.c_str()
        );

        return;
    }

    gLog.log(
        "version failed: source=embedded_json error=%s",
        err.c_str()
    );

    MessageBoxW(
        NULL,
        s2ws(err).c_str(),
        L"Embedded version data error",
        MB_ICONERROR | MB_OK
    );
}

static void LoadAppLanguages()
{
    std::string err;
    std::string langPath = AppPath("conf\\languages.json");

    bool langOk = LoadLanguages(langPath.c_str(), gCfg.language, gLang, err);
    bool langDefaulted = langOk && !err.empty();

    if (langOk && !langDefaulted)
    {
        gLog.log(
            "languages loaded: path=%s requested=%s active=%s name=%s font_face=%s font_file=%s",
            langPath.c_str(),
            gLang.requestedCode.c_str(),
            gLang.activeCode.c_str(),
            gLang.active.name.c_str(),
            gLang.active.fontFace.c_str(),
            gLang.active.fontFile.c_str()
        );

        ApplyLoadedLanguageState();

        return;
    }

    if (langDefaulted)
    {
        gLog.log(
            "languages defaulted: path=%s warning=%s requested=%s active=%s name=%s font_face=%s font_file=%s",
            langPath.c_str(),
            err.c_str(),
            gLang.requestedCode.c_str(),
            gLang.activeCode.c_str(),
            gLang.active.name.c_str(),
            gLang.active.fontFace.c_str(),
            gLang.active.fontFile.c_str()
        );

        ApplyLoadedLanguageState();

        return;
    }

    gLog.log(
        "languages failed: path=%s error=%s using built-in English defaults",
        langPath.c_str(),
        err.c_str()
    );

    SetLanguagesDefaults(gLang);

    ApplyLoadedLanguageState();

    MessageBoxW(
        NULL,
        s2ws(err).c_str(),
        L"languages.json error",
        MB_ICONWARNING | MB_OK
    );
}

static void CreateAlertFonts()
{
    if (!gAlertFont)
    {
        gAlertFont = UiFontsCreateFont(
            -14,
            FW_NORMAL
        );
    }

    if (!gAlertFontBold)
    {
        gAlertFontBold = UiFontsCreateFont(
            -15,
            FW_BOLD
        );
    }
}

static void DestroyAlertFonts()
{
    if(gAlertFont)
    {
        DeleteObject(gAlertFont);
        gAlertFont = NULL;
    }

    if(gAlertFontBold)
    {
        DeleteObject(gAlertFontBold);
        gAlertFontBold = NULL;
    }
}

static void RebuildUiFontsForActiveLanguage()
{
    DestroyAlertFonts();

    std::string err;

    bool ok = UiFontsConfigure(
        gLang.active.fontFace,
        gLang.active.fontFile,
        err
    );

    if (ok)
    {
        gLog.log(
            "ui font configured: active=%s face=%s file=%s",
            gLang.activeCode.c_str(),
            UiFontsActiveFace().c_str(),
            UiFontsActiveFile().c_str()
        );
    }
    else
    {
        gLog.log(
            "ui font warning: active=%s face=%s file=%s warning=%s",
            gLang.activeCode.c_str(),
            UiFontsActiveFace().c_str(),
            UiFontsActiveFile().c_str(),
            err.c_str()
        );
    }

    CreateAlertFonts();
}

static void ApplyLoadedLanguageState()
{
    ApplyConfigLanguage(gCfg, gLang.activeCode);
    RebuildUiFontsForActiveLanguage();
}

static void SetControlFont(HWND h, HFONT font)
{
    if(h && font)
        SendMessage(h, WM_SETFONT, (WPARAM)font, TRUE);
}

static void ApplyAboutFonts()
{
    SetControlFont(gAboutInfo, gAlertFont);
    SetControlFont(gAboutReadme, gAlertFont);
    SetControlFont(gAboutManual, gAlertFont);
    SetControlFont(gAboutLog, gAlertFont);
    SetControlFont(gAboutHomepage, gAlertFont);
    SetControlFont(gAboutOk, gAlertFont);
}

static void ApplyAlertFonts()
{
    SetControlFont(gAlertTitle, gAlertFontBold);
    SetControlFont(gAlertText, gAlertFont);
    SetControlFont(gAlertFooter, gAlertFont);
    SetControlFont(gAlertYes, gAlertFont);
    SetControlFont(gAlertNo, gAlertFont);
}

static HICON makeIcon(COLORREF c)
{
    HDC hdc = GetDC(NULL);
    HDC mem = CreateCompatibleDC(hdc);

    HBITMAP bmpColor = CreateCompatibleBitmap(hdc, 16, 16);
    HBITMAP bmpMask = CreateBitmap(16, 16, 1, 1, NULL);

    HBITMAP oldBmp = (HBITMAP)SelectObject(mem, bmpColor);

    RECT rc = { 0, 0, 16, 16 };

    HBRUSH bg = CreateSolidBrush(c);
    FillRect(mem, &rc, bg);
    DeleteObject(bg);

    SetBkMode(mem, TRANSPARENT);

    HFONT font = CreateFontW(
        -11,
        0, 0, 0,
        FW_HEAVY,
        FALSE, FALSE, FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        NONANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        L"Tahoma"
    );

    HFONT oldFont = (HFONT)SelectObject(mem, font);

    RECT textRc = { 1, 1, 17, 15 };

    SetTextColor(mem, RGB(0, 0, 0));

    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dy = -1; dy <= 1; dy++)
        {
            if (dx == 0 && dy == 0)
                continue;

            RECT r = textRc;
            OffsetRect(&r, dx, dy);

            DrawTextW(
                mem,
                L"FS",
                -1,
                &r,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE
            );
        }
    }

    SetTextColor(mem, RGB(255, 255, 255));

    DrawTextW(
        mem,
        L"FS",
        -1,
        &textRc,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );

    SelectObject(mem, oldFont);
    DeleteObject(font);

    SelectObject(mem, oldBmp);
    DeleteDC(mem);
    ReleaseDC(NULL, hdc);

    ICONINFO ii;
    ZeroMemory(&ii, sizeof(ii));
    ii.fIcon = TRUE;
    ii.hbmColor = bmpColor;
    ii.hbmMask = bmpMask;

    HICON ico = CreateIconIndirect(&ii);

    DeleteObject(bmpColor);
    DeleteObject(bmpMask);

    return ico;
}

static void CreateTrayIcons()
{
    if (!gIconGray)
        gIconGray = makeIcon(RGB(120, 120, 120));

    if (!gIconYellow)
        gIconYellow = makeIcon(RGB(255, 190, 0));

    if (!gIconGreen)
        gIconGreen = makeIcon(RGB(0, 210, 0));

    if (!gIconRed)
        gIconRed = makeIcon(RGB(255, 40, 40));
}

static void DestroyTrayIcons()
{
    if (gIconGray)
    {
        DestroyIcon(gIconGray);
        gIconGray = NULL;
    }

    if (gIconYellow)
    {
        DestroyIcon(gIconYellow);
        gIconYellow = NULL;
    }

    if (gIconGreen)
    {
        DestroyIcon(gIconGreen);
        gIconGreen = NULL;
    }

    if (gIconRed)
    {
        DestroyIcon(gIconRed);
        gIconRed = NULL;
    }

    gTrayState = TRAY_STATE_NONE;
}

static HICON GetTrayIconForState(int state)
{
    switch (state)
    {
    case TRAY_STATE_GRAY:
        return gIconGray;

    case TRAY_STATE_YELLOW:
        return gIconYellow;

    case TRAY_STATE_GREEN:
        return gIconGreen;

    case TRAY_STATE_RED:
        return gIconRed;

    default:
        return gIconGray;
    }
}

static void UpdateTray()
{
    RuntimeStatus st = gRuntime.status();

    bool anyErr = false;

    for (int i = 0; i < 4; i++)
    {
        if (st.ch[i].enabled && !st.ch[i].ok)
        {
            anyErr = true;
            break;
        }
    }

    int newState;

    if (!st.comOk)
        newState = TRAY_STATE_GRAY;
    else if (!st.cpuOk)
        newState = TRAY_STATE_YELLOW;
    else if (anyErr)
        newState = TRAY_STATE_RED;
    else
        newState = TRAY_STATE_GREEN;

    std::wstring tip = s2ws(gCfg.trayTooltip);
    lstrcpynW(gNid.szTip, tip.c_str(), sizeof(gNid.szTip) / sizeof(WCHAR));

    if (newState != gTrayState)
    {
        gNid.hIcon = GetTrayIconForState(newState);
        gTrayState = newState;
        Shell_NotifyIconW(NIM_MODIFY, &gNid);
    }
    else
    {
        Shell_NotifyIconW(NIM_MODIFY, &gNid);
    }
}

static std::wstring BuildInputAlertTitle(DWORD repeatMask)
{
    if (repeatMask)
        return LangTextW(gLang, "alert.repeat.title");

    return LangTextW(gLang, "alert.input.title");
}

static std::wstring BuildCommandFailedTitle()
{
    return LangTextW(gLang, "alert.command_failed.title");
}

static std::wstring BuildInputAlertText(DWORD mask, DWORD repeatMask)
{
    std::wstring text;

    if (repeatMask)
    {
        text += LangTextW(gLang, "alert.repeat.prefix");
        text += L"\r\n\r\n";

        for (int i = 0; i < 4; i++)
        {
            if (repeatMask & (1u << i))
            {
                text += L" - ";
                text += s2ws(gCfg.inputs[i].repeatFault.text);
                text += L"\r\n";
            }
        }

        text += L"\r\n";
        text += LangTextW(gLang, "alert.active_errors");
        text += L"\r\n";
    }

    for (int i = 0; i < 4; i++)
    {
        if (mask & (1u << i))
        {
            text += L" - ";
            text += s2ws(gCfg.inputs[i].alertText);
            text += L"\r\n";
        }
    }

    return text;
}

static std::wstring BuildInputAlertFooter()
{
    std::wstring text;

    text += LangTextW(gLang, "alert.input.footer_yes");
    text += L"\r\n";
    text += LangTextW(gLang, "alert.input.footer_no");

    return text;
}

static std::wstring BuildCommandFailedText(DWORD mask)
{
    std::wstring text;

    text += LangTextW(gLang, "alert.command_failed.text");
    text += L"\r\n\r\n";

    text += LangTextW(gLang, "alert.affected_channels");
    text += L"\r\n";

    for (int i = 0; i < 4; i++)
    {
        if (mask & (1u << i))
        {
            text += L" - ";
            text += s2ws(gCfg.inputs[i].name);
            text += L"\r\n";
        }
    }

    return text;
}

static std::wstring BuildCommandFailedFooter()
{
    std::wstring text;

    text += LangTextW(gLang, "alert.command_failed.footer_yes");
    text += L"\r\n";
    text += LangTextW(gLang, "alert.command_failed.footer_no");

    return text;
}

static void CloseAlertWindow()
{
    if(gAlertWnd)
        DestroyWindow(gAlertWnd);
}

static void DrawAlertWarningIcon(HDC dc, int x, int y, int s, COLORREF fillColor)
{
    int cx = x + s / 2;

    POINT tri[3];
    tri[0].x = cx;          tri[0].y = y + 4;
    tri[1].x = x + s - 5;   tri[1].y = y + s - 6;
    tri[2].x = x + 5;       tri[2].y = y + s - 6;

    HBRUSH fill = CreateSolidBrush(fillColor);
    HPEN border = CreatePen(PS_SOLID, 4, RGB(70, 70, 70));

    HGDIOBJ oldBrush = SelectObject(dc, fill);
    HGDIOBJ oldPen = SelectObject(dc, border);

    Polygon(dc, tri, 3);

    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);

    DeleteObject(border);
    DeleteObject(fill);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(0, 0, 0));

    HFONT font = CreateFontW(
        -(s * 62 / 100),
        0, 0, 0,
        FW_HEAVY,
        FALSE, FALSE, FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        L"Tahoma"
    );

    HFONT oldFont = (HFONT)SelectObject(dc, font);

    RECT r;
    r.left = x;
    r.top = y + s / 4;
    r.right = x + s;
    r.bottom = y + s - 4;

    DrawTextW(
        dc,
        L"!",
        -1,
        &r,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );

    SelectObject(dc, oldFont);
    DeleteObject(font);
}

static void DrawAlertErrorIcon(HDC dc, int x, int y, int s, COLORREF fillColor)
{
    HBRUSH fill = CreateSolidBrush(fillColor);
    HPEN border = CreatePen(PS_SOLID, 4, RGB(80, 0, 0));

    HGDIOBJ oldBrush = SelectObject(dc, fill);
    HGDIOBJ oldPen = SelectObject(dc, border);

    Ellipse(dc, x + 4, y + 4, x + s - 4, y + s - 4);

    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);

    DeleteObject(border);
    DeleteObject(fill);

    HPEN crossBorder = CreatePen(PS_SOLID, s / 5, RGB(80, 0, 0));
    oldPen = SelectObject(dc, crossBorder);

    MoveToEx(dc, x + s / 3, y + s / 3, NULL);
    LineTo(dc, x + 2 * s / 3, y + 2 * s / 3);

    MoveToEx(dc, x + 2 * s / 3, y + s / 3, NULL);
    LineTo(dc, x + s / 3, y + 2 * s / 3);

    SelectObject(dc, oldPen);
    DeleteObject(crossBorder);

    HPEN cross = CreatePen(PS_SOLID, s / 8, RGB(255, 255, 255));
    oldPen = SelectObject(dc, cross);

    MoveToEx(dc, x + s / 3, y + s / 3, NULL);
    LineTo(dc, x + 2 * s / 3, y + 2 * s / 3);

    MoveToEx(dc, x + 2 * s / 3, y + s / 3, NULL);
    LineTo(dc, x + s / 3, y + 2 * s / 3);

    SelectObject(dc, oldPen);
    DeleteObject(cross);
}

static void LayoutAlertWindow(HWND h)
{
    RECT rc;
    GetClientRect(h, &rc);

    int w = rc.right - rc.left;
    int hgt = rc.bottom - rc.top;

    int margin = 24;
    int titleH = 30;
    int iconSize = 96;
    int gap = 12;
    int btnW = 112;
    int btnH = 34;
    int btnGap = 14;
    int footerH = 50;
    int lineH = 2;

    int contentTop = margin + titleH + gap;
    int buttonsY = hgt - btnH - margin;
    int footerY = buttonsY - footerH - 10;
    int lineY = footerY - 4;

    if(gAlertTitle)
        MoveWindow(gAlertTitle, margin, margin, w - 2 * margin - iconSize - 16, titleH, TRUE);

    if(gAlertText)
        MoveWindow(
            gAlertText,
            margin,
            contentTop,
            w - 2 * margin - iconSize - 36,
            lineY - contentTop - 10,
            TRUE
        );

    if(gAlertLine)
        MoveWindow(
            gAlertLine,
            margin,
            lineY,
            w - 2 * margin,
            lineH,
            TRUE
        );

    if(gAlertFooter)
        MoveWindow(
            gAlertFooter,
            margin,
            footerY,
            w - 2 * margin,
            footerH,
            TRUE
        );

    if(gAlertYes)
        MoveWindow(
            gAlertYes,
            w - 2 * btnW - btnGap - margin,
            buttonsY,
            btnW,
            btnH,
            TRUE
        );

    if(gAlertNo)
        MoveWindow(
            gAlertNo,
            w - btnW - margin,
            buttonsY,
            btnW,
            btnH,
            TRUE
        );
}

static bool ShellOpenFileW(const std::wstring& file)
{
    HINSTANCE r = ShellExecuteW(
        NULL,
        L"open",
        file.c_str(),
        NULL,
        NULL,
        SW_SHOWNORMAL
    );

    return ((INT_PTR)r > 32);
}

static void OpenLocalFileW(const std::wstring& file)
{
    if (ShellOpenFileW(file))
        return;

    std::wstring msg;
    msg += L"Cannot open file:\r\n";
    msg += file;

    MessageBoxW(
        NULL,
        msg.c_str(),
        L"FlexiSoft Runtime",
        MB_ICONWARNING | MB_OK
    );
}

static bool LocalFileExistsW(const std::wstring& file)
{
    DWORD attr = GetFileAttributesW(file.c_str());

    if (attr == INVALID_FILE_ATTRIBUTES)
        return false;

    if (attr & FILE_ATTRIBUTE_DIRECTORY)
        return false;

    return true;
}

static std::wstring QuoteProcessArgW(const std::wstring& arg)
{
    std::wstring out;
    out += L"\"";

    size_t backslashes = 0;

    for (size_t i = 0; i < arg.size(); i++)
    {
        wchar_t c = arg[i];

        if (c == L'\\')
        {
            backslashes++;
        }
        else if (c == L'"')
        {
            out.append(backslashes * 2 + 1, L'\\');
            out += L'"';
            backslashes = 0;
        }
        else
        {
            out.append(backslashes, L'\\');
            backslashes = 0;
            out += c;
        }
    }

    out.append(backslashes * 2, L'\\');
    out += L"\"";

    return out;
}

static bool RunProgramWithArgW(
    const std::wstring& exe,
    const std::wstring& arg1,
    const std::wstring& arg2 = L""
)
{
    if (exe.empty())
        return false;

    if (!LocalFileExistsW(exe))
        return false;

    std::wstring cmdLine;
    cmdLine += QuoteProcessArgW(exe);
    cmdLine += L" ";
    cmdLine += QuoteProcessArgW(arg1);

    if (!arg2.empty())
    {
        cmdLine += L" ";
        cmdLine += QuoteProcessArgW(arg2);
    }

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    si.cb = sizeof(si);

    BOOL ok = CreateProcessW(
        exe.c_str(),
        &cmdLine[0],
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    );

    if (!ok)
        return false;

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return true;
}

static std::wstring GetWindowsNotepadPathW()
{
    wchar_t dir[MAX_PATH];

    UINT n = GetWindowsDirectoryW(
        dir,
        sizeof(dir) / sizeof(dir[0])
    );

    if (n == 0 || n >= sizeof(dir) / sizeof(dir[0]))
        return L"notepad.exe";

    std::wstring path = dir;

    if (!path.empty())
    {
        wchar_t last = path[path.size() - 1];

        if (last != L'\\' && last != L'/')
            path += L"\\";
    }

    path += L"notepad.exe";

    return path;
}

static bool IsAbsolutePathW(const std::wstring& path)
{
    if (path.size() >= 3)
    {
        wchar_t c = path[0];

        bool drive =
            ((c >= L'A' && c <= L'Z') ||
                (c >= L'a' && c <= L'z')) &&
            path[1] == L':' &&
            (path[2] == L'\\' || path[2] == L'/');

        if (drive)
            return true;
    }

    if (path.size() >= 2 &&
        path[0] == L'\\' &&
        path[1] == L'\\')
    {
        return true;
    }

    return false;
}

static std::wstring BuildMarkdownReaderFontArgW()
{
    /*
        Reader must receive the font that the runtime actually uses
        after UiFontsConfigure() fallback logic.

        Do not use gLang.active.fontFace/fontFile directly here.
        Those are requested JSON values, not necessarily the effective font.

        Reader receives:
            1) full path to the effective private font file, if available,
            2) otherwise the effective system font face,
            3) otherwise hard fallback "Tahoma".
    */

    std::string activeFileUtf8 = UiFontsActiveFile();

    if (!activeFileUtf8.empty())
    {
        std::wstring fontFile = s2ws(activeFileUtf8);

        for (size_t i = 0; i < fontFile.size(); i++)
        {
            if (fontFile[i] == L'/')
                fontFile[i] = L'\\';
        }

        std::wstring fullFontFile = IsAbsolutePathW(fontFile)
            ? fontFile
            : AppPathW(fontFile.c_str());

        if (LocalFileExistsW(fullFontFile))
            return fullFontFile;
    }

    std::string activeFaceUtf8 = UiFontsActiveFace();

    if (!activeFaceUtf8.empty())
        return s2ws(activeFaceUtf8);

    return L"Tahoma";
}

static bool OpenMarkdownDocumentW(const std::wstring& file)
{
    /*
        Preferred:
            FlexiSoftMdReader.exe "<doc.md>" "<font-file-or-font-face>"

        Fallback:
            notepad.exe "<doc.md>"

        Last fallback:
            ShellExecute on the .md file itself.
    */

    std::wstring reader = AppPathW(L"FlexiSoftMdReader.exe");
    std::wstring fontArg = BuildMarkdownReaderFontArgW();

    if (RunProgramWithArgW(reader, file, fontArg))
        return true;

    std::wstring notepad = GetWindowsNotepadPathW();

    if (RunProgramWithArgW(notepad, file))
        return true;

    if (ShellOpenFileW(file))
        return true;

    return false;
}

static std::wstring FindLocalizedDocW(const wchar_t* baseName)
{
    if (!baseName || !baseName[0])
        return L"";

    /*
        docs/README_<active>.md -> docs/README.md
        docs/MANUAL_<active>.md -> docs/MANUAL.md

        Example:
            active=cz -> docs/README_cz.md -> docs/README.md
            active=uk -> docs/README_uk.md -> docs/README.md

        No "ua" alias. Ukrainian is "uk".
    */

    std::wstring relBase = L"docs\\";
    relBase += baseName;

    if (!gLang.activeCode.empty())
    {
        std::wstring relLocalized = relBase;
        relLocalized += L"_";
        relLocalized += s2ws(gLang.activeCode);
        relLocalized += L".md";

        std::wstring fullLocalized = AppPathW(relLocalized.c_str());

        if (LocalFileExistsW(fullLocalized))
            return fullLocalized;
    }

    std::wstring relFallback = relBase;
    relFallback += L".md";

    std::wstring fullFallback = AppPathW(relFallback.c_str());

    if (LocalFileExistsW(fullFallback))
        return fullFallback;

    return L"";
}

static void OpenLocalizedDocW(const wchar_t* baseName, const char* missingTextKey)
{
    std::wstring file = FindLocalizedDocW(baseName);

    if (file.empty())
    {
        MessageBoxW(
            gAboutWnd ? gAboutWnd : gWnd,
            LangTextW(gLang, missingTextKey).c_str(),
            L"FlexiSoft Runtime",
            MB_ICONWARNING | MB_OK
        );

        return;
    }

    if (OpenMarkdownDocumentW(file))
        return;

    std::wstring msg;
    msg += L"Cannot open documentation file:\r\n";
    msg += file;

    MessageBoxW(
        gAboutWnd ? gAboutWnd : gWnd,
        msg.c_str(),
        L"FlexiSoft Runtime",
        MB_ICONWARNING | MB_OK
    );
}

static void LayoutAboutWindow(HWND h)
{
    if (!h)
        return;

    RECT rc;
    GetClientRect(h, &rc);

    int w = rc.right - rc.left;
    int hgt = rc.bottom - rc.top;

    int margin = 24;
    int gap = 14;

    int docBtnH = 34;
    int okBtnW = 92;
    int okBtnH = 34;

    int okY = hgt - margin - okBtnH;
    int docY = okY - gap - docBtnH;
    int infoY = margin;
    int infoH = docY - infoY - gap;

    if (infoH < 150)
        infoH = 150;

    if (gAboutInfo)
        MoveWindow(
            gAboutInfo,
            margin,
            infoY,
            w - 2 * margin,
            infoH,
            TRUE
        );

    bool hasHomepage = gAboutHomepage != NULL;

    int docCount = hasHomepage ? 4 : 3;
    int docGap = 14;
    int docAreaW = w - 2 * margin;
    int docBtnW = (docAreaW - (docCount - 1) * docGap) / docCount;

    int x = margin;

    if (gAboutReadme)
    {
        MoveWindow(gAboutReadme, x, docY, docBtnW, docBtnH, TRUE);
        x += docBtnW + docGap;
    }

    if (gAboutManual)
    {
        MoveWindow(gAboutManual, x, docY, docBtnW, docBtnH, TRUE);
        x += docBtnW + docGap;
    }

    if (gAboutLog)
    {
        MoveWindow(gAboutLog, x, docY, docBtnW, docBtnH, TRUE);
        x += docBtnW + docGap;
    }

    if (gAboutHomepage)
    {
        MoveWindow(gAboutHomepage, x, docY, docBtnW, docBtnH, TRUE);
    }

    if (gAboutOk)
        MoveWindow(
            gAboutOk,
            w - margin - okBtnW,
            okY,
            okBtnW,
            okBtnH,
            TRUE
        );
}

static std::wstring BuildAboutText()
{
    std::wstring aboutText;

    aboutText += s2ws(gVersion.product);
    aboutText += L"\r\n";

    aboutText += LangTextW(gLang, "about.version");
    aboutText += L": ";
    aboutText += s2ws(gVersion.version);
    aboutText += L"\r\n";

    aboutText += LangTextW(gLang, "about.build_date");
    aboutText += L": ";
    aboutText += s2ws(gVersion.buildDate);
    aboutText += L"\r\n\r\n";

    aboutText += s2ws(VersionDescription(gVersion, gLang.activeCode));

    aboutText += L"\r\n\r\n";
    aboutText += s2ws(gVersion.author);
    aboutText += L"\r\n";
    aboutText += s2ws(gVersion.email);

    return aboutText;
}

static void RefreshAboutWindowText()
{
    if (!gAboutWnd)
        return;

    SetWindowTextW(gAboutWnd, LangTextW(gLang, "about.title").c_str());

    if (gAboutInfo)
        SetWindowTextW(gAboutInfo, BuildAboutText().c_str());

    if (gAboutReadme)
        SetWindowTextW(gAboutReadme, LangTextW(gLang, "about.open_readme").c_str());

    if (gAboutManual)
        SetWindowTextW(gAboutManual, LangTextW(gLang, "about.open_manual").c_str());

    if (gAboutLog)
        SetWindowTextW(gAboutLog, LangTextW(gLang, "about.open_log").c_str());

    if (gAboutHomepage)
        SetWindowTextW(gAboutHomepage, LangTextW(gLang, "about.project_page").c_str());

    if (gAboutOk)
        SetWindowTextW(gAboutOk, LangTextW(gLang, "button.ok").c_str());

    ApplyAboutFonts();

    if (gAboutWnd) {
        LayoutAboutWindow(gAboutWnd);
        InvalidateRect(gAboutWnd, NULL, TRUE);
    }
}

static void ShowAboutWindow()
{
    if (gAboutWnd)
    {
        ShowWindow(gAboutWnd, SW_SHOWNORMAL);
        SetForegroundWindow(gAboutWnd);
        return;
    }

    int w = 680;
    int h = 420;

    int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

    gAboutWnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        L"FlexiSoftAboutWnd",
        LangTextW(gLang, "about.title").c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        x, y, w, h,
        gWnd,
        NULL,
        gInst,
        NULL
    );

    ShowWindow(gAboutWnd, SW_SHOWNORMAL);
}

static LRESULT CALLBACK AboutWndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_CREATE)
    {
        CreateAlertFonts();

        std::wstring aboutText = BuildAboutText();

        gAboutInfo = CreateWindowW(
            L"EDIT",
            aboutText.c_str(),
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPSIBLINGS |
            ES_LEFT | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
            24, 22, 460, 130,
            h,
            NULL,
            gInst,
            NULL
        );

        SendMessageW(
            gAboutInfo,
            EM_SETMARGINS,
            EC_LEFTMARGIN | EC_RIGHTMARGIN,
            MAKELPARAM(4, 4)
        );

        bool hasHomepage = !gVersion.homepage.empty();

        int btnY = 185;
        int btnH = 32;

        if (hasHomepage)
        {
            gAboutReadme = CreateWindowW(
                L"BUTTON",
                LangTextW(gLang, "about.open_readme").c_str(),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                24, btnY, 120, btnH,
                h,
                (HMENU)ID_ABOUT_README,
                gInst,
                NULL
            );

            gAboutManual = CreateWindowW(
                L"BUTTON",
                LangTextW(gLang, "about.open_manual").c_str(),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                154, btnY, 120, btnH,
                h,
                (HMENU)ID_ABOUT_MANUAL,
                gInst,
                NULL
            );

            gAboutLog = CreateWindowW(
                L"BUTTON",
                LangTextW(gLang, "about.open_log").c_str(),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                284, btnY, 120, btnH,
                h,
                (HMENU)ID_ABOUT_LOG,
                gInst,
                NULL
            );

            gAboutHomepage = CreateWindowW(
                L"BUTTON",
                LangTextW(gLang, "about.project_page").c_str(),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                414, btnY, 120, btnH,
                h,
                (HMENU)ID_ABOUT_HOMEPAGE,
                gInst,
                NULL
            );
        }

        else
        {
            gAboutReadme = CreateWindowW(
                L"BUTTON",
                LangTextW(gLang, "about.open_readme").c_str(),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                24, btnY, 145, btnH,
                h,
                (HMENU)ID_ABOUT_README,
                gInst,
                NULL
            );

            gAboutManual = CreateWindowW(
                L"BUTTON",
                LangTextW(gLang, "about.open_manual").c_str(),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                184, btnY, 145, btnH,
                h,
                (HMENU)ID_ABOUT_MANUAL,
                gInst,
                NULL
            );

            gAboutLog = CreateWindowW(
                L"BUTTON",
                LangTextW(gLang, "about.open_log").c_str(),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                344, btnY, 145, btnH,
                h,
                (HMENU)ID_ABOUT_LOG,
                gInst,
                NULL
            );
        }

        gAboutOk = CreateWindowW(
            L"BUTTON",
            LangTextW(gLang, "button.ok").c_str(),
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            404, 240, 85, 32,
            h,
            (HMENU)ID_ABOUT_OK,
            gInst,
            NULL
        );

        ApplyAboutFonts();

        SendMessage(h, WM_SETICON, ICON_SMALL, (LPARAM)gIconGreen);
        SendMessage(h, WM_SETICON, ICON_BIG, (LPARAM)gIconGreen);

        LayoutAboutWindow(h);

        return 0;
    }

    if (msg == WM_CTLCOLORSTATIC || msg == WM_CTLCOLOREDIT)
    {
        HDC dc = (HDC)wp;
        SetBkMode(dc, OPAQUE);
        SetBkColor(dc, GetSysColor(COLOR_BTNFACE));
        SetTextColor(dc, RGB(20, 20, 20));
        return (LRESULT)(HBRUSH)(COLOR_BTNFACE + 1);
    }

    if (msg == WM_SIZE)
    {
        LayoutAboutWindow(h);
        return 0;
    }

    if (msg == WM_COMMAND)
    {
        int id = LOWORD(wp);

        if (id == ID_ABOUT_README)
        {
            OpenLocalizedDocW(
                L"README",
                "error.missing_readme"
            );

            return 0;
        }

        if (id == ID_ABOUT_MANUAL)
        {
            OpenLocalizedDocW(
                L"MANUAL",
                "error.missing_manual"
            );

            return 0;
        }

        if (id == ID_ABOUT_LOG)
        {
            OpenLocalFileW(s2ws(gCfg.logFile));
            return 0;
        }

        if (id == ID_ABOUT_HOMEPAGE)
        {
            if (!gVersion.homepage.empty())
                OpenLocalFileW(s2ws(gVersion.homepage));

            return 0;
        }

        if (id == ID_ABOUT_OK)
        {
            DestroyWindow(h);
            return 0;
        }
    }

    if (msg == WM_CLOSE)
    {
        DestroyWindow(h);
        return 0;
    }

    if (msg == WM_DESTROY)
    {
        if (h == gAboutWnd)
        {
            gAboutWnd = NULL;
            gAboutInfo = NULL;
            gAboutReadme = NULL;
            gAboutManual = NULL;
            gAboutLog = NULL;
            gAboutHomepage = NULL;
            gAboutOk = NULL;
        }

        return 0;
    }

    return DefWindowProc(h, msg, wp, lp);
}

static LRESULT CALLBACK AlertWndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    if(msg == WM_CREATE)
    {
        CreateAlertFonts();

        gAlertTitle = CreateWindowW(
            L"STATIC",
            L"",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            22, 22, 430, 28,
            h,
            NULL,
            gInst,
            NULL
        );

        gAlertText = CreateWindowW(
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPSIBLINGS |
            ES_LEFT | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
            22, 62, 500, 150,
            h,
            NULL,
            gInst,
            NULL
        );

        SendMessageW(
            gAlertText,
            EM_SETMARGINS,
            EC_LEFTMARGIN | EC_RIGHTMARGIN,
            MAKELPARAM(4, 4)
        );

        gAlertLine = CreateWindowW(
            L"STATIC",
            L"",
            WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
            22, 220, 500, 2,
            h,
            NULL,
            gInst,
            NULL
        );

        gAlertFooter = CreateWindowW(
            L"STATIC",
            L"",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            22, 230, 500, 48,
            h,
            NULL,
            gInst,
            NULL
        );

        gAlertYes = CreateWindowW(
            L"BUTTON",
            LangTextW(gLang, "button.yes").c_str(),
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            300, 280, 100, 32,
            h,
            (HMENU)ID_ALERT_YES,
            gInst,
            NULL
        );

        gAlertNo = CreateWindowW(
            L"BUTTON",
            LangTextW(gLang, "button.no").c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            414, 280, 100, 32,
            h,
            (HMENU)ID_ALERT_NO,
            gInst,
            NULL
        );

        ApplyAlertFonts();

        LayoutAlertWindow(h);
        return 0;
    }

    if(msg == WM_PAINT)
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(h, &ps);

        RECT rc;
        GetClientRect(h, &rc);

        int iconSize = 96;
        int margin = 24;
        int x = rc.right - margin - iconSize;
        int y = 18;

        if (gAlertIconKind == 1)
            DrawAlertWarningIcon(dc, x, y, iconSize, gAlertIconColor);
        else if (gAlertIconKind == 2)
            DrawAlertErrorIcon(dc, x, y, iconSize, gAlertIconColor);

        EndPaint(h, &ps);
        return 0;
    }

    if(msg == WM_SIZE)
    {
        LayoutAlertWindow(h);
        return 0;
    }

    if (msg == WM_CTLCOLORSTATIC || msg == WM_CTLCOLOREDIT)
    {
        HDC dc = (HDC)wp;
        SetBkMode(dc, OPAQUE);
        SetBkColor(dc, GetSysColor(COLOR_BTNFACE));
        SetTextColor(dc, RGB(20, 20, 20));
        return (LRESULT)(HBRUSH)(COLOR_BTNFACE + 1);
    }

    if(msg == WM_COMMAND)
    {
        int id = LOWORD(wp);

        if(id == ID_ALERT_YES)
        {
            DWORD mask = gAlertMask;
            int type = gAlertType;

            CloseAlertWindow();

            if(type == ALERT_TYPE_INPUT)
                gRuntime.alertYes(mask);
            else if(type == ALERT_TYPE_COMMAND_FAILED)
                gRuntime.commandRetryYes(mask);

            return 0;
        }

        if(id == ID_ALERT_NO)
        {
            DWORD mask = gAlertMask;
            int type = gAlertType;

            CloseAlertWindow();

            if(type == ALERT_TYPE_INPUT)
                gRuntime.alertNo(mask);
            else if(type == ALERT_TYPE_COMMAND_FAILED)
                gRuntime.commandRetryNo(mask);

            return 0;
        }
    }

    if(msg == WM_CLOSE)
    {
        DWORD mask = gAlertMask;
        int type = gAlertType;

        DestroyWindow(h);

        if(type == ALERT_TYPE_INPUT)
            gRuntime.alertNo(mask);
        else if(type == ALERT_TYPE_COMMAND_FAILED)
            gRuntime.commandRetryNo(mask);

        return 0;
    }

    if(msg == WM_DESTROY)
    {
        if(h == gAlertWnd)
        {
            gAlertWnd = NULL;
            gAlertTitle = NULL;
            gAlertText = NULL;
            gAlertFooter = NULL;
            gAlertLine = NULL;
            gAlertIcon = NULL;
            gAlertYes = NULL;
            gAlertNo = NULL;
            gAlertMask = 0;
            gAlertRepeatMask = 0;
            gAlertType = ALERT_TYPE_NONE;
            gAlertIconKind = 0;
            gAlertIconColor = RGB(255, 220, 0);
        }
        return 0;
    }

    return DefWindowProc(h, msg, wp, lp);
}

static void ShowModelessAlert(int type, DWORD mask, DWORD repeatMask)
{
    if(mask == 0)
    {
        CloseAlertWindow();
        return;
    }

    std::wstring title;
    std::wstring heading;
    std::wstring text;
    std::wstring footer;

    if(type == ALERT_TYPE_COMMAND_FAILED)
    {
        title = LangTextW(gLang, "alert.command_failed.window_title");
        heading = BuildCommandFailedTitle();
        text = BuildCommandFailedText(mask);
        footer = BuildCommandFailedFooter();

        gAlertIconKind = 2;
        gAlertIconColor = RGB(220, 0, 0);
    }

    else
    {
        title = repeatMask
            ? LangTextW(gLang, "alert.repeat.window_title")
            : LangTextW(gLang, "alert.input.window_title");
        heading = BuildInputAlertTitle(repeatMask);
        text = BuildInputAlertText(mask, repeatMask);
        footer = BuildInputAlertFooter();

        gAlertIconKind = 1;
        gAlertIconColor = repeatMask ? RGB(255, 140, 0) : RGB(255, 220, 0);
    }

    if(!gAlertWnd)
    {
        int w = 620;
        int h = 340;
        int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
        int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

        gAlertWnd = CreateWindowExW(
            WS_EX_TOPMOST,
            L"FlexiSoftAlertWnd",
            title.c_str(),
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
            x, y, w, h,
            gWnd,
            NULL,
            gInst,
            NULL
        );
    }

    gAlertType = type;
    gAlertMask = mask;
    gAlertRepeatMask = repeatMask;

    if(gAlertWnd)
    {
        SetWindowTextW(gAlertWnd, title.c_str());

        // FS icon in title bar.
        SendMessage(gAlertWnd, WM_SETICON, ICON_SMALL, (LPARAM)gIconGreen);
        SendMessage(gAlertWnd, WM_SETICON, ICON_BIG, (LPARAM)gIconGreen);

        if(gAlertTitle)
            SetWindowTextW(gAlertTitle, heading.c_str());

        if(gAlertText)
            SetWindowTextW(gAlertText, text.c_str());

        if(gAlertFooter)
            SetWindowTextW(gAlertFooter, footer.c_str());

        if (gAlertYes)
            SetWindowTextW(gAlertYes, LangTextW(gLang, "button.yes").c_str());

        if (gAlertNo)
            SetWindowTextW(gAlertNo, LangTextW(gLang, "button.no").c_str());

        ApplyAlertFonts();

        InvalidateRect(gAlertWnd, NULL, TRUE);

        LayoutAlertWindow(gAlertWnd);
        ShowWindow(gAlertWnd, SW_SHOWNORMAL);
        SetForegroundWindow(gAlertWnd);
    }
}

static void RefreshAlertWindowText()
{
    if (!gAlertWnd)
        return;

    if (gAlertType == ALERT_TYPE_NONE || gAlertMask == 0)
        return;

    ShowModelessAlert(gAlertType, gAlertMask, gAlertRepeatMask);
}

static void ApplyLanguageToOpenWindows()
{
    UpdateTray();
    RefreshAboutWindowText();
    RefreshAlertWindowText();
}

static void SwitchLanguage(const std::string& code)
{
    std::string requested = code;

    if (requested.empty())
        requested = "en";

    for (size_t i = 0; i < gLang.available.size(); i++)
    {
        if (gLang.available[i].code == requested)
        {
            gLang.requestedCode = requested;
            gLang.activeCode = gLang.available[i].code;
            gLang.active = gLang.available[i];
            gLang.activeLoadedFromFile = true;


            gCfg.language = gLang.activeCode;

            gRuntimeState.language = gLang.activeCode;
            gRuntimeState.hasLanguage = true;


            ApplyLoadedLanguageState();
            SaveAppRuntimeState();

            gLog.log(
                "language switched: requested=%s active=%s name=%s font_face=%s font_file=%s available=%lu",
                gLang.requestedCode.c_str(),
                gLang.activeCode.c_str(),
                gLang.active.name.c_str(),
                gLang.active.fontFace.c_str(),
                gLang.active.fontFile.c_str(),
                (unsigned long)gLang.available.size()
            );

            ApplyLanguageToOpenWindows();
            return;
        }
    }

    gLog.log(
        "language switch ignored: requested=%s not found available=%lu active=%s",
        requested.c_str(),
        (unsigned long)gLang.available.size(),
        gLang.activeCode.c_str()
    );
}

static void ShowMenu()
{
    RuntimeStatus st = gRuntime.status();

    HMENU m = CreatePopupMenu();

    AppendMenuW(
        m,
        MF_STRING | MF_DISABLED | MF_GRAYED,
        0,
        LangTextW(gLang, "tray.title").c_str()
    );

    AppendMenuW(m, MF_SEPARATOR, 0, NULL);

    std::wstring disabled = LangTextW(gLang, "status.disabled");
    std::wstring ok = LangTextW(gLang, "status.ok");
    std::wstring error = LangTextW(gLang, "status.error");
    std::wstring on = LangTextW(gLang, "status.on");
    std::wstring off = LangTextW(gLang, "status.off");

    for (int i = 0; i < 4; i++)
    {
        wchar_t line[128];

        if (!st.ch[i].enabled)
        {
            _snwprintf(
                line,
                sizeof(line) / sizeof(line[0]) - 1,
                L"CH%d: %s",
                i + 1,
                disabled.c_str()
            );

            line[sizeof(line) / sizeof(line[0]) - 1] = 0;
        }
        else
        {
            _snwprintf(
                line,
                sizeof(line) / sizeof(line[0]) - 1,
                L"CH%d: %s / %s",
                i + 1,
                st.ch[i].ok ? ok.c_str() : error.c_str(),
                st.ch[i].on ? on.c_str() : off.c_str()
            );

            line[sizeof(line) / sizeof(line[0]) - 1] = 0;
        }

        AppendMenuW(m, MF_STRING | MF_DISABLED | MF_GRAYED, 0, line);
    }

    AppendMenuW(m, MF_SEPARATOR, 0, NULL);

    bool canSendCommands = st.comOk && st.cpuOk;

    for (int i = 0; i < 4; i++)
    {
        std::wstring txt = s2ws(
            LangTextReplace(
                gLang,
                "tray.send_command",
                "{channel}",
                gCfg.inputs[i].name
            )
        );

        AppendMenuW(
            m,
            (gCfg.inputs[i].enabled && canSendCommands)
            ? MF_STRING
            : (MF_STRING | MF_DISABLED | MF_GRAYED),
            ID_SEND_BASE + i,
            txt.c_str()
        );
    }

    AppendMenuW(m, MF_SEPARATOR, 0, NULL);

    HMENU langMenu = CreatePopupMenu();

    for (size_t i = 0; i < gLang.available.size(); i++)
    {
        UINT flags = MF_STRING;

        if (gLang.available[i].code == gLang.activeCode)
            flags |= MF_CHECKED;

        AppendMenuW(
            langMenu,
            flags,
            ID_LANG_BASE + (UINT)i,
            s2ws(gLang.available[i].name).c_str()
        );
    }

    AppendMenuW(
        m,
        MF_POPUP,
        (UINT_PTR)langMenu,
        LangTextW(gLang, "tray.language").c_str()
    );

    AppendMenuW(m, MF_SEPARATOR, 0, NULL);

    AppendMenuW(
        m,
        MF_STRING,
        ID_TRAY_RELOAD,
        LangTextW(gLang, "tray.reload_config").c_str()
    );

    AppendMenuW(
        m,
        MF_STRING,
        ID_TRAY_RECONNECT,
        LangTextW(gLang, "tray.reconnect").c_str()
    );

    AppendMenuW(m, MF_SEPARATOR, 0, NULL);

    AppendMenuW(
        m,
        MF_STRING,
        ID_TRAY_ABOUT,
        LangTextW(gLang, "tray.about").c_str()
    );

    AppendMenuW(m, MF_SEPARATOR, 0, NULL);

    AppendMenuW(
        m,
        MF_STRING,
        ID_TRAY_EXIT,
        LangTextW(gLang, "tray.exit").c_str()
    );

    POINT pt;
    GetCursorPos(&pt);

    SetForegroundWindow(gWnd);
    TrackPopupMenu(m, TPM_RIGHTBUTTON, pt.x, pt.y, 0, gWnd, NULL);
    DestroyMenu(m);
}

static LRESULT CALLBACK WndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_TRAY)
    {
        if (lp == WM_RBUTTONUP || lp == WM_LBUTTONUP)
            ShowMenu();

        return 0;
    }

    if (msg == WM_FLEXI_STATUS)
    {
        UpdateTray();
        return 0;
    }

    if (msg == WM_FLEXI_ALERT_SHOW_INPUT)
    {
        ShowModelessAlert(ALERT_TYPE_INPUT, (DWORD)wp, (DWORD)lp);
        return 0;
    }

    if (msg == WM_FLEXI_ALERT_CLOSE_INPUT)
    {
        if(gAlertType == ALERT_TYPE_INPUT)
            CloseAlertWindow();
        return 0;
    }

    if (msg == WM_FLEXI_ALERT_SHOW_COMMAND_FAILED)
    {
        ShowModelessAlert(ALERT_TYPE_COMMAND_FAILED, (DWORD)wp, 0);
        return 0;
    }

    if (msg == WM_COMMAND)
    {
        int id = LOWORD(wp);

        if (id >= ID_LANG_BASE && id < ID_LANG_BASE + (int)gLang.available.size())
        {
            int index = id - ID_LANG_BASE;

            if (index >= 0 && index < (int)gLang.available.size())
                SwitchLanguage(gLang.available[index].code);

            return 0;
        }

        if (id == ID_TRAY_ABOUT)
        {
            ShowAboutWindow();
            return 0;
        }

        if (id == ID_TRAY_EXIT)
        {
            gLog.log("User action: shutdown");

            DestroyWindow(h);
            return 0;
        }

        if (id == ID_TRAY_RELOAD)
        {
            gLog.log("User action: configuration/languages reload");

            CloseAlertWindow();

            LoadAppConfig();
            LoadAppRuntimeState();
            LoadAppLanguages();
            gRuntime.stop();
            gRuntime.start(gWnd, gCfg);
            UpdateTray();
            ApplyLanguageToOpenWindows();
            return 0;
        }

        if (id == ID_TRAY_RECONNECT)
        {
            gLog.log("User action: reconnect");

            CloseAlertWindow();

            gRuntime.stop();
            gRuntime.start(gWnd, gCfg);
            UpdateTray();
            return 0;
        }

        if (id >= ID_SEND_BASE && id < ID_SEND_BASE + 4)
        {
            int ch = id - ID_SEND_BASE;
            RuntimeStatus st = gRuntime.status();

            if (!st.comOk || !st.cpuOk)
            {
                gLog.log(
                    "Manual command ignored CH%d: not connected com_ok=%s cpu_ok=%s last_error=%s",
                    ch + 1,
                    st.comOk ? "true" : "false",
                    st.cpuOk ? "true" : "false",
                    st.lastError
                );

                return 0;
            }

            gRuntime.pulseOutput(ch);
            return 0;
        }
    }

    if (msg == WM_DESTROY)
    {
        if (gAboutWnd)
            DestroyWindow(gAboutWnd);

        CloseAlertWindow();
        DestroyAlertFonts();
        UiFontsShutdown();
        gRuntime.stop();
        Shell_NotifyIconW(NIM_DELETE, &gNid);
        DestroyTrayIcons();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(h, msg, wp, lp);
}

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int)
{
    gInst = hInst;

    InitAppPath();

    EnsureRuntimeDirectories();

    LoadAppConfig();
    LoadAppRuntimeState();
    LoadAppLanguages();
    LoadAppVersion();
    CreateTrayIcons();

    WNDCLASSW wc;
    ZeroMemory(&wc, sizeof(wc));

    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"FlexiSoftRuntimeWnd";

    RegisterClassW(&wc);

    WNDCLASSW awc;
    ZeroMemory(&awc, sizeof(awc));
    awc.lpfnWndProc = AlertWndProc;
    awc.hInstance = hInst;
    awc.lpszClassName = L"FlexiSoftAlertWnd";
    awc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    awc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&awc);

    WNDCLASSW bwc;
    ZeroMemory(&bwc, sizeof(bwc));
    bwc.lpfnWndProc = AboutWndProc;
    bwc.hInstance = hInst;
    bwc.lpszClassName = L"FlexiSoftAboutWnd";
    bwc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    bwc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&bwc);

    gWnd = CreateWindowW(
        wc.lpszClassName,
        L"FlexiSoftRuntime",
        0,
        0,
        0,
        0,
        0,
        NULL,
        NULL,
        hInst,
        NULL
    );

    ZeroMemory(&gNid, sizeof(gNid));

#ifdef NOTIFYICONDATAW_V2_SIZE
    gNid.cbSize = NOTIFYICONDATAW_V2_SIZE;
#else
    gNid.cbSize = sizeof(gNid);
#endif

    gNid.hWnd = gWnd;
    gNid.uID = 1;
    gNid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    gNid.uCallbackMessage = WM_TRAY;
    gNid.hIcon = gIconGray;

    lstrcpynW(
        gNid.szTip,
        s2ws(gCfg.trayTooltip).c_str(),
        sizeof(gNid.szTip) / sizeof(WCHAR)
    );

    Shell_NotifyIconW(NIM_ADD, &gNid);

    gRuntime.start(gWnd, gCfg);
    UpdateTray();

    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}