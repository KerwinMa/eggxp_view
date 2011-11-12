//---------------------------------------------------------------------------


#pragma hdrstop

#include "fsm.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)
int  CTransitionEvent::DoBeforeAction()  //״̬ת��֮ "ǰ" ִ���¼�����
{
	  if (m_BeforeEventAction!=NULL)
	  {
		 m_BeforeEventAction();
	  }
	  return _DesStauID;
}
//---------------------------------------------------------------------------
void  CTransitionEvent::DoAfterAction()  //״̬ת��֮ "��" ִ���¼�����
{
	  if (m_AfterEventAction!=NULL)
	  {
		 m_AfterEventAction();
	  }
}
//---------------------------------------------------------------------------
__fastcall CStat::CStat(int StatID,int StatImageIndex,String StatInfo,String Hint)
{
    _ID         = StatID;
    _ImageIndex = StatImageIndex;
    _Info       = StatInfo;
    _Hint       = Hint;
    TranstionList   = new  AList<CTransitionEvent>(true,100);
}
//---------------------------------------------------------------------------
__fastcall CStat::~CStat()
{
    delete TranstionList;
}
//---------------------------------------------------------------------------
int   CStat::GetTEIndex(int EventID)
{
   CTransitionEvent *TranEvent;
   for (int i=0;i<TranstionList->Count();i++)
   {
      TranEvent = TranstionList->At(i);
      if (TranEvent->GetEventID() == EventID)
      {
         return i;
      }
   }
   return -1;
}
//---------------------------------------------------------------------------
//  //����һ���¼��������
bool __fastcall  CStat::AddTransitionEvent(CTransitionEvent *NewTE)
{
    if (NewTE==NULL || GetTEIndex(NewTE->GetEventID())>=0)   //������Ч������Ѿ�����
    {
        return false;
	}
    TranstionList->Add(NewTE);
    return true;
}
//---------------------------------------------------------------------------
CTransitionEvent *  __fastcall  CStat::AddTransitionEvent(int EventID,TFUN_EVENT_ACTION BeforeEventAction,TFUN_EVENT_ACTION AfterAction,int DesStauID)
{
	if (GetTEIndex(EventID)>=0)   //������Ч������Ѿ�����
	{
		return NULL;
	}
	CTransitionEvent *NewTE = new CTransitionEvent(EventID,BeforeEventAction,AfterAction,DesStauID);
	if (AddTransitionEvent(NewTE))
	{
	   return NewTE;
	}
	else
	{
	   delete  NewTE;
	   return NULL;
	}
}
//---------------------------------------------------------------------------
void __fastcall CStat::EventSuccess(int EventID)
{
   int TEIndex = GetTEIndex(EventID);
   if (TEIndex==-1)
   {
	 return;
   }
   CTransitionEvent *TranEvent;
   TranEvent = TranstionList->At(TEIndex);
   if (TranEvent==NULL)
   {
	 return;
   }
   TranEvent->DoAfterAction();
}
//---------------------------------------------------------------------------
int __fastcall CStat::Transition(int EventID)
{
   int TEIndex = GetTEIndex(EventID);
   if (TEIndex==-1)
   {
	 return -1;
   }
   CTransitionEvent *TranEvent;
   TranEvent = TranstionList->At(TEIndex);
   if (TranEvent==NULL)
   {
	 return -1;
   }
   return TranEvent->DoBeforeAction(); //ִ���¼���Ӧ�Ķ�������,������һ��״̬
}
//״̬������
//---------------------------------------------------------------------------
__fastcall CFSM_Manage::CFSM_Manage()
{
    StatList = new  AList<CStat>(true,1000);
    OnStausChanage = NULL;
}
//---------------------------------------------------------------------------
__fastcall CFSM_Manage::~CFSM_Manage()
{
    delete  StatList;
}
//----------------------------------------------------------------------------
int   CFSM_Manage::GetStatIndex(int StatID)
{
   CStat *Stat;
   for (int i=0;i<StatList->Count();i++)
   {
      Stat = StatList->At(i);
      if (Stat->GetID() == StatID)
      {
         return i;
      }
   }
   return -1;
}
//----------------------------------------------------------------------------
CStat *    CFSM_Manage::GetStat(int StatID) //����ID��ȡ״̬����
{
    return StatList->At(GetStatIndex(StatID));
}
//----------------------------------------------------------------------------
bool __fastcall  CFSM_Manage::Add(CStat *NewStat)
{
	if (NewStat==NULL || GetStatIndex(NewStat->GetID())>=0)   //������Ч������Ѿ�����
	{
		return false;
    }
	StatList->Add(NewStat);
    return true;
}
//----------------------------------------------------------------------------
CStat * CFSM_Manage::Add(int StatID,int StatImageIndex,String StatInfo,String Hint)
{
	if (GetStatIndex(StatID)>=0)
	{
		return NULL;
	}
	CStat *NewStat  = new CStat(StatID,StatImageIndex,StatInfo,Hint);
	if (Add(NewStat))
	{
	   return  NewStat;
	}
	else
	{
	   delete  NewStat;
	   return NULL;
	}
}
//----------------------------------------------------------------------------
int __fastcall CFSM_Manage::Transition(int EventID)
{
   CStat * OldStat =  GetStat(_CurrentStatID);
   if (OldStat==NULL) //��ǰ����δ����״̬
	return -1;
   int DesStat =  OldStat->Transition(EventID);
   if (DesStat==-1)    //ִ��ת������ʧ��(�¼����ʹ���)
   {
	   return -1;
   }
   _CurrentStatID = DesStat;  //�ɹ�������ǰ״̬���ó��л����״̬
   OldStat->EventSuccess(EventID);
   if (OnStausChanage)
   {
	   OnStausChanage(OldStat,GetCurrentStat());
   }
   return 1;
}
//----------------------------------------------------------------------------
void __fastcall CFSM_Manage::SetStat(int NewStatID)
{
    _CurrentStatID=NewStatID;
   if (OnStausChanage)
   {
       OnStausChanage(GetCurrentStat(),GetCurrentStat());
   }

}

//----------------------------------------------------------------------------
void     CFSM_Manage::RefreshGUI()
{
   if (OnStausChanage)
   {
       OnStausChanage(GetCurrentStat(),GetCurrentStat());
   }
}
