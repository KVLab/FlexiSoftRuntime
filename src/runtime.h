#pragma once
#include "common.h"
#include "config.h"
#include "transport.h"
#include "rk512.h"

#define WM_FLEXI_STATUS (WM_APP+10)
#define WM_FLEXI_ALERT_SHOW_INPUT (WM_APP+20)
#define WM_FLEXI_ALERT_CLOSE_INPUT (WM_APP+21)
#define WM_FLEXI_ALERT_SHOW_COMMAND_FAILED (WM_APP+22)

struct ChannelState
{
    bool enabled;
    bool on;
    bool ok;
};

struct RuntimeStatus
{
    bool comOk;
    bool cpuOk;
    ChannelState ch[4];
    char lastError[160];
};

class Runtime
{
public:
    Runtime();
    ~Runtime();

    bool start(HWND notify, const AppConfig& cfg);
    void stop();
    RuntimeStatus status();

    // Public API used by tray/menu. It does NOT touch RK512 directly.
    // It only queues a command request for the runtime worker thread.
    void pulseOutput(int idx);
    void alertYes(DWORD mask);
    void alertNo(DWORD mask);
    void commandRetryYes(DWORD mask);
    void commandRetryNo(DWORD mask);

    void reload(const AppConfig& cfg);

private:
    static DWORD WINAPI threadProc(LPVOID p);

    void loop();
    bool connect();
    void closeTransport();
    void pollOnce();
    bool fetchProcessImage(std::vector<BYTE>& data, std::string& err);
    DWORD evaluateChannels(const std::vector<BYTE>& data);
    DWORD applyPostCommandState(DWORD rawErrorMask);
    void handleErrorMask(DWORD errorMask);
    void registerPostCommandFaults(DWORD mask);
    DWORD buildRepeatMask(DWORD errorMask);
    DWORD takeManualCommandMask();
    bool sendCommandForMask(DWORD channelMask, std::string& err);
    void setStatusError(const std::string& e);
    void setCpuError(const std::string& e);
    void setConnectedStatus(bool cpuOk, const char* errText);
    DWORD ioTimeout() const;

    AppConfig cfg_;
    HWND notify_;
    HANDLE thread_;
    HANDLE stopEvent_;

    // Protects runtime status and masks shared with UI thread.
    CRITICAL_SECTION cs_;

    RuntimeStatus st_;
    ITransport* io_;

    // Error handling masks. Bit 0 = CH1, bit 1 = CH2, ...
    DWORD confirmedMask_;
    DWORD abortedMask_;
    DWORD manualCommandMask_;
    DWORD awaitingPostCommandMask_;
    DWORD activeAlertMask_;
    DWORD activeRepeatMask_;
    DWORD commandFailedMask_;
    DWORD postCommandIgnoreUntil_[4];
    DWORD repeatWindowPauseStart_[4];
    DWORD repeatFirstTick_[4];
    int repeatCount_[4];
    bool writeTokenReady_;
    bool errorLogActive_;
    char lastLoggedError_[160];
};

extern Runtime gRuntime;
