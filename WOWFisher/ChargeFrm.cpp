//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "ChargeFrm.h"
#include "ClientAuther.h"
#include "ShareDef.h"
#include "ClientServerShared.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfrmChargeFrm *frmChargeFrm;
//---------------------------------------------------------------------------
__fastcall TfrmChargeFrm::TfrmChargeFrm(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TfrmChargeFrm::FormCreate(TObject *Sender)
{
//	PageControl1->ActivePageIndex = 0;
}
//---------------------------------------------------------------------------
void __fastcall TfrmChargeFrm::btChargeClick(TObject *Sender)
{
	AnsiString cardNo = edtChargeText->Text;
	AnsiString acc = edtAcc->Text;
	if(cardNo.Length() < CHARGE_CARD_SIZE_MIN || cardNo.Length() > CHARGE_CARD_SIZE_MAX)
	{
		ShowMessage(GBText("��ֵ����ʽ����"));
		return;
	}
	for(int i=0; i<acc.Length(); i++)
	{
		if(acc.c_str()[i] < 0)
		{
			ShowMessage(GBText("���������Ϸ�ʺ�����, ��ȷ�������������Ϸ�ʺ�, ���ǽ�ɫ����!"));
			return;
		}
	}
	String msg = FormatStr(GBText("��׼�����˺�[%s]��ֵ, �Ƿ�ȷ��?"), String(acc));
	if (Application->MessageBox(msg.c_str(),L"Charge",MB_OKCANCEL)!=IDOK)
	{
		return;
	}

//	PageControl1->ActivePageIndex = 1;
	memChargeMsg->Lines->Clear();
	GetClientAuth()->BeginCharge(acc, cardNo);
}
//---------------------------------------------------------------------------
void	TfrmChargeFrm::AddLog(String log)
{
	if(memChargeMsg->Lines->Count > 1000)
	{
		memChargeMsg->Lines->Clear();
	}
	memChargeMsg->Lines->Add(log);
}

void	TfrmChargeFrm::ShowFrm()
{
	if(this->Showing)
		return;
	memChargeMsg->Lines->Clear();
	this->Show();
}
