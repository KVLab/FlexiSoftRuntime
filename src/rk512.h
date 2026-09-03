#pragma once
#include "common.h"
#include "transport.h"
class RK512 {
public:
    RK512(ITransport& io, BYTE local, BYTE reply, DWORD timeoutMs);
    bool fetch(BYTE block, WORD sizeWordsLikeFlexi, std::vector<BYTE>& data, std::string& err);
    bool store(BYTE block, const std::vector<BYTE>& data, std::string& err);
    bool acquireToken(BYTE block, const std::vector<BYTE>& token, std::string& err);
private:
    ITransport& io_; BYTE local_; BYTE reply_; DWORD timeoutMs_;
    static WORD crcCcitt(const BYTE* data, size_t len);
};
