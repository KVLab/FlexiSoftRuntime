#pragma once
#include "transport.h"

class NetTransport : public ITransport {
public:
    NetTransport();
    ~NetTransport();
    bool open(const AppConfig& cfg, std::string& err);
    void close();
    bool isOpen() const;
    bool writeAll(const BYTE* data, DWORD len, std::string& err);
    bool readExact(BYTE* data, DWORD len, DWORD timeoutMs, std::string& err);
    void purgeRx();
    const char* name() const { return mode_ == NET_UDP ? "udp" : "tcp_client"; }
private:
    SOCKET s_;
    NetMode mode_;
    sockaddr_in remote_;
    bool udpPeerKnown_;
    bool startupDone_;
    bool openTcpClient(const NetworkConfig& cfg, std::string& err);
    bool openUdp(const NetworkConfig& cfg, std::string& err);
};
