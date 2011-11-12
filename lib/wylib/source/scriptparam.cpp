//---------------------------------------------------------------------------


#pragma hdrstop

#include "scriptparam.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)
//---------------------------------------------------------------------------
WY_Params::WY_Params()
{
	m_ParamList = new AList<WYParam>(true,10);
	m_RunTimeCount = 0;
}
//---------------------------------------------------------------------------
WY_Params::~WY_Params()
{
    delete m_ParamList;
}
//---------------------------------------------------------------------------
WYParam * WY_Params::Add(const String &ParamName,const String &Info,Variant  Value,int Type)
{
    //�½�һ������
    WYParam * lpParam  =	Get(ParamName);
    if (lpParam)
    {
        return lpParam;
    }
    lpParam = new WYParam;
    lpParam->Value = Value;
    lpParam->Name = ParamName.UpperCase(); //ǿ��ת���ɴ�д����,�Ժ��Դ�Сд
    lpParam->Info = Info;
    lpParam->Type = Type;
    m_ParamList->Add(lpParam);
    return  lpParam;
}
//---------------------------------------------------------------------------
WYParam * WY_Params::Get(const String &ParamName)
{
    //ĳ�����Ƿ����
    for (int i = 0; i <Count(); i++)
    {
        if (m_ParamList->At(i)->Name ==  ParamName.UpperCase())
        {
            return (m_ParamList->At(i));
        }
    }
    return NULL;
}
//---------------------------------------------------------------------------
Variant  WY_Params::ValueByName(const String &ParamName)
{
    WYParam * lpParam  =	Get(ParamName);
    if (lpParam)
    {
        return lpParam->Value;
    }
    return  Variant(-1);
}
//---------------------------------------------------------------------------
Variant  WY_Params::ValueByIndex(int Index)
{
    WYParam * lpParam  =	m_ParamList->At(Index);
    if (lpParam)
    {
        return lpParam->Value;
    }
    return  Variant(-1);
}
//---------------------------------------------------------------------------
bool WY_Params::ReadFromStrings(TStrings * ParamStr)
{
    if (m_ParamList->Count() <  ParamStr->Count)
    {
        //������������ȷ
        return false;
	}

	m_RunTimeCount = ParamStr->Count;
	WYParam * lpParam ;
    for (int i = 0; i <ParamStr->Count; i++)
    {
        lpParam = m_ParamList->At(i);
		if (lpParam->Type == 0) //��ֵ�͵Ĳ���
        {
            lpParam->Value =  ParamStr->Strings[i].ToIntDef(-1);
        }
        else          //�ַ����Ĳ���
        {
            lpParam->Value =  ParamStr->Strings[i];
        }
    }
    return true;
}
//---------------------------------------------------------------------------
void WY_Params::Assign(WY_Params * OtherParams)
{
	WYParam * lpParam;
    m_ParamList->Clear();
    for (int i = 0; i <OtherParams->Count(); i++)
    {
        lpParam = OtherParams->At(i);
        Add(lpParam->Name,lpParam->Info,lpParam->Value,lpParam->Type);
    }
}
