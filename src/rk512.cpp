#include "rk512.h"

RK512::RK512(ITransport& io, BYTE local, BYTE reply, DWORD timeoutMs)
    : io_(io), local_(local), reply_(reply), timeoutMs_(timeoutMs)
{
}

WORD RK512::crcCcitt(const BYTE* data, size_t len)
{
    WORD crc = 0xFFFF;

    for (size_t i = 0; i < len; i++)
    {
        crc ^= (WORD)data[i] << 8;

        for (int b = 0; b < 8; b++)
        {
            if (crc & 0x8000)
                crc = (WORD)((crc << 1) ^ 0x1021);
            else
                crc = (WORD)(crc << 1);
        }
    }

    return crc;
}

bool RK512::fetch(BYTE block, WORD size, std::vector<BYTE>& data, std::string& err)
{
    BYTE tx[10] = {
        0x00, 0x00,
        0x45, 0x44,
        block,
        0x00,
        (BYTE)(size >> 8),
        (BYTE)(size & 0xFF),
        0xFF,
        local_
    };

    if (!io_.writeAll(tx, 10, err))
        return false;

    BYTE hdr[10];

    if (!io_.readExact(hdr, 10, timeoutMs_, err))
        return false;

    if (hdr[0] != 0x00 || hdr[1] != 0x00 || hdr[2] != 0x00 || hdr[3] != 0x00)
    {
        char msg[128];

        _snprintf(
            msg,
            sizeof(msg) - 1,
            "bad fetch status RX=%02X %02X %02X %02X",
            hdr[0],
            hdr[1],
            hdr[2],
            hdr[3]
        );

        msg[sizeof(msg) - 1] = 0;

        err = msg;
        return false;
    }

    if (hdr[4] != block || hdr[9] != reply_)
    {
        char msg[160];

        _snprintf(
            msg,
            sizeof(msg) - 1,
            "bad fetch header block=%02X reply=%02X size=%02X%02X",
            hdr[4],
            hdr[9],
            hdr[6],
            hdr[7]
        );

        msg[sizeof(msg) - 1] = 0;

        err = msg;
        return false;
    }

    WORD rxSizeField = ((WORD)hdr[6] << 8) | hdr[7];

    if (rxSizeField == 0)
        rxSizeField = size;

    /*
       Flexi Soft RK512 read size field is not payload bytes.

       Confirmed examples:
         0x42: size field 0x0006 -> payload 4 bytes
         0x76: size field 0x0036 -> payload 100 bytes
         0x7E: size field 0x0022 -> payload 60 bytes
         0x7C: size field 0x00AC -> payload 336 bytes

       Formula: payload_bytes = size_field * 2 - 8
    */
    DWORD payloadBytes = 0;

    if (rxSizeField >= 4)
        payloadBytes = ((DWORD)rxSizeField * 2u) - 8u;

    data.assign(payloadBytes, 0);

    if (payloadBytes > 0)
    {
        if (!io_.readExact(&data[0], payloadBytes, timeoutMs_, err))
            return false;
    }

    return true;
}

bool RK512::store(BYTE block, const std::vector<BYTE>& data, std::string& err)
{
    /*
       Store size field is block-dependent on Flexi Soft visualization blocks.

       Confirmed:
         token block 0x41: data 8 bytes -> size field 8
         write block 0x42: data 4 bytes -> size field 6

       For now this runtime only writes these two blocks.
    */
    WORD size = (WORD)data.size();

    if (block == 0x42 && data.size() == 4)
        size = 6;

    std::vector<BYTE> tx;

    BYTE hdr[10] = {
        0x00, 0x00,
        0x41, 0x44,
        block,
        0x00,
        (BYTE)(size >> 8),
        (BYTE)(size & 0xFF),
        0xFF,
        local_
    };

    tx.insert(tx.end(), hdr, hdr + 10);
    tx.insert(tx.end(), hdr + 4, hdr + 10);
    tx.insert(tx.end(), data.begin(), data.end());

    WORD crc = crcCcitt(&tx[10], tx.size() - 10);

    tx.push_back((BYTE)(crc & 0xFF));
    tx.push_back((BYTE)(crc >> 8));

    if (!io_.writeAll(&tx[0], (DWORD)tx.size(), err))
        return false;

    BYTE rx[4];

    if (!io_.readExact(rx, 4, timeoutMs_, err))
        return false;

    if (rx[0] || rx[1] || rx[2] || rx[3])
    {
        char msg[128];

        _snprintf(
            msg,
            sizeof(msg) - 1,
            "store rejected RX=%02X %02X %02X %02X",
            rx[0],
            rx[1],
            rx[2],
            rx[3]
        );

        msg[sizeof(msg) - 1] = 0;

        err = msg;
        return false;
    }

    return true;
}

bool RK512::acquireToken(BYTE block, const std::vector<BYTE>& token, std::string& err)
{
    return store(block, token, err);
}
