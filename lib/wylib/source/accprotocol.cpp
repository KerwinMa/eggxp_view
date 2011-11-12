//---------------------------------------------------------------------------


#pragma hdrstop
#include <SysUtils.hpp>
#include "accprotocol.h"
#include "GenericFunc.h"
#include "CommFunc.h"


//---------------------------------------------------------------------------

#pragma package(smart_init)

//CAccBase Class
//---------------------------------------------------------------------------
__fastcall CAccBase::CAccBase()
{
    m_IsEncryptPacket = true;
    m_lpCrypt = NewCryptClass(DATA_ENCRYPT_TYPE);
}
//---------------------------------------------------------------------------
__fastcall CAccBase::~CAccBase()
{
    delete m_lpCrypt;
}

//---------------------------------------------------------------------------
double    CAccBase::GetRandKey()
{
   return Now()-Now() % 1000;
}
//---------------------------------------------------------------------------
bool CAccBase::ReadHead(char *buf,int len,MSGHEAD &Head)
{
  //��ȡ���ͷ
  if (len<MSG_HEAD_SIZE) //���Ȳ���
  {
    return false;
  }
  CopyMemory(&Head,buf,MSG_HEAD_SIZE); //ȡ�÷��ͷ��Ϣ
  if (Head.PackFlag != PACK_FLAG || len < Head.DataSize || Head.DataSize<MSG_HEAD_SIZE) //���ͷ���Ի��߷��������
  {
    return false;
  }
  return true; //��ȡ���ͷ�ɹ�
}
//---------------------------------------------------------------------------
bool  CAccBase::CheckPacket(char *buf,int len)
{
   //У�����Ϸ��� �Ϸ�����0,�Ƿ����ط�0
   //������:
   //1 = ��ȡ���ͷʧ��
   //2 = δ֪��������
   //3 = �����ܳ��Ȳ���ȷ
   //4 =������ݳ��Ȳ���ȷ
   //5 =���MD5У���ʧ��
   MSGHEAD Head;
   if (!ReadHead(buf,len,Head))   //��ȡ���ͷʧ��
   {
      LastError =  "���ͷ����";
      return false;
   }
   if (!IsValidityCmd(Head.Cmd))
   {
      LastError = FormatStr("δ֪���� %d",Head.Cmd);
      return false;    //δ֪��������
   }
   if (len < Head.DataSize)
   {
      LastError = FormatStr("���ȴ��� len = %d,datasize = %d",len,Head.DataSize);
      return false;                 //�����ܳ��Ȳ���ȷ
   }
   int PacketDataLen = Head.DataSize - MSG_HEAD_SIZE;
   if (PacketDataLen<=0)        //������ݳ��Ȳ���ȷ
   {
      LastError = FormatStr("PacketDataLen ���� %d",PacketDataLen);
      return false;
   }
   //DumpString(String(buf,len),"d:\\serverloginall.dat");
   //DumpString(String(&buf[MSG_HEAD_SIZE],PacketDataLen),"d:\\serverlogin.dat");
   String DataMD5 = m_MD5.GetMD5(&buf[MSG_HEAD_SIZE],PacketDataLen);
   if (DataMD5 != Head.CmdMD5)
   {
      LastError = FormatStr("local md5[%s] !=remote md5[%s]",DataMD5,Head.CmdMD5);
      ZeroMemory(buf,len);
      return false;  //���MD5У���ʧ��
   }
   return true;      //�ɹ�

}
//---------------------------------------------------------------------------
void   CAccBase::SetPacketEncryptMode(bool Mode)
{
     m_IsEncryptPacket = Mode;
}
//---------------------------------------------------------------------------
bool  CAccBase::GetPacketEncryptMode()
{
     return m_IsEncryptPacket;
}
//---------------------------------------------------------------------------
bool  CAccBase::IsValidityCmd(BYTE Cmd)
{
  TCmdType CmdType = (TCmdType)Cmd;
  return (CmdType == acLOGIN || CmdType == acCREATE_USER || CmdType == acSAVEPOINT || CmdType == acCMDBACK || CmdType ==  acQUERYCARD);
}
//---------------------------------------------------------------------------
int CAccBase::DecryptPacket(char *buf,int len,double RandKey)
{
   Xor_Sub(buf,len,(char *)&RandKey,sizeof(RandKey));
   return len;
}
//---------------------------------------------------------------------------
int CAccBase::EncryptPacket(char *buf,int len,double RandKey)
{
   Xor_Add(buf,len,(char *)&RandKey,sizeof(RandKey));
   return len;
}
#ifdef SERVER_CRYPT_SIDE
//---------------------------------------------------------------------------
//CAccServerProtocol Class
//---------------------------------------------------------------------------
__fastcall CAccServerProtocol::CAccServerProtocol()
{

}
//---------------------------------------------------------------------------
__fastcall CAccServerProtocol::~CAccServerProtocol()
{

}
//---------------------------------------------------------------------------
bool     CAccServerProtocol::ReadLoginCmd(char *buf,int len,PC_LOGIN_PACKET LoginPacket)
{
    if (!ReadHead(buf,len,LoginPacket->PackHead)) //���ͷ��ȡʧ��
    {
       return false;
    }
    if (LoginPacket->PackHead.Cmd != acLOGIN)
    {
       //��½��־����
       return false;
    }
    //��ȡRSA��Կ
    CopyMemory(&LoginPacket->KeyLen,buf+MSG_HEAD_SIZE,RSA_KEY_BUFFER_LEN+1);
    if (LoginPacket->KeyLen > RSA_KEY_BUFFER_LEN)
    {
       //����KeyLen���ȹ���
       return false;
    }
    String EncryptKey = String(LoginPacket->CryptKey,LoginPacket->KeyLen);
    //ʹ��˽Կ����,��ԭ�����ݼ���Key
    EncryptKey = m_Rsa.PrivateDecrypt(EncryptKey);
    //��������
    m_lpCrypt->Init(EncryptKey);
    int HeadSize = MSG_HEAD_SIZE+RSA_KEY_BUFFER_LEN+1;
    String ClientLoginData =  String(&buf[HeadSize],len-HeadSize);
    ClientLoginData = m_lpCrypt->DecryptStr(ClientLoginData);
    int ClientLen = ClientLoginData.Length();
    if (ClientLen < C_LOGIN_PACKET_SIZE-HeadSize)
    {
       //����������ݳ��ȴ���
       return false;
    }
    //���ƽ�������
    CopyMemory(LoginPacket,buf,MSG_HEAD_SIZE);
    CopyMemory(&LoginPacket->UserId,ClientLoginData.c_str(),C_LOGIN_PACKET_SIZE-HeadSize);
    return true;
}
//---------------------------------------------------------------------------
bool     CAccServerProtocol::ReadCreateUserCmd(char *buf,int len,PC_CREATE_USER_PACKET CreatePacket)
{
    if (!ReadHead(buf,len,CreatePacket->PackHead)) //���ͷ��ȡʧ��
    {
       return false;
    }
    if (CreatePacket->PackHead.Cmd != acCREATE_USER)
    {
       return false;
    }
    if (m_IsEncryptPacket)
    {
       DecryptPacket(&buf[MSG_HEAD_SIZE],CreatePacket->PackHead.DataSize-MSG_HEAD_SIZE,CreatePacket->PackHead.RandKey);
    }
    CopyMemory(CreatePacket,buf,CreatePacket->PackHead.DataSize);
    return true;
}
//---------------------------------------------------------------------------
bool   CAccServerProtocol::ReadSavePointCmd(char *buf,int len,PC_SAVE_POINT_PACKET SpPacket)
{
    if (!ReadHead(buf,len,SpPacket->PackHead)) //���ͷ��ȡʧ��
    {
       return false;
    }
    if (SpPacket->PackHead.Cmd != acSAVEPOINT)
    {
       return false;
    }
    if (m_IsEncryptPacket)
    {
       DecryptPacket(&buf[MSG_HEAD_SIZE],SpPacket->PackHead.DataSize-MSG_HEAD_SIZE,SpPacket->PackHead.RandKey);
    }
    CopyMemory(SpPacket,buf,SpPacket->PackHead.DataSize);
    return true;
}
//---------------------------------------------------------------------------
bool   CAccServerProtocol::ReadQueryCardCmd(char *buf,int len,PC_QUERY_CARD_PACKET SpPacket)
{
    if (!ReadHead(buf,len,SpPacket->PackHead)) //���ͷ��ȡʧ��
    {
       return false;
    }
    if (SpPacket->PackHead.Cmd != acQUERYCARD)
    {
       return false;
    }
    if (m_IsEncryptPacket)
    {
       DecryptPacket(&buf[MSG_HEAD_SIZE],SpPacket->PackHead.DataSize-MSG_HEAD_SIZE,SpPacket->PackHead.RandKey);
    }
    CopyMemory(SpPacket,buf,SpPacket->PackHead.DataSize);
    return true;
}
//------------------------------------------------------------------------------
String   CAccServerProtocol::MakeLoginReturn(S_LOGIN_PACKET &ServerData)
{
    //�����ͷ
    ServerData.Head.PackHead.PackFlag = PACK_FLAG;
    ServerData.Head.PackHead.RandKey  = GetRandKey();
    ServerData.Head.PackHead.Cmd      = acLOGIN;
    //�����չ�����Ƿ���ȷ
    if (ServerData.Head.ExDataLen != ServerData.ExData.Length())
    {
       ServerData.Head.ExDataLen = 0;
    }
    //����ʵ�����ݳ��� = ����ܳ���-��Ϣͷ����
    int PackLen = S_LOGIN_PACKET_HEAD_SIZE -MSG_HEAD_SIZE;
    int KeyDataLen = RSA_KEY_BUFFER_LEN+1; //���Key�����ݳ���
    //��������
     //���ɸ���ע��id���ɼ���Key
       String EncryptKey =   MakeKeyByID(ServerData.Head.UserId);
       m_lpCrypt->Init(EncryptKey);
       //���ɴ����ܻ�����
       String EncryptData = String((char *)&ServerData.Head.ReturnCode,PackLen-KeyDataLen)+ServerData.ExData;
       //���ܷ������,�������������Ϣͷ
       EncryptData = m_lpCrypt->EncryptStr(EncryptData);
       //����Keyʹ��RSA 1024λ˽Կ���ܱ���
       EncryptKey = m_Rsa.PrivateEncrypt(EncryptKey);
       ServerData.Head.KeyLen = EncryptKey.Length();

    String PacketDataBuffer; //����������շ���Ļ�����
    //������ܺ�ķ������  ����Key��Ϣ����+���ܺ�����ݳ���
    PackLen =  KeyDataLen+EncryptData.Length();
    //���û�������С = ���ͷ����+������ݳ���
    PacketDataBuffer.SetLength(MSG_HEAD_SIZE+PackLen);
    ServerData.Head.PackHead.DataSize = PacketDataBuffer.Length();
    int Pos = MSG_HEAD_SIZE;
    //д����Կ����
    WriteBYTE(PacketDataBuffer.c_str(),Pos,ServerData.Head.KeyLen);
    WriteBuf(PacketDataBuffer.c_str(),Pos,EncryptKey.c_str(),ServerData.Head.KeyLen);
    Pos = Pos+ (RSA_KEY_BUFFER_LEN-ServerData.Head.KeyLen);
    //д����ܺ������
    WriteBuf(PacketDataBuffer.c_str(),Pos,EncryptData.c_str(),EncryptData.Length());
    //����md5
    String MD5Str =  m_MD5.GetMD5((PacketDataBuffer.c_str()+MSG_HEAD_SIZE),PacketDataBuffer.Length()-MSG_HEAD_SIZE);
    memset(ServerData.Head.PackHead.CmdMD5,0,sizeof(ServerData.Head.PackHead.CmdMD5));
    wsprintf(ServerData.Head.PackHead.CmdMD5,"%s",MD5Str);
    //д��ͷ��Ϣ
    CopyMemory(PacketDataBuffer.c_str(),&ServerData.Head.PackHead,sizeof(ServerData.Head.PackHead));
    return  PacketDataBuffer;
}
//---------------------------------------------------------------------------
String   CAccServerProtocol::MakeLoginReturn_Old(S_LOGIN_PACKET &ServerData)
{
    //�����ͷ
    ServerData.Head.PackHead.PackFlag = PACK_FLAG;
    ServerData.Head.PackHead.RandKey  = GetRandKey();
    ServerData.Head.PackHead.Cmd      = acLOGIN;
    if (ServerData.Head.ExDataLen != ServerData.ExData.Length())
    {
       ServerData.Head.ExDataLen = 0;
    }
    int PackLen = S_LOGIN_PACKET_HEAD_SIZE + ServerData.Head.ExDataLen-MSG_HEAD_SIZE;
    String ReturnData;
    ServerData.Head.PackHead.DataSize = S_LOGIN_PACKET_HEAD_SIZE + ServerData.Head.ExDataLen;
    ReturnData.SetLength(ServerData.Head.PackHead.DataSize);
    char *lpbuf = ReturnData.c_str();
    int  DataPos=0;
    CopyMemory(&lpbuf[DataPos],&ServerData.Head,sizeof(ServerData.Head));
    DataPos += sizeof(ServerData.Head);
    CopyMemory(&lpbuf[DataPos],ServerData.ExData.c_str(),ServerData.ExData.Length());
    DataPos += sizeof(ServerData.ExData.Length());
    if (m_IsEncryptPacket) //�Ƿ���ܷ��
    {
       //���ɸ���ע��id���ɼ���Key
       String EncryptKey =   MakeKeyByID(ServerData.Head.UserId);
       m_lpCrypt->Init(EncryptKey);
       //���ɴ����ܻ�����
       int KeyDataLen = RSA_KEY_BUFFER_LEN+1; //���Key�����ݳ���
       String EncryptData = String(&lpbuf[MSG_HEAD_SIZE+KeyDataLen],PackLen-KeyDataLen);
       //���ܷ������,�������������Ϣͷ
       EncryptData = m_lpCrypt->EncryptStr(EncryptData);
       //����Keyʹ��RSA 1024λ˽Կ���ܱ���
       EncryptKey = m_Rsa.PrivateEncrypt(EncryptKey);
    }

    ZeroMemory(&ServerData.Head.PackHead.CmdMD5,sizeof(ServerData.Head.PackHead.CmdMD5));
    String MD5Str =  m_MD5.GetMD5(&lpbuf[MSG_HEAD_SIZE],PackLen);
    wsprintf(ServerData.Head.PackHead.CmdMD5,"%s",MD5Str);
    CopyMemory(lpbuf,&ServerData.Head.PackHead,sizeof(ServerData.Head.PackHead));
    return ReturnData;
}
//---------------------------------------------------------------------------
bool   CAccServerProtocol::MakeQueryReturn(S_QUERY_CARD_PACKET &ServerData)
{
    ServerData.PackHead.PackFlag = PACK_FLAG;
    ServerData.PackHead.RandKey  = GetRandKey();
    ServerData.PackHead.Cmd      = acQUERYCARD;

    int PackLen = sizeof(S_QUERY_CARD_PACKET) - MSG_HEAD_SIZE;
    ServerData.PackHead.DataSize = sizeof(S_QUERY_CARD_PACKET);
    if (m_IsEncryptPacket) //�Ƿ���ܷ��
    {
       //���ܷ������,�������������Ϣͷ
       EncryptPacket(ServerData.CardNO,PackLen,ServerData.PackHead.RandKey);
    }
    ZeroMemory(&ServerData.PackHead.CmdMD5,sizeof(ServerData.PackHead.CmdMD5));
    String MD5Str =  m_MD5.GetMD5(ServerData.CardNO,PackLen);
    wsprintf(ServerData.PackHead.CmdMD5,"%s",MD5Str);
    return true;
}
//---------------------------------------------------------------------------
String  CAccServerProtocol::MakeKeyByID(String ID)
{
    String KeyOrg =  ID+IntToStr(GetCrc32(0,ID.c_str(),ID.Length()))+IntToStr(GetTickCount());
    return m_MD5.GetMD5(KeyOrg.c_str(),KeyOrg.Length());
}
//---------------------------------------------------------------------------
S_RETURN_PACKET  CAccServerProtocol::MakeCommBackCmd(int ReturnCode,BYTE CmdType)
{
    S_RETURN_PACKET  ServerData;
    ServerData.PackHead.PackFlag = PACK_FLAG;
    ServerData.PackHead.RandKey  = GetRandKey();
    ServerData.PackHead.Cmd      = CmdType;
    ServerData.ReturnCode        = ReturnCode;
    int PackLen = S_RETURN_PACKET_SIZE -MSG_HEAD_SIZE;
    ServerData.PackHead.DataSize = PackLen+MSG_HEAD_SIZE;
    if (m_IsEncryptPacket) //�Ƿ���ܷ��
    {
       //���ܷ������,�������������Ϣͷ
       EncryptPacket((char *)&ServerData.ReturnCode,PackLen,ServerData.PackHead.RandKey);
    }
    ZeroMemory(&ServerData.PackHead.CmdMD5,sizeof(ServerData.PackHead.CmdMD5));
    String MD5Str =  m_MD5.GetMD5((char *)&ServerData.ReturnCode,PackLen);
    wsprintf(ServerData.PackHead.CmdMD5,"%s",MD5Str);
    return ServerData;
}
#endif
//---------------------------------------------------------------------------
//CAccClientProtocol Class
//---------------------------------------------------------------------------
__fastcall CAccClientProtocol::CAccClientProtocol()
{

}
//---------------------------------------------------------------------------
__fastcall CAccClientProtocol::~CAccClientProtocol()
{

}
//---------------------------------------------------------------------------
String  CAccClientProtocol::MakeLoginCmd(String UserId,String PassWord,BYTE ItemType,DWORD Reserve)
{
   C_LOGIN_PACKET LoginPacket;
   ZeroMemory(&LoginPacket,C_LOGIN_PACKET_SIZE);
   LoginPacket.PackHead.PackFlag = PACK_FLAG;
   LoginPacket.PackHead.RandKey  = GetRandKey();
   LoginPacket.PackHead.Cmd      = acLOGIN;
   wsprintf(LoginPacket.UserId,"%s",UserId);
   wsprintf(LoginPacket.UserPass,"%s",PassWord);
   LoginPacket.ItemType = ItemType;
   LoginPacket.Reserve  = Reserve;


    //����ʵ�����ݳ��� = ����ܳ���+��չ���ݳ���-��Ϣͷ����
    int PackLen = C_LOGIN_PACKET_SIZE -MSG_HEAD_SIZE;
    int KeyDataLen = RSA_KEY_BUFFER_LEN+1; //���Key�����ݳ���
     //��������
     //���ɸ���ע��id���ɼ���Key
       String EncryptKey =   MakeKeyByID(LoginPacket.UserId);
       m_lpCrypt->Init(EncryptKey);
       //���ɴ����ܻ�����
       String EncryptData = String((char *)&LoginPacket.UserId,PackLen-KeyDataLen);
       //���ܷ������,�������������Ϣͷ
       EncryptData = m_lpCrypt->EncryptStr(EncryptData);
       //����Keyʹ��RSA 1024λ˽Կ���ܱ���
       EncryptKey = m_Rsa.PublicEncrypt(EncryptKey);
       LoginPacket.KeyLen = EncryptKey.Length();

    String PacketDataBuffer; //����������շ���Ļ�����
    //������ܺ�ķ������  ����Key��Ϣ����+���ܺ�����ݳ���
    PackLen =  KeyDataLen+EncryptData.Length();
    //���û�������С = ���ͷ����+������ݳ���
    PacketDataBuffer.SetLength(MSG_HEAD_SIZE+PackLen);
    LoginPacket.PackHead.DataSize = PacketDataBuffer.Length();
    int Pos = MSG_HEAD_SIZE;
    //д����Կ����
    WriteBYTE(PacketDataBuffer.c_str(),Pos,LoginPacket.KeyLen);
    WriteBuf(PacketDataBuffer.c_str(),Pos,EncryptKey.c_str(),LoginPacket.KeyLen);
    Pos = Pos+ (RSA_KEY_BUFFER_LEN-LoginPacket.KeyLen);
    //д����ܺ������
    WriteBuf(PacketDataBuffer.c_str(),Pos,EncryptData.c_str(),EncryptData.Length());
    //����md5
    String MD5Str =  m_MD5.GetMD5((PacketDataBuffer.c_str()+MSG_HEAD_SIZE),PacketDataBuffer.Length()-MSG_HEAD_SIZE);
    //д��md5
    memset(LoginPacket.PackHead.CmdMD5,0,sizeof(LoginPacket.PackHead.CmdMD5));
    wsprintf(LoginPacket.PackHead.CmdMD5,"%s",MD5Str);
    //д��ͷ��Ϣ
    CopyMemory(PacketDataBuffer.c_str(),&LoginPacket.PackHead,MSG_HEAD_SIZE);
    return  PacketDataBuffer;
}
//---------------------------------------------------------------------------
C_CREATE_USER_PACKET CAccClientProtocol::MakeCreateUserCmd(String UserId,String PassWord,String Email,BYTE GameType,DWORD Reserve)
{
    C_CREATE_USER_PACKET  ServerData;
    ZeroMemory(&ServerData,C_CREATE_USER_PACKET_SIZE);
    ServerData.PackHead.PackFlag = PACK_FLAG;
    ServerData.PackHead.RandKey  = GetRandKey();
    ServerData.PackHead.Cmd      = acCREATE_USER;
    wsprintf(ServerData.UserId,"%s",UserId);
    wsprintf(ServerData.UserPass,"%s",PassWord);
    wsprintf(ServerData.EMail,"%s",Email);
    ServerData.ItemType = GameType;
    ServerData.Reserve  = Reserve;
    int PackLen = C_CREATE_USER_PACKET_SIZE -MSG_HEAD_SIZE;
    ServerData.PackHead.DataSize = C_CREATE_USER_PACKET_SIZE;
    if (m_IsEncryptPacket) //�Ƿ���ܷ��
    {
       //���ܷ������,�������������Ϣͷ
       EncryptPacket((char *)&ServerData.UserId,PackLen,ServerData.PackHead.RandKey);
    }
    String MD5Str = m_MD5.GetMD5((char *)&ServerData.UserId,PackLen);
    wsprintf(ServerData.PackHead.CmdMD5,"%s",MD5Str);
    return ServerData;
}
//---------------------------------------------------------------------------
C_SAVE_POINT_PACKET  CAccClientProtocol::MakeSavePointCmd(String CardNo,String CardPassWord,String UserId,String lgUserId,String PassWord,WORD CardType,BYTE GameType,int Reserve)
{
    C_SAVE_POINT_PACKET  ServerData;
    ZeroMemory(&ServerData,C_SAVE_POINT_PACKET_SIZE);
    ServerData.PackHead.PackFlag = PACK_FLAG;
    ServerData.PackHead.RandKey  = GetRandKey();
    ServerData.PackHead.Cmd      = acSAVEPOINT;
    wsprintf(ServerData.CardNo ,"%s",CardNo);
    wsprintf(ServerData.CardPass,"%s",CardPassWord);
    wsprintf(ServerData.UserId,"%s",UserId);
    wsprintf(ServerData.UserPass,"%s",PassWord);
    ServerData.Cardtype = CardType;
    ServerData.ItemType = GameType;
    ServerData.Reserve  = Reserve;
    int PackLen = C_SAVE_POINT_PACKET_SIZE -MSG_HEAD_SIZE;
    ServerData.PackHead.DataSize = C_SAVE_POINT_PACKET_SIZE;
    if (m_IsEncryptPacket) //�Ƿ���ܷ��
    {
       //���ܷ������,�������������Ϣͷ
       EncryptPacket((char *)&ServerData.CardNo,PackLen,ServerData.PackHead.RandKey);
    }
    String MD5Str = m_MD5.GetMD5((char *)&ServerData.CardNo,PackLen);
    wsprintf(ServerData.PackHead.CmdMD5,"%s",MD5Str);
    return ServerData;
}
//---------------------------------------------------------------------------
bool   CAccClientProtocol::ReadLoginBackCmd(char * buf,int len,PS_LOGIN_PACKET LoginCmd)
{
    if (!CheckPacket(buf,len)) //���ͷ��ȡʧ��
    {
       return false;
    }
    if (!ReadHead(buf,len,LoginCmd->Head.PackHead))
    {
       return false;
    }
    if (LoginCmd->Head.PackHead.Cmd != acLOGIN)
    {
       return false;
    }
    //��ȡKey
    CopyMemory(&LoginCmd->Head.KeyLen,buf+MSG_HEAD_SIZE,RSA_KEY_BUFFER_LEN+1);
    if (LoginCmd->Head.KeyLen >RSA_KEY_BUFFER_LEN)
    {
       return false;
    }
    String EncryptKey = String((char *)&LoginCmd->Head.CryptKey,LoginCmd->Head.KeyLen);
    EncryptKey = m_Rsa.PublicDecrypt(EncryptKey);
    //����.....!!!!!!
    //�ڴ˴����Եõ�����Key������ EncryptKey
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    m_lpCrypt->Init(EncryptKey);        //<---- ����
    //ͷ��Ϣ���� ���ͷ+RSAKey����������+1
    int HeadLen =   MSG_HEAD_SIZE + RSA_KEY_BUFFER_LEN+1;
    //���ݳ���  =�ܳ���-ͷ����
    int DataLen =  LoginCmd->Head.PackHead.DataSize - HeadLen;
    String EncryptData = String(buf+HeadLen,DataLen);
    EncryptData = m_lpCrypt->DecryptStr(EncryptData);
    #ifdef _DEBUG
    DumpString(EncryptData,"c:\\newlogin.dat");
    #endif
    m_lpCrypt->Init("");
    int DecryptDataLen = S_LOGIN_PACKET_HEAD_SIZE-HeadLen;
    if (EncryptData.Length() < DecryptDataLen)
    {
       return false;
    }
    //���Ʒ��ͷ
    CopyMemory(&LoginCmd->Head.PackHead,buf,sizeof(LoginCmd->Head.PackHead));
    //���Ʒ������
    CopyMemory(&LoginCmd->Head.ReturnCode,EncryptData.c_str(),DecryptDataLen);
    //������չ����
    if (LoginCmd->Head.ExDataLen>0)
    {
      LoginCmd->ExData.SetLength(LoginCmd->Head.ExDataLen);
      CopyMemory(LoginCmd->ExData.c_str(),EncryptData.c_str()+DecryptDataLen,LoginCmd->Head.ExDataLen);
    }
    return true;
}
//---------------------------------------------------------------------------
bool   CAccClientProtocol::ReadPackBackCmd(char *buf,int len,PS_RETURN_PACKET ReturnPack)
{
    if (!CheckPacket(buf,len) || !ReadHead(buf,len,ReturnPack->PackHead)) //���ͷ��ȡʧ��
    {
       return false;
    }
    if (m_IsEncryptPacket)
    {
       DecryptPacket(&buf[MSG_HEAD_SIZE],ReturnPack->PackHead.DataSize-MSG_HEAD_SIZE,ReturnPack->PackHead.RandKey);
    }
    CopyMemory(ReturnPack,buf,ReturnPack->PackHead.DataSize);
    return true;
}

//---------------------------------------------------------------------------
C_QUERY_CARD_PACKET  CAccClientProtocol::MakeQueryCardCmd(String CardNo,WORD CardType,BYTE GameType,int Reserve)
{
    C_QUERY_CARD_PACKET  ServerData;
    ZeroMemory(&ServerData,sizeof(C_QUERY_CARD_PACKET));
    ServerData.PackHead.PackFlag = PACK_FLAG;
    ServerData.PackHead.RandKey  = GetRandKey();
    ServerData.PackHead.Cmd      = acQUERYCARD;
    wsprintf(ServerData.CardNO,"%s",CardNo);
    ServerData.Cardtype = CardType;
    ServerData.ItemType = GameType;
    ServerData.Reserve  = Reserve;
    int PackLen = sizeof(C_QUERY_CARD_PACKET) -MSG_HEAD_SIZE;
    ServerData.PackHead.DataSize = sizeof(C_QUERY_CARD_PACKET);
    if (m_IsEncryptPacket) //�Ƿ���ܷ��
    {
       //���ܷ������,�������������Ϣͷ
       EncryptPacket((char *)&ServerData.CardNO,PackLen,ServerData.PackHead.RandKey);
    }
    String MD5Str = m_MD5.GetMD5((char *)&ServerData.CardNO,PackLen);
    wsprintf(ServerData.PackHead.CmdMD5,"%s",MD5Str);
    return ServerData;
}
//---------------------------------------------------------------------------
bool      CAccClientProtocol::ReadQueryCardCmd(char *buf,int len,PS_QUERY_CARD_PACKET ReturnPack)
{
    if (!CheckPacket(buf,len) || !ReadHead(buf,len,ReturnPack->PackHead)) //���ͷ��ȡʧ��
    {
       return false;
    }
    if (ReturnPack->PackHead.Cmd !=acQUERYCARD)
    {
       return false;
    }
    if (m_IsEncryptPacket)
    {
       DecryptPacket(&buf[MSG_HEAD_SIZE],ReturnPack->PackHead.DataSize-MSG_HEAD_SIZE,ReturnPack->PackHead.RandKey);
    }
    CopyMemory(ReturnPack,buf,ReturnPack->PackHead.DataSize);
    return true;

}
//---------------------------------------------------------------------------
String  CAccClientProtocol::MakeKeyByID(String ID)
{
    return EncodeStr(ID+"=frmMain"+IntToStr(GetTickCount()));
}
