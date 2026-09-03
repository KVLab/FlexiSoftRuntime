#include "net_transport.h"
#include "logger.h"

static std::string wsaErr(const char* prefix)
{
    char b[128];

    _snprintf(
        b,
        sizeof(b) - 1,
        "%s WSA=%d",
        prefix ? prefix : "winsock",
        WSAGetLastError()
    );

    b[sizeof(b) - 1] = 0;

    return b;
}

NetTransport::NetTransport():s_(INVALID_SOCKET),mode_(NET_TCP_CLIENT),udpPeerKnown_(false),startupDone_(false)
{
    ZeroMemory(&remote_,sizeof(remote_));
}

NetTransport::~NetTransport()
{
    close();
    if(startupDone_) WSACleanup();
}

bool NetTransport::open(const AppConfig& cfg, std::string& err)
{
    close();
    mode_=cfg.network.mode;
    WSADATA wd;

    if(!startupDone_)
    {
        if(WSAStartup(MAKEWORD(2,2),&wd)!=0)
        {
            err=wsaErr("WSAStartup");
            return false;
        }
        startupDone_=true;
    }

    if(mode_==NET_UDP)
        return openUdp(cfg.network,err);

    return openTcpClient(cfg.network,err);
}

bool NetTransport::openTcpClient(const NetworkConfig& cfg, std::string& err)
{
    s_=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

    if(s_==INVALID_SOCKET)
    {
        err=wsaErr("socket");
        return false;
    }

    sockaddr_in a;
    ZeroMemory(&a,sizeof(a));
    a.sin_family=AF_INET;
    a.sin_port=htons(cfg.port);
    a.sin_addr.s_addr=inet_addr(cfg.host.c_str());

    if(a.sin_addr.s_addr==INADDR_NONE)
    {
        err="Only numeric IPv4 host is supported on XP-safe build";
        close();
        return false;
    }

    u_long nb=1;
    ioctlsocket(s_,FIONBIO,&nb);
    int r=connect(s_,(sockaddr*)&a,sizeof(a));

    if(r==SOCKET_ERROR && WSAGetLastError()!=WSAEWOULDBLOCK)
    {
        err=wsaErr("connect");
        close();
        return false;
    }

    fd_set wf;
    FD_ZERO(&wf);
    FD_SET(s_,&wf);

    timeval tv;
    tv.tv_sec=cfg.connectTimeoutMs/1000;
    tv.tv_usec=(cfg.connectTimeoutMs%1000)*1000;

    r=select(0,NULL,&wf,NULL,&tv);

    if(r<=0)
    {
        err="tcp connect timeout";
        close();
        return false;
    }

    int soerr=0;
    int slen=sizeof(soerr);
    getsockopt(s_,SOL_SOCKET,SO_ERROR,(char*)&soerr,&slen);

    if(soerr!=0)
    {
        WSASetLastError(soerr);
        err=wsaErr("tcp connect");
        close();
        return false;
    }

    nb=0;
    ioctlsocket(s_,FIONBIO,&nb);
    remote_=a;
    return true;
}

bool NetTransport::openUdp(const NetworkConfig& cfg, std::string& err)
{
    s_=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);

    if(s_==INVALID_SOCKET)
    {
        err=wsaErr("udp socket");
        return false;
    }

    if(cfg.bindPort)
    {
        sockaddr_in b;
        ZeroMemory(&b,sizeof(b));
        b.sin_family=AF_INET;
        b.sin_port=htons(cfg.bindPort);
        b.sin_addr.s_addr=inet_addr(cfg.bindHost.c_str());

        if(b.sin_addr.s_addr==INADDR_NONE)
            b.sin_addr.s_addr=INADDR_ANY;

        if(bind(s_,(sockaddr*)&b,sizeof(b))==SOCKET_ERROR)
        {
            err=wsaErr("udp bind");
            close();
            return false;
        }
    }

    ZeroMemory(&remote_,sizeof(remote_));
    remote_.sin_family=AF_INET;
    remote_.sin_port=htons(cfg.port);
    remote_.sin_addr.s_addr=inet_addr(cfg.host.c_str());

    if(remote_.sin_addr.s_addr==INADDR_NONE)
    {
        err="Only numeric IPv4 host is supported for UDP";
        close();
        return false;
    }

    udpPeerKnown_=true;
    return true;
}

void NetTransport::close()
{
    if(s_!=INVALID_SOCKET)
    {
        closesocket(s_);
        s_=INVALID_SOCKET;
    }
    udpPeerKnown_=false;
}

bool NetTransport::isOpen() const
{
    return s_!=INVALID_SOCKET;
}

bool NetTransport::writeAll(const BYTE* data, DWORD len, std::string& err)
{
    if(s_==INVALID_SOCKET)
    {
        err="network socket closed";
        return false;
    }

    if(mode_==NET_UDP)
    {
        int r=sendto(s_,(const char*)data,len,0,(sockaddr*)&remote_,sizeof(remote_));

        if(r==SOCKET_ERROR || (DWORD)r!=len)
        {
            err=wsaErr("udp sendto");
            return false;
        }
        return true;
    }

    DWORD done=0;
    while(done<len)
    {
        int r=send(s_,(const char*)data+done,(int)(len-done),0);

        if(r<=0)
        {
            err=wsaErr("tcp send");
            return false;
        }
        done+=r;
    }

    return true;
}

bool NetTransport::readExact(BYTE* data, DWORD len, DWORD timeoutMs, std::string& err)
{
    if(s_==INVALID_SOCKET)
    {
        err="network socket closed";
        return false;
    }

    DWORD done=0;
    DWORD start=GetTickCount();

    while(done<len)
    {
        DWORD now=GetTickCount();
        DWORD elapsed=now-start;

        if(elapsed>=timeoutMs)
        {
            err="network read timeout";
            return false;
        }

        fd_set rf;
        FD_ZERO(&rf);
        FD_SET(s_,&rf);
        DWORD left=timeoutMs-elapsed;

        timeval tv;
        tv.tv_sec=left/1000;
        tv.tv_usec=(left%1000)*1000;

        int sr=select(0,&rf,NULL,NULL,&tv);

        if(sr<=0)
        {
            err=(sr==0)?"network read timeout":wsaErr("select");
            return false;
        }

        if(mode_==NET_UDP)
        {
            sockaddr_in from;
            int flen=sizeof(from);
            int r=recvfrom(s_,(char*)data+done,(int)(len-done),0,(sockaddr*)&from,&flen);

            if(r<=0)
            {
                err=wsaErr("udp recvfrom");
                return false;
            }
            done+=r;
        }

        else
        {
            int r=recv(s_,(char*)data+done,(int)(len-done),0);

            if(r<=0)
            {
                err=wsaErr("tcp recv");
                return false;
            }
            done+=r;
        }
    }

    return true;
}

void NetTransport::purgeRx()
{
    if(s_==INVALID_SOCKET)
        return;

    u_long nb=1;
    ioctlsocket(s_, FIONBIO, &nb);

    char buf[256];

    for(;;)
    {
        int r;
        if(mode_==NET_UDP)
        {
            sockaddr_in from;
            int flen=sizeof(from);
            r=recvfrom(s_, buf, sizeof(buf), 0, (sockaddr*)&from, &flen);
        }

        else
        {
            r=recv(s_, buf, sizeof(buf), 0);
        }

        if(r<=0)
            break;
    }

    nb=0;
    ioctlsocket(s_, FIONBIO, &nb);
}
