//---------------------------------------------------------------------------

#ifndef DiabloIIIPackageDispatcherH
#define DiabloIIIPackageDispatcherH
//---------------------------------------------------------------------------
#include <vcl.h>
#include "ReviewerCommon.h"
#include "PackageCommon.h"

class	DiabloIIIPackageDispatcher : public PackageDispatcher
{
//�ڲ�ʹ�ñ���
private:
	WOWPackage				m_SendBufferPacket;
    WOWPackage				m_RecvBufferPacket;
//�ڲ�ʹ�ûص�
private:


//�ڲ�ʹ�ú���
public:
    String              GetOPcodeMsgByCMD(uint8 cmd);

    int                 DigestSendPacket();
    int                 DigestRecvPacket();
    int                 IsPacketNeedDecrypt(WOWPackage *packet);

public:
	DiabloIIIPackageDispatcher();
	~DiabloIIIPackageDispatcher();


	//���÷���
public:
	void				GetOrignSendPacket(WOWPackage *  packet);
    void				GetOrignRecvPacket(WOWPackage *  packet);
	//�������
	void				PackData(char * buf, int len);
	void				Clear();
};

#endif
