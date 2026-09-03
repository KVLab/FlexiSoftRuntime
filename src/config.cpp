#include "config.h"
#include "json.h"
#include <algorithm>
#include <cctype>
#include <cstring>

static BYTE parseByteStr(const std::string& s)
{
    return (BYTE)strtol(s.c_str(), NULL, 0);
}

static void toLower(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
}

const char* ConfigTransportName(TransportKind t)
{
    switch (t)
    {
        case TRANSPORT_NETWORK:
        return "network";

        case TRANSPORT_SERIAL:

        default:
        return "serial";
    }
}

const char* ConfigNetModeName(NetMode m)
{
    switch (m)
    {
        case NET_UDP:
        return "udp";

        case NET_TCP_CLIENT:

        default:
        return "tcp_client";
    }
}

static bool startsWith(const std::string& s, const std::string& prefix)
{
    if (s.size() < prefix.size())
        return false;

    return s.compare(0, prefix.size(), prefix) == 0;
}

static LocalizedText readLocalizedText(
    const JVal & obj,
    const char* baseKey,
    const LocalizedText & current
)
{
    LocalizedText out = current;

    /*
        Backward compatibility:
            Old config used:
                "alert_text": "..."
                "text": "..."

            New config uses:
                "alert_text_en": "..."
                "alert_text_cz": "..."
                "text_en": "..."
                "text_cz": "..."
    */

    std::string legacy = JsonString(obj, baseKey, "");

    if (!legacy.empty())
    {
        out.fallback = legacy;
        out.values["en"] = legacy;
    }

    std::string prefix = std::string(baseKey) + "_";

    std::map<std::string, JVal>::const_iterator it;

    for (it = obj.o.begin(); it != obj.o.end(); ++it)
    {
        if (!startsWith(it->first, prefix))
            continue;

        if (it->second.t != JVal::STR)
            continue;

        std::string lang = it->first.substr(prefix.size());

        if (lang.empty())
            continue;

        toLower(lang);

        out.values[lang] = it->second.s;
    }

    return out;
}

std::string ConfigResolveLocalizedText(
    const LocalizedText& text,
    const std::string& language,
    const std::string& fallback
)
{
    std::string lang = language;
    toLower(lang);

    if (!lang.empty())
    {
        std::map<std::string, std::string>::const_iterator it = text.values.find(lang);

        if (it != text.values.end() && !it->second.empty())
            return it->second;
    }

    std::map<std::string, std::string>::const_iterator en = text.values.find("en");

    if (en != text.values.end() && !en->second.empty())
        return en->second;

    if (!text.fallback.empty())
        return text.fallback;

    return fallback;
}

void ApplyConfigLanguage(AppConfig & cfg, const std::string & language)
{
    std::string lang = language;

    if (lang.empty())
        lang = cfg.language;

    toLower(lang);

    cfg.trayTooltip = ConfigResolveLocalizedText(
        cfg.trayTooltipLocalized,
        lang,
        cfg.trayTooltip
    );

    for (int i = 0; i < 4; i++)
    {
        cfg.inputs[i].alertText = ConfigResolveLocalizedText(
            cfg.inputs[i].alertTextLocalized,
            lang,
            cfg.inputs[i].alertText
        );

        cfg.inputs[i].repeatFault.text = ConfigResolveLocalizedText(
            cfg.inputs[i].repeatFault.textLocalized,
            lang,
            cfg.inputs[i].repeatFault.text
        );
    }
}

std::vector<BYTE> ParseHexBytes(const std::string& s)
{
    std::vector<BYTE> v;
    const char* p = s.c_str();

    while (*p)
    {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
            p++;

        if (!*p)
            break;

        char* end = NULL;
        long x = strtol(p, &end, 16);

        if (end == p)
            break;

        v.push_back((BYTE)x);
        p = end;
    }

    return v;
}

static void defaults(AppConfig& cfg)
{
    cfg.transport = TRANSPORT_SERIAL;

    cfg.serial.port = "COM4";
    cfg.serial.baud = 115200;
    cfg.serial.dataBits = 8;
    cfg.serial.parity = 'N';
    cfg.serial.stopBits = 1;
    cfg.serial.timeoutMs = 2500;

    cfg.network.mode = NET_TCP_CLIENT;
    cfg.network.host = "192.168.0.7";
    cfg.network.port = 4001;
    cfg.network.bindHost = "0.0.0.0";
    cfg.network.bindPort = 0;
    cfg.network.connectTimeoutMs = 3000;
    cfg.network.timeoutMs = 1500;

    cfg.deviceLocal = 0x4F;
    cfg.deviceReply = 0x4D;
    cfg.token = ParseHexBytes("0F 0F 46 4C 58 54 30 31");

    cfg.pollPeriodMs = 1000;
    cfg.readBlock = 0x76;
    cfg.readSize = 54;

    cfg.language = "en";
    cfg.trayTooltip = "FlexiSoft Runtime";

    cfg.trayTooltipLocalized = LocalizedText();
    cfg.trayTooltipLocalized.fallback = cfg.trayTooltip;
    cfg.trayTooltipLocalized.values["en"] = cfg.trayTooltip;

    cfg.loggingEnabled = true;
    cfg.logFile = "flexi_runtime.log";
    cfg.loggingNewestFirst = true;
    cfg.loggingMaxBytes = 65536;

    cfg.debugForceCommandFail = false;

    for (int i = 0; i < 4; i++)
    {
        cfg.inputs[i].enabled = false;
        cfg.inputs[i].name = "CH" + std::to_string(i + 1);
        cfg.inputs[i].statusByte = 0;
        cfg.inputs[i].onBit = i * 2;
        cfg.inputs[i].okBit = i * 2 + 1;

        cfg.inputs[i].alertText = cfg.inputs[i].name + " reports an error";

        cfg.inputs[i].alertTextLocalized = LocalizedText();
        cfg.inputs[i].alertTextLocalized.fallback = cfg.inputs[i].alertText;
        cfg.inputs[i].alertTextLocalized.values["en"] = cfg.inputs[i].alertText;

        cfg.inputs[i].output.block = 0x42;
        cfg.inputs[i].output.byteIndex = 0;
        cfg.inputs[i].output.bit = i;
        cfg.inputs[i].output.pulseMs = 1000;

        cfg.inputs[i].repeatFault.count = 3;
        cfg.inputs[i].repeatFault.windowMs = 30000;
        cfg.inputs[i].repeatFault.ignoreAfterCommandMs = 1500;
        cfg.inputs[i].repeatFault.text = "Check proper closing of the guard";

        cfg.inputs[i].repeatFault.textLocalized = LocalizedText();
        cfg.inputs[i].repeatFault.textLocalized.fallback = cfg.inputs[i].repeatFault.text;
        cfg.inputs[i].repeatFault.textLocalized.values["en"] = cfg.inputs[i].repeatFault.text;
    }
}

bool LoadConfig(const char* path, AppConfig& cfg, std::string& err)
{
    defaults(cfg);
    err.clear();

    JVal root;

    if (!JsonParseFile(path, root, err))
    {
        if (err == "JSON file not found or empty")
        {
            err = "config.json not found or empty, using defaults";
            return true;
        }

        return false;
    }

    cfg.language = JsonString(root, "language", cfg.language);
    toLower(cfg.language);

    std::string tr = JsonString(root, "transport", "serial");
    toLower(tr);

    if (tr == "network" || tr == "tcp" || tr == "tcp_client")
        cfg.transport = TRANSPORT_NETWORK;
    else
        cfg.transport = TRANSPORT_SERIAL;

    if (const JVal* s = JsonGet(root, "serial"))
    {
        cfg.serial.port = JsonString(*s, "port", cfg.serial.port);
        cfg.serial.baud = JsonInt(*s, "baud", cfg.serial.baud);
        cfg.serial.dataBits = (BYTE)JsonInt(*s, "data_bits", cfg.serial.dataBits);

        std::string par = JsonString(*s, "parity", std::string(1, cfg.serial.parity));
        if (!par.empty())
            cfg.serial.parity = par[0];

        cfg.serial.stopBits = (BYTE)JsonInt(*s, "stop_bits", cfg.serial.stopBits);
        cfg.serial.timeoutMs = JsonInt(*s, "timeout_ms", cfg.serial.timeoutMs);
    }

    if (const JVal* n = JsonGet(root, "network"))
    {
        std::string m = JsonString(*n, "mode", "tcp_client");
        toLower(m);

        cfg.network.mode = (m == "udp") ? NET_UDP : NET_TCP_CLIENT;

        cfg.network.host = JsonString(*n, "host", cfg.network.host);
        cfg.network.port = (WORD)JsonInt(*n, "port", cfg.network.port);
        cfg.network.bindHost = JsonString(*n, "bind_host", cfg.network.bindHost);
        cfg.network.bindPort = (WORD)JsonInt(*n, "bind_port", cfg.network.bindPort);
        cfg.network.connectTimeoutMs = JsonInt(*n, "connect_timeout_ms", cfg.network.connectTimeoutMs);
        cfg.network.timeoutMs = JsonInt(*n, "timeout_ms", cfg.network.timeoutMs);
    }

    if (const JVal* r = JsonGet(root, "rk512"))
    {
        cfg.deviceLocal = parseByteStr(JsonString(*r, "device_local", "0x4F"));
        cfg.deviceReply = parseByteStr(JsonString(*r, "device_reply", "0x4D"));

        std::string th = JsonString(
            *r,
            "token_hex",
            JsonString(*r, "token", "0F 0F 46 4C 58 54 30 31")
        );

        cfg.token = ParseHexBytes(th);
    }

    if (const JVal* po = JsonGet(root, "poll"))
    {
        cfg.pollPeriodMs = JsonInt(*po, "period_ms", cfg.pollPeriodMs);
        cfg.readBlock = parseByteStr(JsonString(*po, "read_block", "0x76"));
        cfg.readSize = (WORD)JsonInt(*po, "read_size", cfg.readSize);
    }

    if (const JVal* ui = JsonGet(root, "ui"))
    {
        cfg.trayTooltipLocalized =
            readLocalizedText(*ui, "tray_tooltip", cfg.trayTooltipLocalized);
    }

    if (const JVal* lo = JsonGet(root, "logging"))
    {
        cfg.loggingEnabled = JsonBool(*lo, "enabled", cfg.loggingEnabled);
        cfg.logFile = JsonString(*lo, "file", cfg.logFile);
        cfg.loggingNewestFirst = JsonBool(*lo, "newest_first", cfg.loggingNewestFirst);
        cfg.loggingMaxBytes = (DWORD)JsonInt(*lo, "max_bytes", (int)cfg.loggingMaxBytes);

        if (cfg.loggingMaxBytes < 4096)
            cfg.loggingMaxBytes = 4096;
    }

    if (const JVal* dbg = JsonGet(root, "debug"))
    {
        cfg.debugForceCommandFail = JsonBool(*dbg, "force_command_fail", cfg.debugForceCommandFail);
    }

    if (const JVal* arr = JsonGet(root, "inputs"))
    {
        if (arr->t == JVal::ARR)
        {
            for (size_t i = 0; i < arr->a.size() && i < 4; i++)
            {
                const JVal& x = arr->a[i];

                cfg.inputs[i].enabled = JsonBool(x, "enabled", cfg.inputs[i].enabled);
                cfg.inputs[i].name = JsonString(x, "name", cfg.inputs[i].name);
                cfg.inputs[i].statusByte = JsonInt(x, "status_byte", cfg.inputs[i].statusByte);
                cfg.inputs[i].onBit = JsonInt(x, "on_bit", cfg.inputs[i].onBit);
                cfg.inputs[i].okBit = JsonInt(x, "ok_bit", cfg.inputs[i].okBit);

                cfg.inputs[i].alertTextLocalized =
                    readLocalizedText(x, "alert_text", cfg.inputs[i].alertTextLocalized);

                if (const JVal* o = JsonGet(x, "output"))
                {
                    cfg.inputs[i].output.block =
                        parseByteStr(JsonString(*o, "block", "0x42"));

                    cfg.inputs[i].output.byteIndex =
                        JsonInt(*o, "byte", cfg.inputs[i].output.byteIndex);

                    cfg.inputs[i].output.bit =
                        JsonInt(*o, "bit", cfg.inputs[i].output.bit);

                    cfg.inputs[i].output.pulseMs =
                        JsonInt(*o, "pulse_ms", cfg.inputs[i].output.pulseMs);
                }

                if (const JVal* rf = JsonGet(x, "repeat_fault"))
                {
                    cfg.inputs[i].repeatFault.count =
                        JsonInt(*rf, "count", cfg.inputs[i].repeatFault.count);

                    cfg.inputs[i].repeatFault.windowMs =
                        (DWORD)JsonInt(*rf, "window_ms", (int)cfg.inputs[i].repeatFault.windowMs);

                    int ignoreAfterCommandMs =
                        JsonInt(*rf, "ignore_after_command_ms", (int)cfg.inputs[i].repeatFault.ignoreAfterCommandMs);

                    if (ignoreAfterCommandMs < 0)
                        ignoreAfterCommandMs = 0;

                    cfg.inputs[i].repeatFault.ignoreAfterCommandMs =
                        (DWORD)ignoreAfterCommandMs;

                    cfg.inputs[i].repeatFault.textLocalized =
                        readLocalizedText(*rf, "text", cfg.inputs[i].repeatFault.textLocalized);

                    if (cfg.inputs[i].repeatFault.count < 1)
                        cfg.inputs[i].repeatFault.count = 1;

                    if (cfg.inputs[i].repeatFault.windowMs < 1000)
                        cfg.inputs[i].repeatFault.windowMs = 1000;
                }
            }
        }
    }

    ApplyConfigLanguage(cfg, cfg.language);

    return true;
}
