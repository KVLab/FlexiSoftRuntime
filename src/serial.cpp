#include "serial.h"
SerialPort::SerialPort():h_(INVALID_HANDLE_VALUE){}
SerialPort::~SerialPort(){
	close();
}

bool SerialPort::isOpen() const {
	return h_!=INVALID_HANDLE_VALUE;
}

void SerialPort::close(){
	if(isOpen()){
		CloseHandle(h_);
		h_=INVALID_HANDLE_VALUE;
	}
}

bool SerialPort::open(const AppConfig& cfg, std::string& err){
	return openSerial(cfg.serial, err);
}

bool SerialPort::openSerial(const SerialConfig& cfg, std::string& err)
{
    close();

    std::string name = "\\\\.\\" + cfg.port;
    std::wstring wname = s2ws(name);

    h_ = CreateFileW(
        wname.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (h_ == INVALID_HANDLE_VALUE)
    {
        DWORD e = GetLastError();

        char msg[256];

        _snprintf(
            msg,
            sizeof(msg) - 1,
            "COM open failed port=%s winerr=%lu",
            name.c_str(),
            (unsigned long)e
        );

        msg[sizeof(msg) - 1] = 0;

        err = msg;
        h_ = INVALID_HANDLE_VALUE;

        return false;
    }

    SetupComm(h_, 4096, 4096);
    PurgeComm(h_, PURGE_RXCLEAR | PURGE_TXCLEAR);

    DCB dcb;
    ZeroMemory(&dcb, sizeof(dcb));
    dcb.DCBlength = sizeof(dcb);

    if (!GetCommState(h_, &dcb))
    {
        err = "GetCommState failed";
        close();

        return false;
    }

    dcb.BaudRate = cfg.baud;
    dcb.ByteSize = cfg.dataBits;

    if (cfg.parity == 'E' || cfg.parity == 'e')
        dcb.Parity = EVENPARITY;
    else if (cfg.parity == 'O' || cfg.parity == 'o')
        dcb.Parity = ODDPARITY;
    else
        dcb.Parity = NOPARITY;

    dcb.StopBits = (cfg.stopBits == 2) ? TWOSTOPBITS : ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fParity = (dcb.Parity != NOPARITY);

    if (!SetCommState(h_, &dcb))
    {
        err = "SetCommState failed";
        close();

        return false;
    }

    COMMTIMEOUTS t;
    ZeroMemory(&t, sizeof(t));

    t.ReadIntervalTimeout = 20;
    t.ReadTotalTimeoutConstant = cfg.timeoutMs;
    t.ReadTotalTimeoutMultiplier = 2;
    t.WriteTotalTimeoutConstant = cfg.timeoutMs;
    t.WriteTotalTimeoutMultiplier = 2;

    SetCommTimeouts(h_, &t);

    return true;
}

bool SerialPort::writeAll(const BYTE* data,DWORD len,std::string& err){
	DWORD done=0;
	if(!WriteFile(h_,data,len,&done,NULL)||done!=len){
		err="serial write failed";
		return false;
	}
	return true;
}

bool SerialPort::readExact(BYTE* data,DWORD len,DWORD timeoutMs,std::string& err){
	DWORD gotTotal=0;
	DWORD start=GetTickCount();
	while(gotTotal<len){
		DWORD got=0;
		if(!ReadFile(h_,data+gotTotal,len-gotTotal,&got,NULL)){
			err="serial read failed";
			return false;
		}
		gotTotal+=got;
		if(gotTotal>=len) return true;
		if(GetTickCount()-start>timeoutMs){
			err="serial timeout";
			return false;
		}
		Sleep(5);
	}
	return true;
}

void SerialPort::purgeRx(){
    if(isOpen())
        PurgeComm(h_, PURGE_RXCLEAR);
}
