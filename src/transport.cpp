#include "transport.h"
#include "serial.h"
#include "net_transport.h"

ITransport* TransportFactory::create(const AppConfig& cfg)
{
    if (cfg.transport == TRANSPORT_NETWORK)
        return new NetTransport();
    return new SerialPort();
}
