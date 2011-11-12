//---------------------------------------------------------------------------


#pragma hdrstop

#include "NetStream.h"
#include "Log4Me.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

CNetStream::CNetStream(SOCKET    socketID) : m_PackBufer()
{
    m_SocketID = socketID;
}

CNetStream::~CNetStream()
{
}

int         CNetStream::SendBuf(char *lpData, int len)
{
    return  send(m_SocketID, lpData, len, 0);
}

int         CNetStream::RecvBuf(char *lpData, int len)
{                     
    return  recv(m_SocketID, lpData, len, 0);
}

int         CNetStream::SendBuf_n(char *lpData, int len)
{
    //��������
    int iTotalLen=len;
    int iSendLen=0;
    int ret;
    do {
        ret=SendBuf(lpData+iSendLen, iTotalLen-iSendLen );

        //���ʹ���
        if (ret<=0)
        {
            return  ret;
        }
        else
        {
            iSendLen+=ret;
        }
    }while(iSendLen!=iTotalLen);
    return  len;
}
                     
int         CNetStream::SendBuf_o(char *lpData, int len)
{
    try
    {
        if(len == 0)
        {
            if(m_PackBufer.GetResultLength() <= 0)
            {
                //û����Ϣ����
                SetLastError(ERR_SEND_ZERO);
                return 0;
            }
            else
            {
                int ret = SendBuf_n(m_PackBufer.GetResult(), m_PackBufer.GetResultLength());
                m_PackBufer.Reset();
                return  ret;
            }

        }


        if(m_PackBufer.Put(lpData, len))
            return  len;

        //�Ų���, Ӧ��send��
        if(m_PackBufer.GetResultLength())
        {
            int ret = SendBuf_n(m_PackBufer.GetResult(), m_PackBufer.GetResultLength());
            if(ret <= 0)
                return ret;
        }
        m_PackBufer.Reset();
        if(m_PackBufer.Put(lpData, len) == false)
        {
            //���Ƿ��Ͳ���, ֤���ǳ�����, ֱ�ӷ���
            return  SendBuf_n(lpData, len);
        }
        return len;
    }
    catch(Exception &e)
    {
        GetLog()->Fatal("CNetStream::SendBuf_o, lpData = %d, len = %d", lpData, len);
        return  0;
    }
}


/////////////////////////////////////////////////////////////////////////
//����256k�ռ�
CICOPNetStream::CICOPNetStream(IServerClient * serverClient) : m_PackBufer(256 * 1024)
{
    m_IServerClient = serverClient;
}

CICOPNetStream::~CICOPNetStream()
{
}
                     
int         CICOPNetStream::SendBuf_o(char *lpData, int len)
{
    try
    {
        if(len == 0)
        {                     
            if(m_PackBufer.GetResultLength() <= 0)
            {
                //û����Ϣ����
                SetLastError(ERR_SEND_ZERO);
                return 0;
            }
            else
            {             
                int ret = m_IServerClient->SendBuffer(m_PackBufer.GetResult(), m_PackBufer.GetResultLength());
                m_PackBufer.Reset();
                return  ret;
            }

        }


        if(m_PackBufer.Put(lpData, len))
            return  len;

        //�Ų���, Ӧ��send��
        if(m_PackBufer.GetResultLength())
        {
            int ret = m_IServerClient->SendBuffer(m_PackBufer.GetResult(), m_PackBufer.GetResultLength());
            if(ret <= 0)
                return ret;
        }
        m_PackBufer.Reset();
        if(m_PackBufer.Put(lpData, len) == false)
        {
            //���Ƿ��Ͳ���, ֤���ǳ�����, ֱ�ӷ���
            return  m_IServerClient->SendBuffer(lpData, len);
        }
        return len;
    }
    catch(Exception &e)
    {
        GetLog()->Fatal("CICOPNetStream::SendBuf_o, lpData = %d, len = %d", lpData, len);
        return  0;
    }
}
