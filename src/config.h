#pragma once
#include "common.h"
#include <map>

enum TransportKind
{
    TRANSPORT_SERIAL,
    TRANSPORT_NETWORK
};

enum NetMode
{
    NET_TCP_CLIENT,
    NET_UDP
};

struct SerialConfig
{
    std::string port;
    DWORD baud;
    BYTE dataBits;
    char parity;
    BYTE stopBits;
    DWORD timeoutMs;
};

struct NetworkConfig
{
    NetMode mode;
    std::string host;
    WORD port;
    std::string bindHost;
    WORD bindPort;
    DWORD connectTimeoutMs;
    DWORD timeoutMs;
};

struct OutputConfig
{
    BYTE block;
    int byteIndex;
    int bit;
    DWORD pulseMs;
};

struct LocalizedText
{
    std::string fallback;
    std::map<std::string, std::string> values;
};

struct RepeatFaultConfig
{
    int count;
    DWORD windowMs;
    DWORD ignoreAfterCommandMs;
    std::string text;
    LocalizedText textLocalized;
};

struct InputConfig
{
    bool enabled;
    std::string name;
    int statusByte;
    int onBit;
    int okBit;
    std::string alertText;
    LocalizedText alertTextLocalized;
    OutputConfig output;
    RepeatFaultConfig repeatFault;
};

struct AppConfig
{
    TransportKind transport;
    SerialConfig serial;
    NetworkConfig network;

    BYTE deviceLocal;
    BYTE deviceReply;
    std::vector<BYTE> token;

    DWORD pollPeriodMs;
    BYTE readBlock;
    WORD readSize;

    InputConfig inputs[4];

    std::string language;
    std::string trayTooltip;
    LocalizedText trayTooltipLocalized;

    bool loggingEnabled;
    std::string logFile;
    bool loggingNewestFirst;
    DWORD loggingMaxBytes;

    bool debugForceCommandFail;
};

bool LoadConfig(const char* path, AppConfig& cfg, std::string& err);

const char* ConfigTransportName(TransportKind t);
const char* ConfigNetModeName(NetMode m);

std::string ConfigResolveLocalizedText(
    const LocalizedText & text,
    const std::string & language,
    const std::string & fallback
);

void ApplyConfigLanguage(AppConfig & cfg, const std::string & language);

std::vector<BYTE> ParseHexBytes(const std::string& s);
