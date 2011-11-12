//---------------------------------------------------------------------------

#ifndef WOWProxyH
#define WOWProxyH
//---------------------------------------------------------------------------
#include <vcl.h>
#include <WINSOCK2.H>
#include "ThreadManager.h"
#include "CommFunc.h"
#include "AList.h"
#include "AMap.h"
#include "PackageCommon.h"



class WOWProxy;
typedef	 void	(__closure * TOnUserAuthPacket)(WOWProxy *self, WOWPackage *packet);

class WOWProxy
{
private:
	CRITICAL_SECTION m_csLock;
	int					m_StopProxy;
    int                 m_IsDesabled;

    TProxyType          m_ProxyType;

	ASharedPtrQueue<WOWPackage>  m_ClientToServerQueue;
    ASharedPtrQueue<WOWPackage>  m_ServerToClientQueue;
	int                 m_Active;
	map<DWORD, DWORD>	m_ThreadState;

    String          m_DesIP;
    int             m_DesPort;
    SOCKADDR_IN     m_ClientAddr;

    SOCKET          m_ClientSocket;
    SOCKET          m_HostSocket;

    int             m_DestIndex;
    int             m_RealmIndex;       // 20110328 ��realm����

    bool            SendBuf_O(SOCKET  s, char * buf, int len);

	//���������߼�����ֱ�ӷ����ͻ���
	int             RecvProxy(SOCKET  from);
	int             SendProxy(SOCKET  to, ASharedPtrQueue<WOWPackage>  *pool);


	int             HostRecvThread(SingleThread * self);
	int             HostSendThread(SingleThread * self);
	int             ClientRecvThread(SingleThread * self);
	int             ClientSendThread(SingleThread * self);

	int				ThreadInitFunc(SingleThread * self);
	int				ThreadUnInitFunc(SingleThread * self);
    
public:
    WOWProxy();
	~WOWProxy();
   	TOnUserAuthPacket	fpOnUserAuthPacket;
    bool            Start(SOCKET client, SOCKADDR_IN clientAddr, String ip, int port);
    void            Close();

	ASharedPtrQueue<WOWPackage>   * GetClientToServerQueue(){return  &m_ClientToServerQueue;}
    ASharedPtrQueue<WOWPackage>   * GetServerToClientQueue(){return  &m_ServerToClientQueue;}
    int             GetDestIndex(){return       m_DestIndex;}
    void            SetDestIndex(int index){m_DestIndex = index;}
    GEN_GET_SET(int, RealmIndex)
	int             GetActive(){return      m_Active;}
	int             GetClosed();

    void            SetProxyType(int type){m_ProxyType = (TProxyType)type;}
	int             GetProxyType(){return      (int)m_ProxyType;}

	void			ServerAuthOKBeginProxy();
    int             GetDesPort(){return m_DesPort;}
    String          GetDesIP(){return m_DesIP;}
    GEN_GET_SET(int, IsDesabled)
};

/////////////////////////////////////////////////////////////////////////////
class WOWProxyManager
{
private:
	ASharedPtrQueue<WOWPackage>  m_AllQueue;
    AList<WOWProxy> m_WOWProxys;
    int             m_DestPort;
	String          m_DestIP;
	int				m_GateIndex;
    int             m_RealmIndex;
    TProxyType      m_ProxyType;

    int             m_ListenPort;
    SOCKET          m_ListenSocket;
	int             m_ListenThreadCount;
	String			m_RealmdIP;
	int				m_RealmdPort;
    int             m_IsFirstStartWorking;

	//�򵥴���ֱ��ģʽ�� ת���κη��, ������
	bool			m_DirectModel;

	int             ListenThread(SingleThread * self);

public:
	WOWProxyManager();
	~WOWProxyManager();

    void            SetProxyType(int type){m_ProxyType = (TProxyType)type;}
	int             GetProxyType(){return      (int)m_ProxyType;}
	bool            Start(int listenPort, int listenThreadCount = 100);
	void            Close();
	void			ResetConnections();
	TOnUserAuthPacket	fpOnUserAuthPacket;

	WOWProxy    *   GetWOWProxy(int index);
	int             GetWOWProxyCount();
	void            SetDestAddress(String addr);
	void			ProcessRemoveProxyList();
	void			SetForceRealmdIP(String realmdip, int port);
    GEN_GET_SET(int, IsFirstStartWorking)

	GEN_GET_SET(bool, DirectModel)

    WOWProxy    *   GetActiveRealmd(int realmIndex);
	WOWProxy    *   GetActiveWorld(int worldIndex);

	//ע�⣺ ħ�����緢��realm�ͷ���world�ķ��˳����Ҫ�󣬱��뱣֤˳������allqueue�ŵ���������
	//[����Ȼ���ܵ�������]
	ASharedPtrQueue<WOWPackage>   * GetAllQueue(){return    &m_AllQueue;}
};

class WOWProxyPool
{
private:
    AList<WOWProxyManager>      m_WOWProxyManagers;
public:
    WOWProxyPool();
    ~WOWProxyPool();

    void                    AddProxy();
    WOWProxyManager *       GetProxy(int index);
    int                     GetProxyCount();

};

WOWProxyManager  *       GetWOWProxyManager();
WOWProxyPool     *       GetWOWProxyPool();

#endif
