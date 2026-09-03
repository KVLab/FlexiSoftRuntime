#pragma once
#include "transport.h"
class SerialPort : public ITransport {
public:
    SerialPort(); ~SerialPort();
    bool open(const AppConfig& cfg, std::string& err);
    bool openSerial(const SerialConfig& cfg, std::string& err);
    void close(); bool isOpen() const;
    bool writeAll(const BYTE* data, DWORD len, std::string& err);
    bool readExact(BYTE* data, DWORD len, DWORD timeoutMs, std::string& err);
    void purgeRx();
    const char* name() const { return "serial"; }
private:
    HANDLE h_;
};
