//---------------------------------------------------------------------------


#pragma hdrstop

#include "AutoManageListConfig.h"
#include "ShareData.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

//ini�洢��ʽ��xxxxxxxx-xxxxxx-xxxxxx
//             ��Ʒ    -�����-Ԥ����
void	OneListData::LoadSaveString(String iniStr)
{
	auto_ptr<TStringList> ValueList(new TStringList());
	SplitStr(iniStr,"-",ValueList.get());


	if(ValueList->Count != 3)
		return;
		
	m_ManageItem = ValueList->Strings[0];
	m_Tag	=	ValueList->Strings[1];
	m_ManageMinCount	=	ValueList->Strings[2];
}

String 	OneListData::OutPutString()
{
	return FormatStr("%s-%s-%s", m_ManageItem, m_Tag, m_ManageMinCount);
}

void	OneListData::Init(String item, int tag, int count)
{
	m_ManageItem = item;
	m_Tag = IntToStr(tag);
	m_ManageMinCount = IntToStr(count);
}

void		OneListData::InitStr(String item, String tag, String count)
{
	m_ManageItem = item;
	m_Tag = tag;
	m_ManageMinCount = count;
}

void		OneListData::ReadData(OneListData * other)
{
	m_ManageItem = other->m_ManageItem;
	m_Tag = other->m_Tag;
	m_ManageMinCount = other->m_ManageMinCount;
}

union		TUnionData
{
	int		Data;
	WORD	Part[2];
};

int			OneListData::GetManageMinCountHi()
{
	int data = m_ManageMinCount.ToInt();
	TUnionData	unionData;
	unionData.Data = data;
	return	unionData.Part[0];
}

int		OneListData::GetManageMinCountLo()
{
	int data = m_ManageMinCount.ToIntDef(0);
	TUnionData	unionData;
	unionData.Data = data;
	return	unionData.Part[1];
}

int			OneListData::GetTagHi()
{
	int data = m_Tag.ToIntDef(0);
	TUnionData	unionData;
	unionData.Data = data;
	return	unionData.Part[0];
}

int			OneListData::GetTagLo()
{
	int data = m_Tag.ToIntDef(0);
	TUnionData	unionData;
	unionData.Data = data;
	return	unionData.Part[1];
}

//==================================================================================

//--------------------------------------------------------------------------------
AutoManageList::AutoManageList(String indexName, TAutoManage am, int refreshType)
{
	m_StringList = new TStringList;
	m_ListGui = NULL;
	m_ValueListGUI = NULL;
	m_IndexName = indexName;
	m_Index = am;
	m_RefreshType = refreshType;

	m_bItemLoaded = false;
}

//--------------------------------------------------------------------------------
AutoManageList::~AutoManageList()
{
	delete m_StringList;
}

//-------------------------------------------------------------------------

//--------------------------------------------------------------------------------
String		AutoManageList::GetTagString(String tag)
{
	String result = tag;
	return result;
}

//--------------------------------------------------------------------------------
String		AutoManageList::GetCountString(int count)
{
	return FormatStr(_DT("Ԥ��%d��").c_str(), count);
}

//--------------------------------------------------------------------------------
String		AutoManageList::GetDataGuiCaption(OneListData * data)
{
	int		 id = data->m_ManageItem.ToIntDef(0);

	if(m_Index == amAutoKongfu || m_Index == amFastSkill || m_Index == amLearnKongfuList)
	{
		IWogonRecInterface * curKongfu = GameData()->GetWogonRec(id);
		if(curKongfu == NULL)
			return data->m_ManageItem;

		return TEXT_BIG5(curKongfu->GetName());
	}
	else if(m_Index == amChangeLine)
	{
		return	data->m_ManageItem;
	}
	else if(IsItem())
	{
		IItemObjInterface * itemInterface = GameData()->GetItem(id);
		if(itemInterface == NULL)
			return "";

		return	FormatStr("%s (%d)", TEXT_BIG5(itemInterface->GetName()), id);
	}
	else if(m_Index == amUnAttackNPCType)
	{
		//����NPC�����͵õ�NPC������
		tagNPCDetail detail;
		if(GameData()->GetNPCDetailByTypeID(id, detail))
		{
			return	FormatStr("%s (%d)", TEXT_BIG5(detail.Name), id);
		}

		return "";
	}
	else if(m_Index == amAutoRepair)
	{
		return	GetEquipNameByIndex(id) + _DT("��(�;�С��1)");
	}
	else if(m_Index == amAskTeamPlayer || m_Index == amAccTeamLeader)
	{
		return	data->m_ManageItem;
	}
    else if(m_Index == amAutoPaoTui)
    {
        int itemID = GetPetPaoTuiItemIDByIndex(id);
        IItemObjInterface * itemInterface = GameData()->GetItem(itemID);
		if(itemInterface == NULL)
			return "";

		return	FormatStr("%s (%d)", TEXT_BIG5(itemInterface->GetName()), itemID);
    }

	return	data->m_ManageItem;
}

bool		AutoManageList::IsItem()
{
	if(m_Index == amAutoDropItem || m_Index == amAutoUploadItem || m_Index == amAutoOpenBox ||
		m_Index == amAutoSell || m_Index == amAutoBuy || m_Index == amAutoSaveMoneyHouse ||
		m_Index == amAutoDelivery || m_Index == amAutoGetDelivery || m_Index == amAutoBuyShopItem ||
		m_Index == amAutoSellShopItem || m_Index == amAutoMerge || m_Index == amAutoSearch)
		return true;
	return false;
}

String		AutoManageList::GetDataGuiText(OneListData * data)
{
	if(m_Index == amAutoKongfu)
	{
		return	FormatString(_DT("%d ��"), data->GetTag());
	}
	else if(m_Index == amAutoRepair)
	{
		IItemObjInterface * itemInterface = GameData()->GetItem(data->GetTag());
		if(itemInterface == NULL)
			return "";

		return	FormatStr("%s (%d)", TEXT_BIG5(itemInterface->GetName()), data->GetTag());
	}
	else	if(m_Index == amAutoSellShopItem || m_Index == amAutoSearch)
	{
		return	GetPriceText(data->GetTag());
	}
	else	if(m_Index == amDaiLianAsk || m_Index == amAccDaiLian)
	{
		int tag = data->GetTag();
		if(tag == 0)
		{
			return	_DT("����ͷ��");
		}
		else	if(tag == 1)
		{
			return	_DT("����");
		}
		else	if(tag == 2)
		{
			return	_DT("���");	
		}
	}

	return	data->GetTagStr();
}

String		AutoManageList::GetDataGuiText2(OneListData * data)
{
	if(m_Index == amAutoKongfu)
	{
		if(data->GetManageMinCount() == 0)
		{
			return _DT("һֱ����");
		}
		else if(data->GetManageMinCount() == 1)
		{
			return _DT("��·ʱ����");
		}
		else if(data->GetManageMinCount() == 2)
		{
			return _DT("ս��ʱ����");
		}
	}
	if(m_Index == amLearnKongfuList)
	{
		if(data->GetManageMinCount() != 0)
			return _DT("��");
		else
			return _DT("��");
	}
	if(m_Index == amAutoBuyShopItem)
	{
		return	GetPriceText(data->GetManageMinCount());
	}
	return	data->GetManageMinCountStr();
}

void	AutoManageList::AddGuiItem(OneListData * data)
{
	if(m_ListGui)
	{
		if(m_RefreshType == 0)
		{
			TListItem * addItem = m_ListGui->Add();
			addItem->Caption = GetDataGuiCaption(data);
			addItem->SubItems->Add(GetDataGuiText(data));
			addItem->SubItems->Add(GetDataGuiText2(data));
		}
		else if(m_RefreshType == 1)
		{
			TListItem * addItem = m_ListGui->Add();
			addItem->Caption = GetDataGuiCaption(data);
			addItem->SubItems->Add(data->GetTagHi());
			addItem->SubItems->Add(data->GetTagLo());
			addItem->SubItems->Add(GetDataGuiText2(data));
		}
	}
}

//--------------------------------------------------------------------------------
//��ȡini
void	AutoManageList::LoadFromIni(TMemIniFile * IniFile,const String &SecName)
{
	m_ListData.Clear();
	SplitStrOld(IniFile->ReadString(SecName,m_IndexName, "") ,",",m_StringList);

	//�޲�
	for(int i=0; i<m_StringList->Count; i++)
	{
		if(m_StringList->Strings[i] == " " || m_StringList->Strings[i] == "" ||
				m_StringList->Strings[i] == NULL)
			m_StringList->Delete(i);
	}

	//ת��Ϊ�ṹ��
	OneListData * addData;
	for(int i=0; i<m_StringList->Count; i++)
	{
		addData = new OneListData;
		addData->LoadSaveString(m_StringList->Strings[i]);
		m_ListData.Add(addData);
	}

	if(m_Index == amAutoRepair)
	{
		if(Count() != MAX_EQUIP_COUNT)
		{
            //���װ��������ݸ�����(�����·�)���ﲻ��Clear, ��ֹ�ƽ�鳱������
            while(Count() < MAX_EQUIP_COUNT)
            {
                Add(Count(), 36001);		//36001������, Ĭ������������
            }
            
            if(Count() > MAX_EQUIP_COUNT)
            {
                Clear();
                for(int i=0; i<MAX_EQUIP_COUNT; i++)
                {
                    Add(i, 36001);		//36001������, Ĭ������������
                }
            }
		}
	}
	else	if(m_Index == amAutoBuy)
	{
//		Add(38007, 1);		//һ��Ҫ��سǾ�		ȡ����ѡ��
	}

	m_bItemLoaded = true;
}

//-------------------------------------------------------------------------
String  AutoManageList::MakeListToString(TStringList * destList)
{
	String TeamList;
	for (int i = 0; i < destList->Count; i++)
	{
	   if(i ==0)
	   {
		 TeamList =  destList->Strings[i];
	   }
	   else
	   {
		 TeamList =  TeamList + "," +destList->Strings[i];
	   }
	}
	return TeamList;
}

//--------------------------------------------------------------------------------
//����ini
void	AutoManageList::SaveToIni(TMemIniFile * IniFile, String RecordName)
{
	//������б�
	m_StringList->Clear();
	for(int i=0; i<m_ListData.Count(); i++)
	{
		m_StringList->Add(m_ListData[i]->OutPutString());
	}
	IniFile->WriteString(RecordName, m_IndexName,MakeListToString(m_StringList));
}

//--------------------------------------------------------------------------------
//�󶨽���
void	AutoManageList::Bind(TListItems * GuiList)
{
		
	m_ListGui = GuiList;

	RefreshGUI();
}

void	AutoManageList::Bind(TValueListEditor * valueListEditor)
{
	m_ValueListGUI = valueListEditor;
	valueListEditor->Strings->Clear();
	String curValue;
	for(int i=0; i<Count(); i++)
	{
		curValue = FormatStr("%s=%s", GetDataGuiCaption(m_ListData[i]),
										GetDataGuiText(m_ListData[i]));
		valueListEditor->Strings->Add(curValue);
		valueListEditor->ItemProps[i]->EditStyle = esEllipsis;
	}
}

void	AutoManageList::RefreshGUI()
{
	if(m_ListGui == NULL)
		return;
	m_ListGui->Clear();

	for(int i = 0; i < m_ListData.Count(); i++)
	{
		AddGuiItem(m_ListData[i]);
	}
}

void	AutoManageList::Delete(int index)
{
    if(index < 0)
        return;
	m_ListData.Delete(index);
	if(m_ListGui)
	{
		m_ListGui->Delete(index);
	}

	if(m_Index == amPatrol)
	{
		//����Ѳ�ߵ��ʱ��, Ҫ�ı�����
		for(int i=0; i<m_ListData.Count(); i++)
		{
			m_ListData[i]->m_ManageItem = IntToStr(i);
		}
		RefreshGUI();
	}
}

void	AutoManageList::Clear()
{
	m_ListData.Clear();
	if(m_ListGui)
	{
		m_ListGui->Clear();
	}
}

void	AutoManageList::Add(String item, int tag, int count)
{
	if(IsExist(item))
		return;
	OneListData	* addData = new OneListData;
	addData->Init(item, tag, count);
	
	m_ListData.Add(addData);
	AddGuiItem(addData);
}

void	 AutoManageList::Add(String item, WORD tagHi, WORD tagLo, int count)
{
	TUnionData	unData;
	unData.Part[0] = tagHi;
	unData.Part[1] = tagLo;

	Add(item, unData.Data, count);
}

void	AutoManageList::AddStr(String item, String tag, String count)
{
	if(IsExist(item))
		return;
	OneListData	* addData = new OneListData;
	addData->InitStr(item, tag, count);
	
	m_ListData.Add(addData);
	AddGuiItem(addData);
}

void	AutoManageList::EditStr(int index, String item, String tag, String count)
{
	OneListData	* curData = At(index);
	if(curData)
	{
		curData->InitStr(item, tag, count);
	}
	EditModifyView(index, curData);
}

void	AutoManageList::EditModifyView(int index, OneListData * curData)
{
	if(m_ListGui)
	{
		if(index >= 0 && index < m_ListGui->Count)
		{
			TListItem * curItem = m_ListGui->Item[index];
			curItem->Caption = GetDataGuiCaption(curData);
			if(curItem->SubItems->Count == 0)
			{
				curItem->SubItems->Add(GetDataGuiText(curData));
				curItem->SubItems->Add(GetDataGuiText2(curData));
			}
			else if(curItem->SubItems->Count == 1)
			{
				curItem->SubItems->Strings[0] = GetDataGuiText(curData);
				curItem->SubItems->Add(GetDataGuiText2(curData));
			}
            else if(curItem->SubItems->Count == 3)
			{
				curItem->SubItems->Strings[0] = curData->GetTagHi();
				curItem->SubItems->Strings[1] = curData->GetTagLo();
                curItem->SubItems->Strings[2] = GetDataGuiText2(curData);
			}
            else
			{
				curItem->SubItems->Strings[0] = GetDataGuiText(curData);
				curItem->SubItems->Strings[1] = GetDataGuiText2(curData);
			}
		}
	}

	if(m_ValueListGUI)
	{
		if(index >= 0 && index < m_ValueListGUI->Strings->Count)
			m_ValueListGUI->Strings->Strings[index] = FormatStr("%s=%s", GetDataGuiCaption(m_ListData[index]),
										GetDataGuiText(m_ListData[index]));
	}
}

void	AutoManageList::Edit(int index, String item, int tag, int count)
{
	OneListData	* curData = At(index);
	if(curData)
	{
		curData->Init(item, tag, count);
	}
  	EditModifyView(index, curData);
}

void	AutoManageList::Edit(int index, String item, WORD tagHi, WORD tagLo, int count)
{
    OneListData	* curData = At(index);
	if(curData)
	{
        TUnionData	unData;
        unData.Part[0] = tagHi;
        unData.Part[1] = tagLo;
		curData->Init(item, unData.Data, count);
	}
  	EditModifyView(index, curData);
}

int		AutoManageList::IndexOf(String item)
{
	for(int i=0; i<m_ListData.Count(); i++)
	{
		if(m_ListData[i]->m_ManageItem == item)
			return i;
	}
	return -1;
}

bool	AutoManageList::IsExist(String item)
{
	for(int i=0; i<m_ListData.Count(); i++)
	{
		if(m_ListData[i]->m_ManageItem == item)
			return true;
	}

	//���⴦��, �������Ʒ�Ļ�, �����2����Ʒ��������ͬ, ��ô��Ϊ��ͬһ����Ʒ
	if(IsItem())
	{
		IItemObjInterface	*  curItem;
		IItemObjInterface	*  destItem = GameData()->GetItem(item.ToIntDef(0));
		if(destItem == NULL)
			return false;
		for(int i=0; i<m_ListData.Count(); i++)
		{
			curItem = GameData()->GetItem(m_ListData[i]->m_ManageItem.ToIntDef(0));
			if(curItem)
			{
				if(curItem->GetName() == destItem->GetName())
					return true;
			}
		}
	}
	return false;
}

OneListData	*	AutoManageList::At(int index)
{
	return m_ListData[index];
}

OneListData	*	AutoManageList::Find(String item)
{
	for(int i=0; i<m_ListData.Count(); i++)
	{
		if(m_ListData[i]->m_ManageItem == item)
			return m_ListData[i];
	}
	return NULL;
}

void			AutoManageList::LoadSetting(AutoManageList * other)
{
	Clear();
	for(int i=0; i<other->Count(); i++)
	{
		Add(other->At(i)->GetManageItem(), other->At(i)->GetTag(), other->At(i)->GetManageMinCount());
	}
}


void			AutoManageList::MoveUp(int index)
{
	if(index <= 0 || index >= Count())
		return;

	OneListData temp;
	temp.ReadData(At(index-1));
	At(index-1)->ReadData(At(index));
	At(index)->ReadData(&temp);

	RefreshGUI();
}

void			AutoManageList::MoveDown(int index)
{
	if(index < 0 || index >= Count()-1)
		return;

	OneListData temp;
	temp.ReadData(At(index+1));
	At(index+1)->ReadData(At(index));
	At(index)->ReadData(&temp);

	RefreshGUI();
}

//********************************************************************************
//================================================================================


AutoManageListConfig::AutoManageListConfig()
{
	//�Զ��ӳ�
	AutoManageList * list = new AutoManageList("AutoKongfu", amAutoKongfu);
	m_ManageList.Add(list);

	//�Զ�ѡ�����
	list = new AutoManageList("Patrol", amPatrol);
	m_ManageList.Add(list);

	//�Զ�����
	list = new AutoManageList("AutoChangeLine2", amChangeLine);
	m_ManageList.Add(list);

	//�Զ�����
	list = new AutoManageList("AutoDrop", amAutoDropItem);
	m_ManageList.Add(list);

	//�Զ�����
	list = new AutoManageList("AutoUpload", amAutoUploadItem);
	m_ManageList.Add(list);

	//�Զ���/ʹ��
	list = new AutoManageList("AutoOpenOrUse", amAutoOpenBox);
	m_ManageList.Add(list);

	//�����������Ĺ�
	list = new AutoManageList("UnAttackNPC", amUnAttackNPCType);
	m_ManageList.Add(list);

	//�Զ���
	list = new AutoManageList("AutoSellItem", amAutoSell);
	m_ManageList.Add(list);
	
	//�Զ���
	list = new AutoManageList("AutoBuyItem", amAutoBuy);
	m_ManageList.Add(list);

	//�Զ�����
	list = new AutoManageList("AutoRepairItem", amAutoRepair);
	m_ManageList.Add(list);

	//���ܿ����
	list = new AutoManageList("FastSkill", amFastSkill);
	m_ManageList.Add(list);

	//�Զ���Ǯׯ
	list = new AutoManageList("AutoSaveMoneyHouse", amAutoSaveMoneyHouse);
	m_ManageList.Add(list);

	//�Զ���ݸ�XXX
	list = new AutoManageList("AutoDelivery", amAutoDelivery);
	m_ManageList.Add(list);

	//��������˵��б�
	list = new AutoManageList("AskTeamPlayer", amAskTeamPlayer);
	m_ManageList.Add(list);

	//���ܼ����˵��б�
	list = new AutoManageList("AccTeamLeader", amAccTeamLeader);
	m_ManageList.Add(list);

	//�Զ�ȡ��Ʒ
	list = new AutoManageList("AutoGetDelivery", amAutoGetDelivery);
	m_ManageList.Add(list);

	//�������Զ����ߵ����
	list = new AutoManageList("PlayerBlackList", amPlayerBlackList);
	m_ManageList.Add(list);

	//�Զ�ϰ���ϰ���б�
	list = new AutoManageList("LearnKongfuList", amLearnKongfuList);
	m_ManageList.Add(list);

	//�Զ����̵���Ʒ
	list = new AutoManageList("AutoBuyShopList", amAutoBuyShopItem, 1);
	m_ManageList.Add(list);

	//�Զ����̵���б�
	list = new AutoManageList("AutoSellShopItemList", amAutoSellShopItem);
	m_ManageList.Add(list);

	//�Զ�����
	list = new AutoManageList("AutoTeamStr", amAutoTeamStr);
	m_ManageList.Add(list);

	//�Զ��ϳ�
	list = new AutoManageList("AutoMerge", amAutoMerge);
	m_ManageList.Add(list);

	//������
	list = new AutoManageList("BlackList", amBlackList);
	m_ManageList.Add(list);

	//�Զ�������
	list = new AutoManageList("BlockString", amBlockString);
	m_ManageList.Add(list);

	//Ҫ���������
	list = new AutoManageList("DaiLianAsk", amDaiLianAsk);
	m_ManageList.Add(list);

	//���ܴ�������
	list = new AutoManageList("AccDaiLian", amAccDaiLian);
	m_ManageList.Add(list);

    //��������
    list = new AutoManageList("AutoPaoTui", amAutoPaoTui);
	m_ManageList.Add(list);

    //�����Ա�
    list = new AutoManageList("AutoSearch", amAutoSearch);
	m_ManageList.Add(list);

    //���Զ���ӵĳ�Ա�б�
    list = new AutoManageList("DoNotJoinTeam", amDoNotJoinTeam);
    m_ManageList.Add(list);
}

AutoManageListConfig::~AutoManageListConfig()
{
}

//-------------------------------------------------------------------------
AutoManageList *	AutoManageListConfig::GetDefStringList(TAutoManage am)
{
	for(int i=0; i<m_ManageList.Count(); i++)
	{
		if(m_ManageList[i]->GetIndex() == am)
			return m_ManageList[i];
	}

	return NULL;
}

//-------------------------------------------------------------------------
//��ȡini
void	AutoManageListConfig::LoadFromIni(TMemIniFile * IniFile,const String &SecName)
{

	for(int i=0; i<m_ManageList.Count(); i++)
	{
		m_ManageList[i]->LoadFromIni(IniFile, SecName);
	}

}

//---------------------------------------------------------------------------
//�õ��б��е����֣����ڱ���ini
void	AutoManageListConfig::SaveToIni(TMemIniFile * IniFile, String RecordName)
{
	for(int i=0; i<m_ManageList.Count(); i++)
	{
		m_ManageList[i]->SaveToIni(IniFile, RecordName);
	}

}

//---------------------------------------------------------------------------

//============================================================================
__fastcall HEConfig::HEConfig()
{
	ChannelColor[0].channelColor = clMaroon;
	ChannelColor[1].channelColor = clBlack;
	ChannelColor[2].channelColor =clGreen;
	ChannelColor[3].channelColor =clPurple;
	ChannelColor[4].channelColor =clRed;
	ChannelColor[5].channelColor =clTeal;
	ChannelColor[6].channelColor =clFuchsia;
	ChannelColor[7].channelColor =clMaroon;
	ChannelColor[8].channelColor =clNavy;
	ChannelColor[9].channelColor =clLime;
}
//---------------------------------------------------------------------------
__fastcall HEConfig::~HEConfig()
{
}
//---------------------------------------------------------------------------
bool    HEConfig::LoadFromIni(TMemIniFile * IniFile,const String &SecName)
{
	TConfig::LoadFromIni(IniFile,SecName);

	for (int i = 0; i <  MAX_CHANNEL; i++)
	{
		ChannelColor[i].channelColor = (TColor)IniFile->ReadInteger(SecName,FormatStr("Channel_%d",i),ChannelColor[i].channelColor);
	}

	for (int i = 0; i <  MAX_CHANNEL; i++)
	{
		ChannelColor[i].show = (bool)IniFile->ReadInteger(SecName,FormatStr("ShowChannel_%d",i),ChannelColor[i].show);
	}
	
	m_AutoManageListConfig.LoadFromIni(IniFile, SecName);
	return true;
}

//---------------------------------------------------------------------------
bool    HEConfig::LoadFromFile(const String &FileName,const String &SecName)
{
	if (!FileExists(FileName))
	{
		return false;
	}

   auto_ptr<TMemIniFile> IniFile(new TMemIniFile(FileName));
   return LoadFromIni(IniFile.get(),SecName);
}

//---------------------------------------------------------------------------
void    HEConfig::SaveToFile(const String &FileName)
{
	auto_ptr<TMemIniFile> IniFile(new TMemIniFile(FileName));
	SaveToIni(IniFile.get());
}
//---------------------------------------------------------------------------
void    HEConfig::SaveToIni(TMemIniFile * IniFile)
{
	TConfig::SaveToIni(IniFile);

	for (int i = 0; i <  MAX_CHANNEL; i++)
	{
	   IniFile->WriteInteger(this->GetRecordName(),FormatStr("Channel_%d",i),(int)ChannelColor[i].channelColor);
	}
	for (int i = 0; i <  MAX_CHANNEL; i++)
	{
	   IniFile->WriteInteger(this->GetRecordName(),FormatStr("ShowChannel_%d",i),(int)ChannelColor[i].show);
	}
	
	m_AutoManageListConfig.SaveToIni(IniFile, this->GetRecordName());
	IniFile->UpdateFile();

}



AutoManageList *	HEConfig::GetDefStringList(TAutoManage am)
{
	return m_AutoManageListConfig.GetDefStringList(am);
}
//
//---------------------------------------------------------------------------
bool    HEConfig::IsListExist(TAutoManage am, DWORD ID)
{
	AutoManageList * autoList = m_AutoManageListConfig.GetDefStringList(am); 
	return autoList->IsExist(ID);
}

tagChatConfig	HEConfig::GetDefColor(int	ct)
{
	if(ct > 1 && ct < MAX_CHANNEL)
		return	ChannelColor[ct-1];
	tagChatConfig defConfig;
	return	defConfig;
}

void	HEConfig::SetDefColor(int ct, tagChatConfig config)
{
	if(ct > 1 && ct < MAX_CHANNEL)
		ChannelColor[ct-1] = config;
}

void	HEConfig::SetDefColor(int ct, TColor color, bool show)
{
	tagChatConfig curColor;
	curColor.channelColor = color;
	curColor.show = show;

	SetDefColor(ct, curColor);
}
