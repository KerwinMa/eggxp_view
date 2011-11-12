//---------------------------------------------------------------------------

#ifndef MessageContainerH
#define MessageContainerH
#include "AList.h"
#include <VCL.h>
#include <ComCtrls.hpp>

//---------------------------------------------------------------------------

enum	IChannelType
{
ctCmd, ctSys, ctGameSys, ctBattle,
//��Ƶ, ��Ƶ, ��Ƶ, ��Ƶ, ��Ƶ, ��Ƶ,
ctPublic, ctCur, ctGroup, ctFriend, ctTeam, ctPlayer,
ctGM,ctSysAnswer//�Զ������¼
};

String	GetChannelName(IChannelType ct);


struct TInfoRecord
{
  String LogTime;  //ʱ��
  String Info;    //��Ϣ
  String Sender;  //������
  int    Channel; //��ϢƵ��
  String	ChannelName()
  {
  	return	GetChannelName((IChannelType)Channel);
  }
};

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

#define			MAX_CHANNEL			10
#define         MAX_LINE_COUNT      10000

class	MessageContainer
{
private:
	AList<TInfoRecord>		m_Recs;
	TListView		*m_View;
	int				m_MsgType;

    tagChatConfig   m_ChannelColor[MAX_CHANNEL];
    tagChatConfig  *GetDefColor(int index);

	bool			Refresh();
    bool            IsNeedAutoScroll();


	void __fastcall m_ViewCustomDrawItem(TCustomListView *Sender,
			TListItem *Item, TCustomDrawState State, bool &DefaultDraw);
	void __fastcall m_ViewCustomDrawSubItem(TCustomListView *Sender,
			TListItem *Item, int SubItem, TCustomDrawState State,
			bool &DefaultDraw);

	void __fastcall m_ViewData(TObject *Sender, TListItem *Item);

    void            RefreshCurItemText(TListItem *Item, TInfoRecord *InfoRecord);

public:
	MessageContainer();   //0:ϵͳ(4��) 1:����(2��)
	~MessageContainer();
	void			Init(int msgType);
	void			BindControl(TListView * lvView);
	void			Add(int channel, String sender, String info);
	TInfoRecord	*	At(int index){return	m_Recs[index];}
	void			Clear();
	void			SaveToFile(String fileName);
};



#endif
