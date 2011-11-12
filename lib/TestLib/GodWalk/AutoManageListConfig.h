//---------------------------------------------------------------------------

#ifndef AutoManageListConfigH
#define AutoManageListConfigH
//---------------------------------------------------------------------------
#include <vcl.h>
#include <ValEdit.hpp>
#include "HEcommon.h"
#include "ShareDefine.h"
//������б�֮��Ҫ�ģ� AutoManageListConfig���캯����  setup�����е� tag 
enum	TAutoManage {amAutoKongfu, amPatrol, amChangeLine, amAutoDropItem, amAutoUploadItem, amAutoOpenBox,
					amUnAttackNPCType, amAutoSell, amAutoBuy, amAutoRepair, amFastSkill, amAutoSaveMoneyHouse,
					amAutoDelivery, amAskTeamPlayer, amAccTeamLeader, amAutoGetDelivery, amPlayerBlackList,
					amLearnKongfuList, amAutoBuyShopItem, amAutoSellShopItem, amAutoTeamStr, amAutoMerge,
					amBlackList, amBlockString, amDaiLianAsk, amAccDaiLian, amAutoPaoTui,
                    amAutoSearch, amDoNotJoinTeam
					};

class AutoManageListConfig;


//һ���б�����
class AutoManageList;
class	OneListData
{
private:
	String		m_ManageItem;
	String	   	m_Tag;
	//�����Զ������б�����: 0-һֱ���� 1-��·ʱ���� 2-ս��ʱ����
	String		m_ManageMinCount;			//Ԥ��N��
	
friend AutoManageList;
protected:
	void		LoadSaveString(String iniStr);
	String 		OutPutString();
	void		Init(String item, int tag, int count);
	void		InitStr(String item, String tag, String count);
	void		ReadData(OneListData * other);	

public:
	//�����Զ������б�����
	//m_ManageItem: ʹ�ü���      m_Tag:���       m_ManageMinCount:ʹ�÷�ʽ
	String		GetManageItem(){return	m_ManageItem;}
	int			GetManageItemInt(){return	m_ManageItem.ToIntDef(0);}
	int			GetTag(){return	m_Tag.ToIntDef(0);}
	int			GetManageMinCount(){return	m_ManageMinCount.ToIntDef(0);}
	String		GetTagStr(){return	m_Tag;}
	String	  	GetManageMinCountStr(){return	m_ManageMinCount;}

	int			GetManageMinCountHi();
	int			GetManageMinCountLo();

	int			GetTagHi();
	int			GetTagLo();

public:
	OneListData()
	{
	}
	~OneListData(){}

};

class	AutoManageList
{
private:
	TStringList * 			m_StringList;
	AList<OneListData>		m_ListData;
	TListItems * 			m_ListGui;
	TValueListEditor * 		m_ValueListGUI;
	String					m_IndexName;
	TAutoManage				m_Index;

	String					MakeListToString(TStringList * destList);

	friend 					AutoManageListConfig;

	String					GetTagString(String tag);
	String					GetCountString(int count);
	
	bool					m_bItemLoaded;
	int						m_RefreshType;

	String					GetDataGuiCaption(OneListData * data);
	String					GetDataGuiText(OneListData * data);
	String					GetDataGuiText2(OneListData * data);
	void					AddGuiItem(OneListData * data);
	bool					IsItem();		//�������б�����ǲ�����Ʒ
	
protected:
	//��ȡini
	void			LoadFromIni(TMemIniFile * IniFile,const String &SecName);

	//����ini
	void			SaveToIni(TMemIniFile * IniFile, String RecordName);

	void			EditModifyView(int index, OneListData * curData);
public:
	//refreshType : 0 ��ͨ  1 4��,��Ҫ��������
	AutoManageList(String indexName, TAutoManage am, int refreshType=0);
	~AutoManageList();
	//�󶨽���
	void			Bind(TListItems * GuiList);
	void			Bind(TValueListEditor * valueListEditor);
	void			RefreshGUI();
	void			Delete(int index);
	void			Clear();
	void			Add(String item, int tag=0, int count=0);
	void			Add(String item, WORD tagHi, WORD tagLo, int count);
	void			AddStr(String item, String tag="", String count="");
	void			Edit(int index, String item, int tag=0, int count=0);
    void			Edit(int index, String item, WORD tagHi, WORD tagLo, int count);
	void			EditStr(int index, String item, String tag="", String count="");
	bool			IsExist(String item);
	OneListData	*	At(int index);
	OneListData	*	Find(String item);
	int				IndexOf(String item);
	int				Count(){return m_ListData.Count();}
	TAutoManage		GetIndex(){return m_Index;}
	void			LoadSetting(AutoManageList * other);

	void			MoveUp(int index);
	void			MoveDown(int index);
};

class AutoManageListConfig
{
private:
	AList<AutoManageList>	m_ManageList;

public:
	AutoManageListConfig();
	~AutoManageListConfig();

	//��ȡini
	void			LoadFromIni(TMemIniFile * IniFile,const String &SecName);
	//����ini
	void			SaveToIni(TMemIniFile * IniFile, String RecordName);

	AutoManageList *	GetDefStringList(TAutoManage am);
};

//================================================================================
#define			MAX_CHANNEL			10

struct	tagChatConfig
{
	TColor	channelColor;
	bool	show;
	tagChatConfig()
	{
		channelColor = clBlack;
		show = true;
	}
};

class HEConfig : public TConfig
{
private:
	AutoManageListConfig		m_AutoManageListConfig;
	tagChatConfig  ChannelColor[MAX_CHANNEL];
	vector<tagPosition>			m_PatrolList;
public:
	__fastcall  HEConfig();
	__fastcall  ~HEConfig();
	bool    LoadFromIni(TMemIniFile * IniFile,const String &SecName);
	bool    LoadFromFile(const String &FileName,const String &SecName);
	
	void    SaveToIni(TMemIniFile * IniFile);
	void    SaveToFile(const String &FileName);


	AutoManageList *	GetDefStringList(TAutoManage am);
	tagChatConfig	GetDefColor(int	ct);
	void			SetDefColor(int ct, tagChatConfig config);
	void			SetDefColor(int ct, TColor color, bool show);
	
	bool    IsListExist(TAutoManage am, DWORD ID);
};

#endif
