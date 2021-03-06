//---------------------------------------------------------------------------

#ifndef SharedMemInfoH
#define SharedMemInfoH
//---------------------------------------------------------------------------
#include <VCL.h>

#pragma pack(push,1)
struct WOWHookViewInfo
{
    DWORD   HostProcessID;
    DWORD   DestProcessID;
    DWORD   HostPortNumber;
    DWORD   InjectPrepare;
	DWORD   Build;
	DWORD   BaseAddr;
	DWORD   BaseAddrOffset;
    DWORD   ForbiddenAnyMortConnection;
    int     ClientConnectIndex;
	TCHAR 	MainWindowClassName[50];
	int		IsHookHTTP;
	int		WatchPort;					//强制设定观测的端口<由11平台断线重连引入的>
	int		OnlyHookTCP;				//只hooktcp<由11平台断线重连引入的>
	int		ForceUseOneConnection;		//强制使用一个连接<由11平台断线重连引入的>
	int		GameType;					// 1. war3 2. war3自制
    void    Clear()
    {
        memset(this, 0, sizeof(*this));
    }
    bool    IsEmpty()
    {
        return HostProcessID == 0;
    }
};
#pragma pack(pop)

class MutexLock
{
    HANDLE  m_hMutex;
public:
    MutexLock(HANDLE handle)
    {
        m_hMutex = handle;
        WaitForSingleObject(m_hMutex, INFINITE);
    }
    ~MutexLock()
    {
        ReleaseMutex(m_hMutex);
    }
};

class SharedMemInfo
{
private:
    HANDLE          m_hFile;
    LPVOID          m_lpData;
	HANDLE          m_hMutex;
	bool			m_InitOK;
public:
    SharedMemInfo();
    ~SharedMemInfo();
    bool            CreateMapping();
    bool            OpenMapping();
    WOWHookViewInfo *   GetAt(int index);
    int             GetCount();
    WOWHookViewInfo *   FindSelf();
    WOWHookViewInfo *   CreateEmpty(int baseListenPort);
    WOWHookViewInfo *   GetTagMapping(int destProcessID);
	HANDLE              GetMutexHandle(){return m_hMutex;}
	bool			GetInitOK(){return	m_InitOK;}
};

SharedMemInfo   *   GetSharedMemInfo();
#endif
