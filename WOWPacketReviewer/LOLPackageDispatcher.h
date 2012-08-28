//---------------------------------------------------------------------------

#ifndef LOLPackageDispatcherH
#define LOLPackageDispatcherH
//---------------------------------------------------------------------------

#include <vcl.h>
#include "ReviewerCommon.h"
#include "PackageCommon.h"
#include "BlowFish.h"


class LOLBlowFish
{
private:
	BlowFish	*	m_BlowFish;
public:
	LOLBlowFish();
	~LOLBlowFish();
	BlowFish	*	GetBlowFish();
	void	Init(AnsiString key);
};

LOLBlowFish * GetLOLBlowFish();

////////////////////////////////////////////////////////////////////////////////////

class	LOLPackageDispatcher : public PackageDispatcher
{
//�ڲ�ʹ�ñ���
private:
	WOWPackage				m_SendBufferPacket;
    WOWPackage				m_RecvBufferPacket;
//�ڲ�ʹ�ûص�
private:
	void					DecryptData(WOWPackage* pack);

//�ڲ�ʹ�ú���
public:
	String              GetOPcodeMsgByCMD(uint8 cmd);

	int                 DigestSendPacket();
	int                 DigestRecvPacket();
	int                 IsPacketNeedDecrypt(WOWPackage *packet);

public:
	LOLPackageDispatcher();
	~LOLPackageDispatcher();


	//���÷���
public:
	void				GetOrignSendPacket(WOWPackage *  packet);
    void				GetOrignRecvPacket(WOWPackage *  packet);
	//�������
	void				PackData(char * buf, int len);
	void				Clear();
};


#endif
