#include "runtime.h"
#include "logger.h"

Runtime gRuntime;

Runtime::Runtime()
    : notify_(NULL),
      thread_(NULL),
      stopEvent_(NULL),
      io_(NULL),
      confirmedMask_(0),
      abortedMask_(0),
      manualCommandMask_(0),
      awaitingPostCommandMask_(0),
      activeAlertMask_(0),
      activeRepeatMask_(0),
      commandFailedMask_(0),
      writeTokenReady_(false),
      errorLogActive_(false)
{
    stopEvent_ = CreateEvent(NULL, TRUE, FALSE, NULL);
    InitializeCriticalSection(&cs_);
    ZeroMemory(&st_, sizeof(st_));
    ZeroMemory(postCommandIgnoreUntil_, sizeof(postCommandIgnoreUntil_));
    ZeroMemory(repeatWindowPauseStart_, sizeof(repeatWindowPauseStart_));
    ZeroMemory(repeatFirstTick_, sizeof(repeatFirstTick_));
    ZeroMemory(repeatCount_, sizeof(repeatCount_));
    ZeroMemory(lastLoggedError_, sizeof(lastLoggedError_));
}

Runtime::~Runtime()
{
    stop();

    if (stopEvent_)
        CloseHandle(stopEvent_);

    DeleteCriticalSection(&cs_);
}

bool Runtime::start(HWND notify, const AppConfig& cfg)
{
    stop();

    cfg_ = cfg;
    notify_ = notify;

    EnterCriticalSection(&cs_);
    ZeroMemory(&st_, sizeof(st_));
    confirmedMask_ = 0;
    abortedMask_ = 0;
    manualCommandMask_ = 0;
    awaitingPostCommandMask_ = 0;
    activeAlertMask_ = 0;
    activeRepeatMask_ = 0;
    commandFailedMask_ = 0;
    writeTokenReady_ = false;
    errorLogActive_ = false;
    ZeroMemory(lastLoggedError_, sizeof(lastLoggedError_));
    ZeroMemory(postCommandIgnoreUntil_, sizeof(postCommandIgnoreUntil_));
    ZeroMemory(repeatWindowPauseStart_, sizeof(repeatWindowPauseStart_));
    ZeroMemory(repeatFirstTick_, sizeof(repeatFirstTick_));
    ZeroMemory(repeatCount_, sizeof(repeatCount_));

    for (int i = 0; i < 4; i++)
    {
        st_.ch[i].enabled = cfg_.inputs[i].enabled;
        st_.ch[i].on = false;
        st_.ch[i].ok = true;
    }
    LeaveCriticalSection(&cs_);

    ResetEvent(stopEvent_);
    thread_ = CreateThread(NULL, 0, threadProc, this, 0, NULL);

    return thread_ != NULL;
}

void Runtime::stop()
{
    if (thread_)
    {
        SetEvent(stopEvent_);
        WaitForSingleObject(thread_, 5000);
        CloseHandle(thread_);
        thread_ = NULL;
    }

    closeTransport();
}

void Runtime::reload(const AppConfig& cfg)
{
    cfg_ = cfg;
    closeTransport();

    EnterCriticalSection(&cs_);
    confirmedMask_ = 0;
    abortedMask_ = 0;
    manualCommandMask_ = 0;
    awaitingPostCommandMask_ = 0;
    activeAlertMask_ = 0;
    activeRepeatMask_ = 0;
    commandFailedMask_ = 0;
    writeTokenReady_ = false;
    errorLogActive_ = false;
    ZeroMemory(lastLoggedError_, sizeof(lastLoggedError_));
    ZeroMemory(postCommandIgnoreUntil_, sizeof(postCommandIgnoreUntil_));
    ZeroMemory(repeatWindowPauseStart_, sizeof(repeatWindowPauseStart_));
    ZeroMemory(repeatFirstTick_, sizeof(repeatFirstTick_));
    ZeroMemory(repeatCount_, sizeof(repeatCount_));
    LeaveCriticalSection(&cs_);

    if (notify_)
        PostMessage(notify_, WM_FLEXI_ALERT_CLOSE_INPUT, 0, 0);
}

RuntimeStatus Runtime::status()
{
    EnterCriticalSection(&cs_);
    RuntimeStatus s = st_;
    LeaveCriticalSection(&cs_);

    return s;
}

DWORD Runtime::ioTimeout() const
{
    return (cfg_.transport == TRANSPORT_SERIAL)
        ? cfg_.serial.timeoutMs
        : cfg_.network.timeoutMs;
}

static bool startsWithText(const std::string& s, const char* prefix)
{
    if (!prefix)
        return false;

    size_t n = strlen(prefix);

    if (s.size() < n)
        return false;

    return s.compare(0, n, prefix) == 0;
}

static bool isCpuResponseError(const std::string& e)
{
    if (e == "serial timeout")
        return true;

    if (e == "network read timeout")
        return true;

    if (startsWithText(e, "bad fetch status"))
        return true;

    if (startsWithText(e, "bad fetch header"))
        return true;

    return false;
}

void Runtime::setStatusError(const std::string& e)
{
    bool shouldLog = false;

    EnterCriticalSection(&cs_);

    st_.comOk = false;
    st_.cpuOk = false;

    strncpy(st_.lastError, e.c_str(), sizeof(st_.lastError) - 1);
    st_.lastError[sizeof(st_.lastError) - 1] = 0;

    activeAlertMask_ = 0;
    activeRepeatMask_ = 0;

    if (!errorLogActive_ || strncmp(lastLoggedError_, e.c_str(), sizeof(lastLoggedError_) - 1) != 0)
    {
        shouldLog = true;

        strncpy(lastLoggedError_, e.c_str(), sizeof(lastLoggedError_) - 1);
        lastLoggedError_[sizeof(lastLoggedError_) - 1] = 0;

        errorLogActive_ = true;
    }

    LeaveCriticalSection(&cs_);

    if (shouldLog)
        gLog.log("ERR %s", e.c_str());

    if (notify_)
    {
        PostMessage(notify_, WM_FLEXI_STATUS, 0, 0);
        PostMessage(notify_, WM_FLEXI_ALERT_CLOSE_INPUT, 0, 0);
    }
}

void Runtime::setCpuError(const std::string& e)
{
    bool shouldLog = false;

    EnterCriticalSection(&cs_);

    st_.comOk = true;
    st_.cpuOk = false;

    strncpy(st_.lastError, e.c_str(), sizeof(st_.lastError) - 1);
    st_.lastError[sizeof(st_.lastError) - 1] = 0;

    activeAlertMask_ = 0;
    activeRepeatMask_ = 0;
    writeTokenReady_ = false;

    if (!errorLogActive_ || strncmp(lastLoggedError_, e.c_str(), sizeof(lastLoggedError_) - 1) != 0)
    {
        shouldLog = true;

        strncpy(lastLoggedError_, e.c_str(), sizeof(lastLoggedError_) - 1);
        lastLoggedError_[sizeof(lastLoggedError_) - 1] = 0;

        errorLogActive_ = true;
    }

    LeaveCriticalSection(&cs_);

    if (shouldLog)
        gLog.log("ERR %s", e.c_str());

    if (notify_)
    {
        PostMessage(notify_, WM_FLEXI_STATUS, 0, 0);
        PostMessage(notify_, WM_FLEXI_ALERT_CLOSE_INPUT, 0, 0);
    }
}

void Runtime::setConnectedStatus(bool cpuOk, const char* errText)
{
    EnterCriticalSection(&cs_);

    st_.comOk = true;
    st_.cpuOk = cpuOk;

    if (errText && errText[0])
    {
        strncpy(st_.lastError, errText, sizeof(st_.lastError) - 1);
        st_.lastError[sizeof(st_.lastError) - 1] = 0;
    }
    else
    {
        st_.lastError[0] = 0;
    }

    LeaveCriticalSection(&cs_);

    if (notify_)
        PostMessage(notify_, WM_FLEXI_STATUS, 0, 0);
}

DWORD WINAPI Runtime::threadProc(LPVOID p)
{
    ((Runtime*)p)->loop();
    return 0;
}

void Runtime::closeTransport()
{
    if (io_)
    {
        io_->close();
        delete io_;
        io_ = NULL;
    }

    EnterCriticalSection(&cs_);
    writeTokenReady_ = false;
    LeaveCriticalSection(&cs_);
}

bool Runtime::connect()
{
    std::string err;

    if (!io_)
        io_ = TransportFactory::create(cfg_);

    if (!io_)
    {
        setStatusError("transport create failed");
        return false;
    }

    setConnectedStatus(false, "opening transport");

    if (!io_->open(cfg_, err))
    {
        closeTransport();
        setStatusError(err);
        return false;
    }

    gLog.log("Transport opened via %s", io_->name());
    setConnectedStatus(false, "transport opened");

    return true;
}

void Runtime::loop()
{
    while (WaitForSingleObject(stopEvent_, 0) != WAIT_OBJECT_0)
    {
        if (!io_ || !io_->isOpen())
        {
            if (!connect())
            {
                WaitForSingleObject(stopEvent_, 1000);
                continue;
            }
        }

        pollOnce();

        DWORD wait = cfg_.pollPeriodMs ? cfg_.pollPeriodMs : 500;
        WaitForSingleObject(stopEvent_, wait);
    }
}

bool Runtime::fetchProcessImage(std::vector<BYTE>& data, std::string& err)
{
    if (!io_ || !io_->isOpen())
    {
        err = "transport closed";
        return false;
    }

    io_->purgeRx();

    RK512 rk(*io_, cfg_.deviceLocal, cfg_.deviceReply, ioTimeout());
    return rk.fetch(cfg_.readBlock, cfg_.readSize, data, err);
}

DWORD Runtime::evaluateChannels(const std::vector<BYTE>& data)
{
    DWORD errorMask = 0;
    bool needUi = false;

    EnterCriticalSection(&cs_);

    for (int i = 0; i < 4; i++)
    {
        const InputConfig& ic = cfg_.inputs[i];

        bool on = false;
        bool ok = true;

        if (ic.enabled && ic.statusByte >= 0 && ic.statusByte < (int)data.size())
        {
            BYTE b = data[ic.statusByte];
            on = ((b >> ic.onBit) & 1) != 0;
            ok = ((b >> ic.okBit) & 1) != 0;
        }

        if (ic.enabled && !ok)
            errorMask |= (1u << i);

        if (st_.ch[i].enabled != ic.enabled || st_.ch[i].on != on || st_.ch[i].ok != ok)
            needUi = true;

        st_.ch[i].enabled = ic.enabled;
        st_.ch[i].on = on;
        st_.ch[i].ok = ok;
    }

    /*
    Repeat counters are not reset here.

    A channel may look OK for a short time after a command because
    the hardware disconnects and reconnects it. That transient OK
    must not clear repeat_count. Real reset is handled later in
    applyPostCommandState(), after post-command ignore time expires,
    or immediately when OK appears without a pending command.
    */

    char restoredFrom[160];
    bool logRestored = false;

    ZeroMemory(restoredFrom, sizeof(restoredFrom));

    if (errorLogActive_)
    {
        strncpy(restoredFrom, lastLoggedError_, sizeof(restoredFrom) - 1);
        restoredFrom[sizeof(restoredFrom) - 1] = 0;

        errorLogActive_ = false;
        lastLoggedError_[0] = 0;

        logRestored = true;
    }

    st_.comOk = true;
    st_.cpuOk = true;
    st_.lastError[0] = 0;

    LeaveCriticalSection(&cs_);

    if (logRestored)
    {
        if (strncmp(restoredFrom, "command: ", 9) == 0)
        {
            gLog.log(
                "Command status cleared after reconnect: %s",
                restoredFrom + 9
            );
        }
        else
        {
            gLog.log(
                "Communication restored after: %s",
                restoredFrom
            );
        }
    }

    if (needUi && notify_)
        PostMessage(notify_, WM_FLEXI_STATUS, 0, 0);

    return errorMask;
}

DWORD Runtime::applyPostCommandState(DWORD rawErrorMask)
{
    DWORD now = GetTickCount();
    DWORD errorMask = rawErrorMask;
    DWORD postFaultMask = 0;

    EnterCriticalSection(&cs_);

    for (int i = 0; i < 4; i++)
    {
        DWORD bit = (1u << i);
        bool rawError = (rawErrorMask & bit) != 0;
        bool awaiting = (awaitingPostCommandMask_ & bit) != 0;

        if (awaiting)
        {
            DWORD until = postCommandIgnoreUntil_[i];
            bool stillIgnoring = false;

            if (until != 0 && (LONG)(now - until) < 0)
                stillIgnoring = true;

            if (stillIgnoring)
            {
                /*
                During post-command ignore time, raw channel state is
                still visible in runtime status, but it must not drive
                alerts, repeat counters, or repeat reset.
                */
                errorMask &= ~bit;
                continue;
            }

            awaitingPostCommandMask_ &= ~bit;
            postCommandIgnoreUntil_[i] = 0;

            if (repeatWindowPauseStart_[i] != 0)
            {
                DWORD pausedMs = now - repeatWindowPauseStart_[i];

                /*
                Do not let operator/command/verification time shrink
                repeat_fault.window_ms. The repeat window measures only
                active evaluated time between failed recovery cycles.
                */

                if (repeatFirstTick_[i] != 0)
                    repeatFirstTick_[i] += pausedMs;

                repeatWindowPauseStart_[i] = 0;
            }

            if (rawError)
            {
                postFaultMask |= bit;
            }

            else
            {
                repeatFirstTick_[i] = 0;
                repeatCount_[i] = 0;
            }

            continue;
        }

        /*
        A command may already be queued, but not sent yet.
        pollOnce() evaluates inputs before it consumes manualCommandMask_.
        In that short state repeatWindowPauseStart_ is already running,
        while awaitingPostCommandMask_ is not set yet.

        Do not clear the pause here, otherwise operator/queue/send time
        would still shrink repeat_fault.window_ms.
        */

        if (manualCommandMask_ & bit)
            continue;

        postCommandIgnoreUntil_[i] = 0;

        if (!rawError)
        {
            repeatFirstTick_[i] = 0;
            repeatCount_[i] = 0;
        }
    }

    /*
    Forget YES/NO only for errors that are no longer logically active.
    Channels hidden by post-command ignore are excluded from errorMask,
    so they do not immediately re-open alerts while the command settles.
    */
    confirmedMask_ &= errorMask;
    abortedMask_ &= errorMask;

    LeaveCriticalSection(&cs_);

    if (postFaultMask)
        registerPostCommandFaults(postFaultMask);

    return errorMask;
}

void Runtime::registerPostCommandFaults(DWORD mask)
{
    DWORD now = GetTickCount();

    EnterCriticalSection(&cs_);

    for (int i = 0; i < 4; i++)
    {
        DWORD bit = (1u << i);

        if ((mask & bit) == 0)
            continue;

        DWORD win = cfg_.inputs[i].repeatFault.windowMs;
        if (win < 1000)
            win = 1000;

        int limit = cfg_.inputs[i].repeatFault.count;
        if (limit < 1)
            limit = 1;

        if (repeatFirstTick_[i] == 0 || (DWORD)(now - repeatFirstTick_[i]) > win)
        {
            repeatFirstTick_[i] = now;
            repeatCount_[i] = 1;
        }
        else
        {
            repeatCount_[i]++;
        }

        if (repeatCount_[i] >= limit)
        {
            gLog.log(
                "Repeat fault CH%d count=%d/%d",
                i + 1,
                repeatCount_[i],
                limit
            );
        }
        else
        {
            gLog.log(
                "Repeat counter CH%d count=%d/%d",
                i + 1,
                repeatCount_[i],
                limit
            );
        }
    }

    LeaveCriticalSection(&cs_);
}

DWORD Runtime::buildRepeatMask(DWORD errorMask)
{
    DWORD repeatMask = 0;

    EnterCriticalSection(&cs_);

    for (int i = 0; i < 4; i++)
    {
        DWORD bit = (1u << i);
        int limit = cfg_.inputs[i].repeatFault.count;

        if (limit < 1)
            limit = 1;

        if ((errorMask & bit) && repeatCount_[i] >= limit)
            repeatMask |= bit;
    }

    LeaveCriticalSection(&cs_);

    return repeatMask;
}

void Runtime::handleErrorMask(DWORD errorMask)
{
    if (errorMask == 0)
    {
        EnterCriticalSection(&cs_);
        activeAlertMask_ = 0;
        activeRepeatMask_ = 0;
        LeaveCriticalSection(&cs_);

        if (notify_)
        {
            PostMessage(notify_, WM_FLEXI_STATUS, 0, 0);
            PostMessage(notify_, WM_FLEXI_ALERT_CLOSE_INPUT, 0, 0);
        }
        return;
    }

    DWORD alreadyHandled = 0;
    DWORD repeatMask = buildRepeatMask(errorMask);
    DWORD prevActive = 0;
    DWORD prevRepeat = 0;

    EnterCriticalSection(&cs_);
    alreadyHandled = confirmedMask_ | abortedMask_;
    prevActive = activeAlertMask_;
    prevRepeat = activeRepeatMask_;
    LeaveCriticalSection(&cs_);

    DWORD newMask = errorMask & ~alreadyHandled;

    if (newMask == 0)
    {
        if (notify_)
            PostMessage(notify_, WM_FLEXI_STATUS, 0, 0);
        return;
    }

    EnterCriticalSection(&cs_);
    activeAlertMask_ = errorMask;
    activeRepeatMask_ = repeatMask;
    LeaveCriticalSection(&cs_);

    if (notify_)
    {
        PostMessage(notify_, WM_FLEXI_STATUS, 0, 0);

        // Show/update alert if new error appears, repeated warning changes,
        // or the current error set differs from the modeless alert.
        if (prevActive != errorMask || prevRepeat != repeatMask)
            PostMessage(notify_, WM_FLEXI_ALERT_SHOW_INPUT, (WPARAM)errorMask, (LPARAM)repeatMask);
    }
}

DWORD Runtime::takeManualCommandMask()
{
    DWORD mask;

    EnterCriticalSection(&cs_);
    mask = manualCommandMask_;
    manualCommandMask_ = 0;
    LeaveCriticalSection(&cs_);

    return mask;
}

bool Runtime::sendCommandForMask(DWORD channelMask, std::string& err)
{
    if (channelMask == 0)
        return true;

    if (!io_ || !io_->isOpen())
    {
        err = "transport closed";
        return false;
    }

    std::vector<BYTE> onBytes(4, 0);
    std::vector<BYTE> offBytes(4, 0);
    DWORD pulseMs = 0;
    BYTE block = 0x42;

    for (int i = 0; i < 4; i++)
    {
        if ((channelMask & (1u << i)) == 0)
            continue;

        const InputConfig& ic = cfg_.inputs[i];
        const OutputConfig& oc = ic.output;

        if (!ic.enabled)
            continue;

        block = oc.block;

        if (oc.byteIndex >= 0 && oc.byteIndex < (int)onBytes.size() && oc.bit >= 0 && oc.bit < 8)
            onBytes[oc.byteIndex] |= (BYTE)(1u << oc.bit);

        if (oc.pulseMs > pulseMs)
            pulseMs = oc.pulseMs;
    }

    if (pulseMs == 0)
        pulseMs = 1000;

    io_->purgeRx();

    RK512 rk(*io_, cfg_.deviceLocal, cfg_.deviceReply, ioTimeout());

    bool needToken = false;
    EnterCriticalSection(&cs_);
    needToken = !writeTokenReady_;
    LeaveCriticalSection(&cs_);

    if (needToken)
    {
        std::string tokenErr;
        if (rk.acquireToken(0x41, cfg_.token, tokenErr))
        {
            EnterCriticalSection(&cs_);
            writeTokenReady_ = true;
            LeaveCriticalSection(&cs_);
            gLog.log("Command token OK");
        }
        else
        {
            gLog.log("Command token warning: %s", tokenErr.c_str());
            io_->purgeRx();

            EnterCriticalSection(&cs_);
            writeTokenReady_ = true;
            LeaveCriticalSection(&cs_);
        }
    }

    gLog.log("Command ON mask=0x%08lX block=0x%02X bytes=%02X %02X %02X %02X pulse=%lu",
        (unsigned long)channelMask,
        (unsigned int)block,
        onBytes[0], onBytes[1], onBytes[2], onBytes[3],
        (unsigned long)pulseMs);

    if (!rk.store(block, onBytes, err))
        return false;

    DWORD step = 50;
    DWORD elapsed = 0;

    while (elapsed < pulseMs)
    {
        DWORD wait = pulseMs - elapsed;
        if (wait > step)
            wait = step;

        if (WaitForSingleObject(stopEvent_, wait) == WAIT_OBJECT_0)
            break;

        elapsed += wait;
    }

    bool offOk = false;
    std::string offErr;

    for (int attempt = 0; attempt < 3 && !offOk; attempt++)
    {
        offErr.clear();
        io_->purgeRx();

        offOk = rk.store(block, offBytes, offErr);

        if (!offOk)
        {
            gLog.log("Command OFF retry %d failed: %s", attempt + 1, offErr.c_str());
            Sleep(50);
        }
    }

    if (!offOk)
    {
        err = "off failed: " + offErr;
        return false;
    }

    gLog.log("Command OFF OK mask=0x%08lX block=0x%02X", (unsigned long)channelMask, (unsigned int)block);

    // just for testing bad command
    if (cfg_.debugForceCommandFail)
    {
        err = "debug forced command fail";
        gLog.log("DEBUG forced command fail mask=0x%08lX", (unsigned long)channelMask);
        return false;
    }

    return true;
}

void Runtime::pollOnce()
{
    std::string err;
    std::vector<BYTE> data;

    if (!fetchProcessImage(data, err))
    {
        if (io_ && io_->isOpen())
            io_->purgeRx();

        Sleep(20);

        std::string err2;
        if (!fetchProcessImage(data, err2))
        {
            std::string finalErr = err2.empty() ? err : err2;

            if (isCpuResponseError(finalErr))
            {
                if (io_ && io_->isOpen())
                    io_->purgeRx();

                setCpuError("cpu no response: " + finalErr);
                return;
            }

            closeTransport();
            setStatusError("poll: " + finalErr);
            return;
        }
    }

    DWORD rawErrorMask = evaluateChannels(data);
    DWORD errorMask = applyPostCommandState(rawErrorMask);

    if (notify_)
        PostMessage(notify_, WM_FLEXI_STATUS, 0, 0);

    handleErrorMask(errorMask);

    DWORD commandMask = takeManualCommandMask();

    if (commandMask != 0)
    {
        if (!sendCommandForMask(commandMask, err))
        {
            EnterCriticalSection(&cs_);

            {
                DWORD now = GetTickCount();

                for (int i = 0; i < 4; i++)
                {
                    DWORD bit = (1u << i);

                    if ((commandMask & bit) == 0)
                        continue;

                    if (repeatWindowPauseStart_[i] != 0)
                    {
                        DWORD pausedMs = now - repeatWindowPauseStart_[i];

                        if (repeatFirstTick_[i] != 0)
                            repeatFirstTick_[i] += pausedMs;

                        repeatWindowPauseStart_[i] = 0;
                    }
                }
            }

            commandFailedMask_ = commandMask;
            LeaveCriticalSection(&cs_);

            gLog.log("Command FAILED mask=0x%08lX err=%s", (unsigned long)commandMask, err.c_str());

            if (notify_)
                PostMessage(notify_, WM_FLEXI_ALERT_SHOW_COMMAND_FAILED, (WPARAM)commandMask, 0);

            closeTransport();
            setStatusError("command: " + err);
            return;
        }

        // Command has been confirmed: ON store OK and OFF store OK.
        // Allow the same still-present fault to become a new alert/repeat event.
        EnterCriticalSection(&cs_);
        confirmedMask_ &= ~commandMask;
        abortedMask_ &= ~commandMask;
        awaitingPostCommandMask_ |= commandMask;

        DWORD now = GetTickCount();

        for (int i = 0; i < 4; i++)
        {
            DWORD bit = (1u << i);

            if ((commandMask & bit) == 0)
                continue;

            postCommandIgnoreUntil_[i] =
                now + cfg_.inputs[i].repeatFault.ignoreAfterCommandMs;
        }

        LeaveCriticalSection(&cs_);

        for (int i = 0; i < 4; i++)
        {
            DWORD bit = (1u << i);

            if ((commandMask & bit) == 0)
                continue;

            gLog.log(
                "Post-command ignore CH%d %lu ms",
                i + 1,
                (unsigned long)cfg_.inputs[i].repeatFault.ignoreAfterCommandMs
            );
        }

        if (notify_)
            PostMessage(notify_, WM_FLEXI_STATUS, 0, 0);
    }
}

void Runtime::pulseOutput(int idx)
{
    if (idx < 0 || idx >= 4)
        return;

    EnterCriticalSection(&cs_);

    {
        DWORD bit = (1u << idx);
        manualCommandMask_ |= bit;

        if (repeatWindowPauseStart_[idx] == 0)
            repeatWindowPauseStart_[idx] = GetTickCount();
    }

    LeaveCriticalSection(&cs_);

    gLog.log("Manual command queued CH%d", idx + 1);

    if (notify_)
        PostMessage(notify_, WM_FLEXI_STATUS, 0, 0);
}

void Runtime::alertYes(DWORD mask)
{
    if (mask == 0)
        return;

    EnterCriticalSection(&cs_);
    confirmedMask_ |= mask;
    abortedMask_ &= ~mask;
    manualCommandMask_ |= mask;
    activeAlertMask_ &= ~mask;
    activeRepeatMask_ &= ~mask;

    {
        DWORD now = GetTickCount();

        for (int i = 0; i < 4; i++)
        {
            DWORD bit = (1u << i);

            if ((mask & bit) == 0)
                continue;

            if (repeatWindowPauseStart_[i] == 0)
                repeatWindowPauseStart_[i] = now;
        }
    }

    LeaveCriticalSection(&cs_);

    gLog.log("Alert YES mask=0x%08lX", (unsigned long)mask);

    if (notify_)
        PostMessage(notify_, WM_FLEXI_STATUS, 0, 0);
}

void Runtime::alertNo(DWORD mask)
{
    if (mask == 0)
        return;

    EnterCriticalSection(&cs_);
    abortedMask_ |= mask;
    confirmedMask_ &= ~mask;
    activeAlertMask_ &= ~mask;
    activeRepeatMask_ &= ~mask;
    LeaveCriticalSection(&cs_);

    gLog.log("Alert NO mask=0x%08lX", (unsigned long)mask);

    if (notify_)
        PostMessage(notify_, WM_FLEXI_STATUS, 0, 0);
}

void Runtime::commandRetryYes(DWORD mask)
{
    if (mask == 0)
        return;

    EnterCriticalSection(&cs_);
    manualCommandMask_ |= mask;
    commandFailedMask_ &= ~mask;

    {
        DWORD now = GetTickCount();

        for (int i = 0; i < 4; i++)
        {
            DWORD bit = (1u << i);

            if ((mask & bit) == 0)
                continue;

            if (repeatWindowPauseStart_[i] == 0)
                repeatWindowPauseStart_[i] = now;
        }
    }

    LeaveCriticalSection(&cs_);

    gLog.log("Command retry YES mask=0x%08lX", (unsigned long)mask);

    if (notify_)
        PostMessage(notify_, WM_FLEXI_STATUS, 0, 0);
}

void Runtime::commandRetryNo(DWORD mask)
{
    if (mask == 0)
        return;

    EnterCriticalSection(&cs_);
    abortedMask_ |= mask;
    confirmedMask_ &= ~mask;
    commandFailedMask_ &= ~mask;
    LeaveCriticalSection(&cs_);

    gLog.log("Command retry NO mask=0x%08lX", (unsigned long)mask);

    if (notify_)
        PostMessage(notify_, WM_FLEXI_STATUS, 0, 0);
}
