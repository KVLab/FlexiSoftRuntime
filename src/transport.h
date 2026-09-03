#pragma once
#include "common.h"
#include "config.h"

class ITransport {
public:
    virtual ~ITransport() {}
    virtual bool open(const AppConfig& cfg, std::string& err) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual bool writeAll(const BYTE* data, DWORD len, std::string& err) = 0;
    virtual bool readExact(BYTE* data, DWORD len, DWORD timeoutMs, std::string& err) = 0;
    virtual void purgeRx() = 0;
    virtual const char* name() const = 0;
};

class TransportFactory {
public:
    static ITransport* create(const AppConfig& cfg);
};
