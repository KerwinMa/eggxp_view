//---------------------------------------------------------------------------


#pragma hdrstop

#include "clientsocket.h"
#include "CommFunc.h"

#define HIDE_WINDOW_CLASS   "CClientSocketWindow"
#define HIDE_WINDOW_CAPTION "ClientSocketWindow"
//---------------------------------------------------------------------------

#pragma package(smart_init)
__fastcall CClientSocket::CClientSocket(HINSTANCE Instance)
{
    Init();
    m_hInstance =  Instance;
    m_InitSuccess = CreateSocketWindow();

}
//---------------------------------------------------------------------------
__fastcall CClientSocket::CClientSocket(TForm *ParentForm)
{
    Init();
    m_ParentForm       = ParentForm;
    m_ParentWindowMode = true;
    //�滻������Ϣ������
    m_HWnd =  ParentForm->Handle;
    m_pfFormUserWndProc    = ParentForm->WindowProc;
    ParentForm->WindowProc = OnNetWindowMessage;
}
//---------------------------------------------------------------------------
__fastcall CClientSocket::~CClientSocket()
{
    //����û��ص�����,����֪ͨ�û���Ϣ
    OnConnect    = NULL;
    OnDisconnect = NULL;
    OnRecv       = NULL;
    OnSend       = NULL;
    OnError      = NULL;
    //��������Ӿ��ȶϿ�
    if(m_Connected)
    {
        Disconnect();
    }
    //�ָ��û����ں���
    if(m_HWnd)
    {
        if(m_ParentWindowMode)
        {
           m_ParentForm->WindowProc = m_pfFormUserWndProc; //�ָ����ڵ�windows Proc
        }
        else
        {
            UnHookWindowWndProc();
            DestroyWindow(m_HWnd);
        }

        m_HWnd = NULL;
    }

    WSACleanup();
}
//---------------------------------------------------------------------------
void CClientSocket::Init()
{
    WSAStartup(MAKEWORD(2, 0), &wsaData);
    m_Connected = false;
    m_ClientSocketID = INVALID_SOCKET;
    OnConnect    = NULL;
    OnDisconnect = NULL;
    OnRecv       = NULL;
    OnSend       = NULL;

    OnError      = NULL;
    memset(m_ServerIP,0,sizeof(m_ServerIP));
    memset(LastError,0,sizeof(LastError));
    m_ServerPort = 0;
    m_pfOrgWndProc = NULL;
    m_ParentWindowMode   = false;
    m_HWnd = NULL;
    m_hInstance        = NULL;
    m_pfFormUserWndProc  = NULL;
    m_ParentForm =   NULL;
    UserData = NULL;
}

//---------------------------------------------------------------------------
void CClientSocket::ThrowError(int ErrorCode)
{
    ShowDebugInfo(FormatStr("CClientSocket Error:%s",LastError));
    ShowDebugInfo(FormatStr("CClientSocket Error:%s",SysErrorMessage(ErrorCode)));
    if (OnError)
    {
      OnError(this,ErrorCode,LastError);
    }
}
//---------------------------------------------------------------------------
bool CClientSocket::Connect()
{
    SOCKADDR_IN sockAddr;
    if(m_ClientSocketID != INVALID_SOCKET || m_Connected)
    {
        wsprintf(LastError, "socket is active:%d", m_ClientSocketID);
        ThrowError(0);
        return false;
    }

    m_ClientSocketID = socket(AF_INET, SOCK_STREAM, 0);

    if(m_ClientSocketID == INVALID_SOCKET)
    {
        wsprintf(LastError, "socket() generated error %d\n",WSAGetLastError());
        ThrowError(WSAGetLastError());
        return false;
    }

    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(m_ServerPort);
    sockAddr.sin_addr.s_addr = inet_addr(m_ServerIP);
    //sockAddr.sin_addr.S_un   = sockAddr
    if(WSAAsyncSelect(m_ClientSocketID, m_HWnd, ID_SOCKCLIENT_EVENT_MSG,
       FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE) == SOCKET_ERROR)
    {
        wsprintf(LastError,
                 "WSAAsyncSelect() generated error %d,HWND:%d\n",
                 WSAGetLastError(),m_HWnd);
        ThrowError(WSAGetLastError());
        CloseSocket();
        return false;
    }

    if(connect(m_ClientSocketID, (const struct sockaddr FAR *) &sockAddr,
       sizeof(sockAddr)) == SOCKET_ERROR)
    {
        int ErrorCode = WSAGetLastError();
        if (WSAEWOULDBLOCK==ErrorCode)
        {
            OutputDebugString("This socket is Non-Blocking Mode");
        }
        else
        {
            ThrowError(ErrorCode);
            CloseSocket();
            return false;
        }
    }

    return true;
}
//---------------------------------------------------------------------------
void CClientSocket::CloseSocket()
{
    closesocket(m_ClientSocketID);
    m_ClientSocketID = INVALID_SOCKET;
    m_Connected = false;
}
//---------------------------------------------------------------------------
bool CClientSocket::Disconnect(bool ServerDisconnect)
{
    if (!m_Connected && m_ClientSocketID == INVALID_SOCKET)    //�Ѿ��ر���
      return false;
    m_Connected = false;
    if(m_ClientSocketID != INVALID_SOCKET)
    {
        CloseSocket();
        if(OnDisconnect)
        {
            OnDisconnect(this, ServerDisconnect);
        }
        return true;
    }

    return false;
}
//---------------------------------------------------------------------------
//BCB���Ĵ�����Ϣ�ص�����(������Ϊ���Ա����)
void __fastcall CClientSocket::OnNetWindowMessage(Messages::TMessage &Message)
{
    if(Message.Msg == ID_SOCKCLIENT_EVENT_MSG)
    {
        if(WSAGETSELECTERROR(Message.LParam))   //������
        {
            ThrowError(WSAGETSELECTERROR(Message.LParam));
            Disconnect();
            return;
        }

        switch(WSAGETSELECTEVENT(Message.LParam))
        {
            case FD_CONNECT:
                {
                    m_Connected = true;
                    if(OnConnect)
                    {
                        OnConnect(this);
                    }
                    break;
                }

            case FD_CLOSE:
                {
                    Disconnect(true);
                    break;
                }

            case FD_READ:
                {
                    if(OnRecv)                  //�������������,���ݽ���ʹ����������
                    {
                        OnRecv(this, (SOCKET) Message.WParam);
                    }

                    /*recv((SOCKET)wParam,
                                                                                  &m_szBuff[m_nCurRecv],
                                                                                  nRecv - nCurRecv,
                                                                                  0);  */
                    break;
                }
            case FD_WRITE:
            {
                    if(OnSend)                  //�������������,���ݽ���ʹ����������
                    {
                        OnSend(this, (SOCKET) Message.WParam);
                    }
            }
        }
    }
    else
    {
        //�����ʹ���������ڷ�ʽ,����Ҫ�����������ڵ���Ϣ������������Ϣ
        if(m_pfFormUserWndProc)
        {
            m_pfFormUserWndProc(Message);
        }
    }
}
//VC����,������ȫ�ֺ�����static����
//---------------------------------------------------------------------------
LRESULT CALLBACK CClientSocket::DefWindowMessageProc(HWND WinHwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return TRUE;
}
//---------------------------------------------------------------------------
void CClientSocket::HookWindowWndProc()
{

    //����Ϣ���ڴ������滻�����Ա����
    m_pfOrgWndProc = (WNDPROC)GetWindowLong(m_HWnd, GWL_WNDPROC);
    SetWindowLong(m_HWnd,
                  GWL_WNDPROC,
                  LONG(MakeObjectInstance(&OnNetWindowMessage)));
}
//---------------------------------------------------------------------------
void CClientSocket::UnHookWindowWndProc()
{
    void    *lpWindowProc = (void *)GetWindowLong(m_HWnd, GWL_WNDPROC);
    if(lpWindowProc != (void *) m_pfOrgWndProc)
    {
        FreeObjectInstance(lpWindowProc);
        SetWindowLong(m_HWnd,
                      GWL_WNDPROC,
                      LONG(m_pfOrgWndProc));
    }
}
//---------------------------------------------------------------------------
bool CClientSocket::CreateSocketWindow()
{
    WNDCLASSEX  wc;
    if(!GetClassInfoEx(m_hInstance, HIDE_WINDOW_CLASS, &wc))    //�����ȡ����Ϣʧ��,֤����ûע�ᴰ����
    {
        wc.cbSize = sizeof(wc);
        wc.lpszClassName = HIDE_WINDOW_CLASS;
        wc.lpfnWndProc = &DefWindowProc;
        wc.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
        wc.hInstance = m_hInstance;
        wc.hIcon = NULL;
        wc.hIconSm = NULL;
        wc.hCursor = NULL;                                      //LoadCursor(m_hInstance, IDC_ARROW);
        wc.hbrBackground = (HBRUSH) (GRAY_BRUSH);               //(COLOR_WINDOW + 1);
        wc.lpszMenuName = NULL;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        if(RegisterClassEx(&wc) == 0)                           //ע�ᴰ����
        {
            int ClassError = GetLastError();
            wsprintf(LastError,
                     "RegisterClassEx Fail,ErrorCode:%d",
                     ClassError);
            ThrowError(ClassError);
        }
    }

    DWORD   dwStyle = WS_POPUPWINDOW /*WS_OVERLAPPEDWINDOW*/ &~WS_MAXIMIZEBOX;
    m_HWnd = CreateWindowEx(WS_EX_TOOLWINDOW,
                            HIDE_WINDOW_CLASS,
                            HIDE_WINDOW_CAPTION,
                            dwStyle,
                            0,
                            0,
                            0,
                            0,
                            NULL,
                            NULL,
                            m_hInstance,
                            NULL);

    if(m_HWnd == NULL)
    {
        wsprintf(LastError, "Creare Window Fail,ErrorCode:%d", GetLastError());
        ThrowError(GetLastError());
        return false;
    }
    else
    {
        HookWindowWndProc();
        return true;
    }
}
//---------------------------------------------------------------------------
int  CClientSocket::RecvBuf(char * Buf,int Len)
{
  if (!m_Connected)
  {
    return 0;
  }
  int ResultLen=0;
  if (Len <= 0)   //ֻȡ���������ݳ���,��ȡ����
  {
     ioctlsocket(m_ClientSocketID, FIONREAD,(u_long *)&ResultLen);
     return ResultLen;
  }
  ResultLen  = recv(m_ClientSocketID, Buf, Len, 0);
  if (ResultLen == SOCKET_ERROR)
  {
    int ErrorCode = WSAGetLastError();
    if (ErrorCode != WSAEWOULDBLOCK)  //������
    {
        wsprintf(LastError,
                 "recv() generated error %d\n",
                 ErrorCode);
        ThrowError(ErrorCode);
        Disconnect();
        return SOCKET_ERROR;
    }
  }
  return ResultLen;
}
//---------------------------------------------------------------------------
int CClientSocket::SendBuf(char *Buf, int Len)
{
    if(!m_Connected)
    {
        return 0;
    }

    int ResultLen = send(m_ClientSocketID, Buf, Len, 0);
    if(ResultLen == SOCKET_ERROR)
    {
        int ErrorCode = WSAGetLastError();
        if(ErrorCode != WSAEWOULDBLOCK) //������
        {
            wsprintf(LastError, "send() generated error %d\n", ErrorCode);
            ThrowError(ErrorCode);
            Disconnect();
            return SOCKET_ERROR;
        }
    }

    return ResultLen;
}
//---------------------------------------------------------------------------
int  CClientSocket::ReceiveLength()
{
   return RecvBuf(NULL,-1);
}
//---------------------------------------------------------------------------
void CClientSocket::SetServerIP(char * IP)
{
   if (m_Connected)
   {
      return;
   }
   memset(m_ServerIP,0,sizeof(m_ServerIP));
   int DesLen = strlen(IP);
   if (DesLen > 16)
   {
     return;
   }
   CopyMemory(m_ServerIP,IP,DesLen);
}
//---------------------------------------------------------------------------
void CClientSocket::SetServerPort(WORD Port)
{
   if (m_Connected)
   {
      return;
   }
   m_ServerPort = Port;
}
