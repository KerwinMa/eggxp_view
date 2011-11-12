//---------------------------------------------------------------------------

#ifndef WYProfileH
#define WYProfileH
//---------------------------------------------------------------------------
#include <vcl.h>
#include "amap.h"
//�ۺ�����������
//ʹ��ע��
//PROFILE �����̰߳�ȫ,
// ���Ҫ�ڶ��߳���ʹ�ã������������߳�new һ��CProfileManager������߳�ר��
//Ȼ�����THREAD_PROFILE �����
class CTimeCount;
//��������ڵ���

class CProfileNode
{
private:
	AIndexListPro<String,CProfileNode> * m_ChildList;	//���������ӽڵ�
	CTimeCount  * m_TimeCount;
	double		  m_LastCountValue;
	DWORD		  m_CountTimes;
	CProfileNode  * m_Parent;    //���ڵ�
protected:
	String m_Name;
public:
	CProfileNode(String Name);
	~CProfileNode();
	String  GetName(){return  m_Name;}
	void	SetParent(CProfileNode  * Node){m_Parent=Node;}
	CProfileNode * GetParent(){return  m_Parent;}  //��ȡ���ڵ�
	CProfileNode * GetChild(String Name);	//��ȡ�ӽڵ� ,�������ڻᴴ��һ��
	int			   ChildCount(){return m_ChildList->Count();}
	CProfileNode * GetChildByIndex(int Index){return m_ChildList->At(Index);}
	void    Clear();		//���
	void	Start();		//��ʼ��������
	void	End();          //������������
	double  GetCountValue(){return m_LastCountValue;}  //��ȡ���һ��ͳ�ƺ�ʱ
	DWORD	GetCountTimes(){return m_CountTimes;}		//��ȡͳ�ƴ���
	double	GetPercent();	//��ȡ��ͳ�ƽڵ�ռ�ø�ͳ�ƽڵ�İٷֱ�
	void	IncCount(double AddCount){m_LastCountValue+=AddCount;} //�ۼ�ͳ��ʱ��ֵ
};

//�������ݹ�������
class CProfileManager
{
private:
protected:
	CProfileNode * m_RootNode;  //���������ĸ��ڵ�
	CProfileNode * m_CurrentNode;	//��ǰ�ڵ�
	CProfileNode * m_SelecttViewNode; //��ѡ��Ĺ۲�ڵ�
    String         m_Name;
public:
   CProfileManager();
   virtual  ~CProfileManager();
   static CProfileManager * GetIns();		//
   virtual  void StartProfile(const char * Name);
   virtual  void Stopprofile();
   virtual  void Reset();		//ͳ�Ƹ�λ
   virtual  void SetSelecttViewNode(CProfileNode * Node);
   virtual  void     SetName(String name);
   
   CProfileNode * GetSelecttViewNode(){return m_SelecttViewNode;}

   String   GetName();

   String   Clear();
};

class       CThreadProfileManager;
typedef         void        (__closure  *   TWatchFunc)(CThreadProfileManager   *   profile);
class   CThreadProfileManager : public CProfileManager
{
private:
    bool            m_bIsNeedWatch;
    TWatchFunc      fpOnWatch;
public:
    CThreadProfileManager();
    virtual  ~CThreadProfileManager();

    virtual  void Reset();		//ͳ�Ƹ�λ
    virtual  void SetNeedWatch(TWatchFunc   watchFunc);

    void    __fastcall      WatchInfo();
};


//����ӿ���
class CProfileSample
{
private:
	CProfileManager * m_ProfileMgr;
protected:

public:
   CProfileSample(CProfileManager * ProfileMgr,char * Name)
   {
	  m_ProfileMgr = ProfileMgr;
	  ProfileMgr->StartProfile(Name);
   }
   ~CProfileSample()
   {
	  m_ProfileMgr->Stopprofile();
   }
};

//���ڵ��߳�
#ifdef ENABLE_PROFILE
#define PROFILE(name) CProfileSample	__profile(CProfileManager::GetIns(),name)
//���ڶ��̻߳���
#define THREAD_PROFILE(Ptr_ProfileManager,name) CProfileSample	__profile(Ptr_ProfileManager,name)
#else
#define PROFILE(name) ;
#define THREAD_PROFILE(Ptr_ProfileManager,name) ;
#endif
#endif
