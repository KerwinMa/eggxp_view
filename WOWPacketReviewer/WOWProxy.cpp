//---------------------------------------------------------------------------


#pragma hdrstop

#include "WOWProxy.h"
#include "Log4Me.h"
#include "Opcodes.h"
#include "ReviewerCommon.h"
//---------------------------------------------------------------------------

#pragma package(smart_init)

#define     MAX_BUF_SIZE    1024

#define     WATCH_PORT      8085

static      int     gPackIndex = 0;
static      WOWProxyPool     gWOWProxyPool;

WOWProxyPool     *       GetWOWProxyPool()
{
    return          &gWOWProxyPool;
}

WOWProxyManager  *       GetWOWProxyManager()
{
    if(GetWOWProxyPool()->GetProxyCount() == 0)
    {
        GetWOWProxyPool()->AddProxy();
    }

    return  GetWOWProxyPool()->GetProxy(0);
}

WOWProxy::WOWProxy()
{
	InitializeCriticalSection(&m_csLock);
	m_DesPort = 0;
	m_Active = 0;
	m_StopProxy = 0;
    m_IsDesabled = 0;
    m_RealmIndex = 0;
	fpOnUserAuthPacket = NULL;
    m_ProxyType = PROXY_TYPE_REALM;
}

WOWProxy::~WOWProxy()
{
	DeleteCriticalSection(&m_csLock);
}

bool            WOWProxy::SendBuf_O(SOCKET  s, char * buf, int len)
{
    int result = 0;
    while(len != 0)
    {
        result = send(s, buf, len, 0);
        if(result == SOCKET_ERROR)
        {
            return  false;
        }

        len = len - result;
        buf = buf + result;

		if(len == 0)
		{
			return true;
		}
//		GetLog()->Warn("resend");
    }

    return  true;
}

int             WOWProxy::RecvProxy(SOCKET  from)
{
	char        recvBuf[MAX_BUF_SIZE] = {'\0'};
	int         recvLen = 0;

    recvLen = recv(from, recvBuf, sizeof(recvBuf), 0);

	if(recvLen <= 0)
	{
        GetLog()->Warn("WOWProxy::RecvProxy socket error. m_DesIP = %s, msg = %s", m_DesIP, SysErrorMessage(GetLastError()));
        Close();
        return  -1;
    }

	AnsiString pack = AnsiString(recvBuf, recvLen);

//    //--------------------------------------------------------------------------
//    GetLog()->Warn("m_DesIP = %s, m_DesPort = %d, m_ProxyType = %d", m_DesIP, m_DesPort, m_ProxyType);
//    if(from == m_HostSocket)
//    {
//        GetLog()->Warn("First Recv From Server:(%d):%s", pack.Length(), BinToStr(pack.c_str(), pack.Length()));
//    }
//    else
//    {
//        GetLog()->Warn("First Recv From Client:(%d):%s", pack.Length(), BinToStr(pack.c_str(), pack.Length()));
//    }
//    //--------------------------------------------------------------------------

	shared_ptr<WOWPackage> allReq(new WOWPackage(pack, gPackIndex));
	allReq->SetPacketProxyType(this->GetProxyType());
	if(from == m_HostSocket)
	{
		allReq->SetMark(RECV_MARK);
	}
	else
	{
//		GetLog()->Info("DirectModel:%s", BinToStr(recvBuf, recvLen));
		allReq->SetMark(SEND_MARK);
	}

	//------------------------------------------------------------
	if(GetWOWProxyManager()->GetDirectModel())
	{
		allReq->SetOrgPrefixData(allReq->GetOrgData());
		if(from == m_HostSocket)
		{
			this->GetServerToClientQueue()->Push(allReq);
		}
		else
		{
//			GetLog()->Info("DirectModel:%s", BinToStr(recvBuf, recvLen));
			this->GetClientToServerQueue()->Push(allReq);
		}
	}
	//------------------------------------------------------------

    allReq->SetDestIP(m_DesIP);
    allReq->SetDestPort(m_DesPort);
    if(m_ProxyType == PROXY_TYPE_REALM)
    {
        allReq->SetPacketProxyIndex(m_RealmIndex);
    }
    else
    {
        allReq->SetPacketProxyIndex(m_DestIndex);
    }
	GetWOWProxyManager()->GetAllQueue()->Push(allReq);


	{
		ThreadLock lock(&m_csLock);
		if(fpOnUserAuthPacket)
		{
			GetLog()->Warn("Call First Packet. ThreaID = %d", GetCurrentThreadId());
			fpOnUserAuthPacket(this, allReq.get());
			fpOnUserAuthPacket = NULL;
		}
	}

    gPackIndex++;
    return          0;
}

int             WOWProxy::SendProxy(SOCKET  to, ASharedPtrQueue<WOWPackage>  *pool)
{
	if(m_StopProxy)
		return -1;
	shared_ptr<WOWPackage> curPack;
	if(!pool->Pop(&curPack))
	{
		return  2;
	}

	AnsiString pack = curPack->GetOrgPrefixData();

//    if(m_ProxyType != PROXY_TYPE_REALM)
//    {
//        GetLog()->Warn("m_DesIP = %s, m_DesPort = %d, m_DestIndex = %d", m_DesIP, m_DesPort, m_DestIndex);
//        if(pool == &m_ClientToServerQueue)
//        {
//            GetLog()->Warn("Finial Send To Server:(%d):%s", pack.Length(), BinToStr(pack.c_str(), pack.Length()));
//        }
//        else
//        {
//            GetLog()->Warn("Finial Send To Client:(%d):%s", pack.Length(), BinToStr(pack.c_str(), pack.Length()));
//        }
//    }

	pack = pack.Unique();
//	GetLog()->Warn("Delete Pack:%s, p = %p", curPack->GetOpCodeMsg(), curPack);
    if(!SendBuf_O(to, pack.c_str(), pack.Length()))
    {
        GetLog()->Warn("WOWProxy::HostRecvThread socket send error");
        Close();
        return  -1;
    }


    return      0;
}

int             WOWProxy::HostRecvThread(SingleThread * self)
{
	ASharedPtrQueue<WOWPackage>  * queue = &m_ServerToClientQueue;
	return      RecvProxy(m_HostSocket);
}

int             WOWProxy::ClientRecvThread(SingleThread * self)
{
	ASharedPtrQueue<WOWPackage>  * queue = &m_ClientToServerQueue;
    return      RecvProxy(m_ClientSocket);
}

int             WOWProxy::HostSendThread(SingleThread * self)
{
    return      SendProxy(m_HostSocket, &m_ClientToServerQueue);
}

int             WOWProxy::ClientSendThread(SingleThread * self)
{
    return      SendProxy(m_ClientSocket, &m_ServerToClientQueue);
}

int				WOWProxy::ThreadInitFunc(SingleThread * self)
{
	ThreadLock lock(&m_csLock);
	m_Active = 1;
	m_ThreadState[GetCurrentThreadId()] = 1;
//	GetLog()->Warn("Init Thread ID = %d", GetCurrentThreadId());
	return 0;
}

int				WOWProxy::ThreadUnInitFunc(SingleThread * self)
{
	ThreadLock lock(&m_csLock);
	m_ThreadState.erase(GetCurrentThreadId());
	return 0;
}

bool            WOWProxy::Start(SOCKET client, SOCKADDR_IN clientAddr, String ip, int port)
{
    m_DesIP = ip;
    m_DesPort = port;
    m_ClientSocket = client;
    m_ClientAddr = clientAddr;

    m_HostSocket = socket(AF_INET,SOCK_STREAM,0);


    sockaddr_in their_addr; /* connector's address information */
	their_addr.sin_family = AF_INET; /* host byte order */
	their_addr.sin_port = htons(m_DesPort); /* Զ�������˿� */
	AnsiString ansiIP = m_DesIP;
	their_addr.sin_addr.s_addr=inet_addr(ansiIP.c_str()); /* Զ������ip��ַ */

 
	/* ����connect������������������� */
    GetLog()->Warn("WOWProxy::Start, connect ip=%s, port=%d, m_ProxyType = %d, m_DestIndex = %d", m_DesIP, m_DesPort, m_ProxyType, m_DestIndex);
	if (connect(m_HostSocket, (const sockaddr *)&their_addr, sizeof(their_addr)) == -1)
	{
		GetLog()->Warn("WOWProxy::Start, Error");
        return false;
	}

	GetLog()->Warn("Starting InitProxy Threads");
	UserThread *curThread = GetThreadManager()->ManagerCreateThread("ClientRecvThread", ClientRecvThread, false);
	curThread->GetThread()->fpInitFunc = ThreadInitFunc;
	curThread->GetThread()->fpUnInitFunc = ThreadUnInitFunc;
	#if defined(WOW_FISHER) && !defined(_DEBUG)
	if(!fpOnUserAuthPacket)
	{
		ServerAuthOKBeginProxy();
	}
	#else
	ServerAuthOKBeginProxy();
	#endif
	GetLog()->Warn("Finish Starting InitProxy Threads");
	return true;
}

void			WOWProxy::ServerAuthOKBeginProxy()
{
	GetLog()->Warn("Starting WorkerProxy Threads");
	UserThread *curThread = GetThreadManager()->ManagerCreateThread("ClientSendThread", ClientSendThread, false);
	curThread->GetThread()->fpInitFunc = ThreadInitFunc;
	curThread->GetThread()->fpUnInitFunc = ThreadUnInitFunc;
	curThread = GetThreadManager()->ManagerCreateThread("HostRecvThread", HostRecvThread, false);
	curThread->GetThread()->fpInitFunc = ThreadInitFunc;
	curThread->GetThread()->fpUnInitFunc = ThreadUnInitFunc;
	curThread = GetThreadManager()->ManagerCreateThread("HostSendThread", HostSendThread, false);
	curThread->GetThread()->fpInitFunc = ThreadInitFunc;
	curThread->GetThread()->fpUnInitFunc = ThreadUnInitFunc;
	GetLog()->Warn("Finish Starting WorkerProxy Threads");
}

void            WOWProxy::Close()
{
	if(m_StopProxy == 0)
	{
		GetLog()->Warn("WOWProxy::Close() m_DesIP = %s, m_DesPort = %d, m_DestIndex = %d", m_DesIP, m_DesPort, m_DestIndex);
	}
	closesocket(m_ClientSocket);
	closesocket(m_HostSocket);
	m_StopProxy = 1;
}

int             WOWProxy::GetClosed()
{
	if(m_Active && m_ThreadState.size() == 0)
		return 1;

	return 0;
}

/////////////////////////////////////////////////////////////////////////////

WOWProxyManager::WOWProxyManager()
{
    m_ListenSocket = 0;
    m_ListenPort = 0;
	fpOnUserAuthPacket = NULL;
	m_RealmdPort = 0;
	m_DirectModel = false;
    m_ProxyType = PROXY_TYPE_REALM;
}

WOWProxyManager::~WOWProxyManager()
{
    WSACleanup();//�ͷ���Դ�Ĳ���
}

void		   WOWProxyManager::ProcessRemoveProxyList()
{
	for(int i=m_WOWProxys.Count()-1; i>=0; i--)
	{
		if(m_WOWProxys[i]->GetClosed() == 1)
		{
			m_WOWProxys.Delete(i);
		}
	}
}

int            WOWProxyManager::ListenThread(SingleThread * self)
{
	//////////////////////////////////////////////////////////////////////////
    //��ʼ���м���
    //////////////////////////////////////////////////////////////////////////
    SOCKADDR_IN clientsocket;
    int len=sizeof(SOCKADDR);

    while (1)
    {
        GetLog()->Warn("Listen Port : %d", m_ListenPort);
        SOCKET serConn=accept(m_ListenSocket,(SOCKADDR*)&clientsocket,&len);//������ﲻ��accept����conection�Ļ������ͻ᲻�ϵļ���
        if(serConn == (SOCKET)SOCKET_ERROR)
        {
            return  -1;
        }

        if(m_WOWProxys.Count() == 0)
        {
            m_IsFirstStartWorking = 1;
        }

        WOWProxy * curProxy = new WOWProxy();
        m_WOWProxys.Add(curProxy);
		curProxy->SetDestIndex(m_GateIndex);
        curProxy->SetRealmIndex(m_RealmIndex);
        curProxy->SetProxyType(m_ProxyType);
        
		if(curProxy->GetProxyType() == PROXY_TYPE_REALM)
		{
			//��֤
			curProxy->fpOnUserAuthPacket = fpOnUserAuthPacket;
		}
		curProxy->Start(serConn, clientsocket, m_DestIP, m_DestPort);
        if (m_ProxyType == PROXY_TYPE_REALM)
        {
            m_RealmIndex++;
        }
        else
        {
            m_GateIndex++;
        }
    }
}

void			WOWProxyManager::SetForceRealmdIP(String realmdip, int port)
{
	m_RealmdIP = realmdip;
	m_RealmdPort = port;
}

void            WOWProxyManager::SetDestAddress(String addr)
{
	//������2����֤�˿�: 3724 �� 1119
    GetLog()->Warn("User Connected!%s", addr);

    auto_ptr<TStringList>   splitStr(new TStringList);
    SplitStr(addr, "|", splitStr.get());
    if(splitStr->Count != 3)
    {
        GetLog()->Warn("WOWProxy::SetDestAddress, Split error, addr = %s", addr);
        return;
    }

    m_DestIP = splitStr->Strings[0];
	m_DestPort = splitStr->Strings[1].ToIntDef(0);

    int connectIndex = splitStr->Strings[2].ToIntDef(0);
    if (connectIndex == 0)
    {
		m_ProxyType = PROXY_TYPE_REALM;
	}
    else
    {
        m_ProxyType = PROXY_TYPE_WORLD;
    }

	if(m_RealmdIP != "" && m_ProxyType == PROXY_TYPE_REALM)
	{
        // ʹ���Զ����realm ip
		m_DestIP = m_RealmdIP;
		m_DestPort = m_RealmdPort;
	}
}

WOWProxy    *   WOWProxyManager::GetActiveRealmd(int realmIndex)
{
    WOWProxy    *    curProxy = NULL;
    WOWProxy    *    lastProxy = NULL;
    for(int i=0; i<this->GetWOWProxyCount(); i++)
    {
		curProxy = this->GetWOWProxy(i);
        if(curProxy->GetProxyType() == PROXY_TYPE_REALM && curProxy->GetDestIndex() == realmIndex)
        {
            return curProxy;
        }
    }
    return          NULL;
}

WOWProxy    *   WOWProxyManager::GetActiveWorld(int worldIndex)
{
    WOWProxy    *    curProxy = NULL;
//    GetLog()->Warn("this->GetWOWProxyCount() = %d", this->GetWOWProxyCount());
    for(int i=0; i<this->GetWOWProxyCount(); i++)
    {
        curProxy = this->GetWOWProxy(i);
        if(curProxy->GetProxyType() == PROXY_TYPE_WORLD && curProxy->GetDestIndex() == worldIndex)
        {
            return curProxy;
        }
    }
    return          NULL;
}

bool            WOWProxyManager::Start(int listenPort, int listenThreadCount)
{
    m_ListenPort = listenPort;
    m_ListenThreadCount = listenThreadCount;

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,0),&wsaData);

    m_ListenSocket = socket(AF_INET,SOCK_STREAM,0);//�����˿�ʶ���׽���
    //��Ҫ�󶨵Ĳ���
    SOCKADDR_IN addr;
    addr.sin_family=AF_INET;
    addr.sin_addr.S_un.S_addr=htonl(INADDR_ANY);//ip��ַ
    addr.sin_port=htons(m_ListenPort);//�󶨶˿�
	if(bind(m_ListenSocket,(SOCKADDR*)&addr,sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		ShowMessage(FormatStr("Bind Port Failed:%d. %s", listenPort, SysErrorMessage(WSAGetLastError())));
		return false;
	}
	if(listen(m_ListenSocket,m_ListenThreadCount) == SOCKET_ERROR)
	{
		ShowMessage(FormatStr("List Port Failed:%d. %s", listenPort, SysErrorMessage(WSAGetLastError())));
		return false;
	}

	GetThreadManager()->ManagerCreateThread("ListenThread", ListenThread, false);
	return true;
}

void			WOWProxyManager::ResetConnections()
{
	for(int i=0; i<m_WOWProxys.Count(); i++)
	{
		m_WOWProxys[i]->Close();
	}
    m_GateIndex = 0;
    m_RealmIndex = 0;
}

void            WOWProxyManager::Close()
{
	if(m_ListenSocket)
	{
		closesocket(m_ListenSocket);
	}
   	ResetConnections();
}

WOWProxy    *   WOWProxyManager::GetWOWProxy(int index)
{
    return      m_WOWProxys[index];
}

int             WOWProxyManager::GetWOWProxyCount()
{
    return      m_WOWProxys.Count();
}

////////////////////////////////////////////////////////////////
WOWProxyPool::WOWProxyPool()
{
}

WOWProxyPool::~WOWProxyPool()
{
}

void                    WOWProxyPool::AddProxy()
{
    WOWProxyManager * proxy = new WOWProxyManager();
    m_WOWProxyManagers.Add(proxy);
}

WOWProxyManager *        WOWProxyPool::GetProxy(int index)
{
    return      m_WOWProxyManagers.At(index);
}

int                     WOWProxyPool::GetProxyCount()
{
    return      m_WOWProxyManagers.Count();
}


