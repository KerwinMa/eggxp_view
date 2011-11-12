//---------------------------------------------------------------------------

#ifndef NetStreamH
#define NetStreamH
//---------------------------------------------------------------------------

#include    "fl_blockclient.h"
#include    "fl_IServer.h"
#include    <VCL.h>
#include    "PackBuffer.h"

#define MAKE_ERROR(n) (n | 0x20000000) 
#define ERR_SEND_ZERO      MAKE_ERROR(0x1)


class   PackBufer;
class CNetStream
{
private:
    SOCKET          m_SocketID;

    PackBufer       m_PackBufer;
public:
    CNetStream(SOCKET    socketID);
    ~CNetStream();

    int         SendBuf(char *lpData, int len);
    int         RecvBuf(char *lpData, int len);

    //����ָ��n�ֽڵķ��
    int         SendBuf_n(char *lpData, int len);
    //����ָ��n�ֽڵķ��, ����û���յ�ָ�����ȵķ��, �������ȴ�
//    int         RecvBuf_n(char *lpData, int len);

    //�Ż���, �����С���ۼƵ�һ���Ļ�������С, �ٷ���
    //���len���ڻ�����, ֱ�ӷ���
    //���lenС�ڻ�����, �ۼ�, �������޺��ٷ���
    //���lenΪ0�Ļ�, ��ʾ��������ʣ�໺��������
    int         SendBuf_o(char *lpData, int len);
};



class CICOPNetStream
{
private:
    IServerClient   *   m_IServerClient;

    PackBufer       m_PackBufer;
public:
    CICOPNetStream(IServerClient * serverClient);
    ~CICOPNetStream();

    //����ָ��n�ֽڵķ��, ����û���յ�ָ�����ȵķ��, �������ȴ�
//    int         RecvBuf_n(char *lpData, int len);

    //�Ż���, �����С���ۼƵ�һ���Ļ�������С, �ٷ���
    //���len���ڻ�����, ֱ�ӷ���
    //���lenС�ڻ�����, �ۼ�, �������޺��ٷ���
    //���lenΪ0�Ļ�, ��ʾ��������ʣ�໺��������
    int         SendBuf_o(char *lpData, int len);
};
#endif
