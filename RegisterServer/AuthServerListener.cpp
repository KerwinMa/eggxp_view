//---------------------------------------------------------------------------


#pragma hdrstop

#include "AuthServerListener.h"
#include "ShareDef.h"
#include "ClientServerShared.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

#define		RECV_BUFF			1024
#define		SOCKET_TIMEOUT		2 * 60 * 1000

int			GetAllFreeCardCount(TADOQuery *query)
{
	query->SQL->Clear();
	query->SQL->Add("SELECT count(*) as cnt FROM wow_card where is_card_send = 0");
	if(!OpenSQL(query))
		return 0;
	int total = query->FieldByName("cnt")->AsInteger;
	return	total;
}

String			GetFileNameByDate(String name, String ext)
{
	String curTime = Now();
    TReplaceFlags   flag;
    flag << rfReplaceAll;
	curTime = StringReplace(curTime, "/", "_", flag);
	curTime = StringReplace(curTime, ":", "_", flag);
	curTime = StringReplace(curTime, " ", "_", flag);
	return	FormatStr("%s_%s.%s", name, curTime, ext);
}

bool			GenerateCard(TADOQuery *query, String cardType, int cnt)
{
	query->SQL->Clear();

	for(int i=0; i<cnt; i++)
	{
		query->SQL->Clear();
		query->SQL->Add(FormatStr("INSERT wow_card (card_no, card_type, is_card_send, create_time) VALUES ('%s', '%s', %d, '%s')",
				CreateCardNo(cardType), cardType, 0, DateTimeToStr(Now())) );
		if(!ExecSQL(query))
		{
			return false;
		}
	}
	return true;
}

bool			GetExpireTime(TADOQuery *query, String username, vector<String> *outputMsg, TDateTime *result)
{
	outputMsg->clear();
	query->SQL->Text = FormatStr("SELECT end_time FROM WOW_USER WHERE ACCID = '%s'", username);
	if(!OpenSQL(query))
	{
		outputMsg->push_back(GBText("����(15)"));
		return false;
	}
	query->First();
	TDateTime endTime = query->FieldByName("end_time")->AsDateTime;
	if(result)
	{
		*result = endTime;
	}
	return true;
}

int			GetMoneyByCardType(String cardType)
{
	tagCardInfo	*  info = GetAuthServerListener()->GetCardInfo(cardType);
	if(!info)
		return 0;
	return  info->CardMoney;
}

int			GetDaysByCardType(String cardType)
{
	tagCardInfo	*  info = GetAuthServerListener()->GetCardInfo(cardType);
	if(!info)
		return 0;
	return  info->CardDay;
}

String		GetCardEndDataFromNow(String cardType)
{
	tagCardInfo	*  info = GetAuthServerListener()->GetCardInfo(cardType);
	if(!info)
		return "";
	return  Now() + info->CardDay * 24 * 60 * 60;
}

String		GetCardNameByCardType(String cardType)
{
	tagCardInfo	*  info = GetAuthServerListener()->GetCardInfo(cardType);
	if(!info)
		return "";
	return	info->CardName;
}

String		CreateCardNo(String cardType)
{
	_GUID	guid;
	CoCreateGuid(&guid);
	String result = GUIDToString(guid);
	if(result.Length() < 3)
	{
		return "?NEVER BE HERE!?";
	}
	cardType = cardType.UpperCase();
	return FormatStr("%s_%s", cardType, String(result.c_str()+1, result.Length()-2));
}


bool			OpenSQL(TADOQuery *query)
{
	try
	{
		query->Open();
	} catch (Exception &e)
	{
		GetLog()->Error("OpenSQL Error :SQL=\r\n%s\r\nError=%s", query->SQL->Text, e.Message);
		return false;
	}
	return true;
}

bool			ExecSQL(TADOQuery *query)
{
	try
	{
		query->ExecSQL();
	} catch (Exception &e)
	{
		GetLog()->Error("ExecSQL Error :SQL\r\n%s\r\nError=%s", query->SQL->Text, e.Message);
		return false;
	}
	return true;
}

bool        GetCard(TADOQuery *query, String cardType, String *outputCardNo, vector<String> *outputMsg)
{
	query->SQL->Text = FormatStr("UPDATE top (1) wow_card SET is_card_send = 1 , send_time = '%s' output  INSERTED.card_no, INSERTED.card_type WHERE card_type = '%s' and is_card_send = 0",
		DateTimeToStr(Now()), cardType);
	if(!OpenSQL(query))
	{
		outputMsg->push_back(GBText("����(8)"));
		return false;
	}
	if(!query->RecordCount)
		return false;
	query->First();
	if(outputCardNo)
	{
		*outputCardNo = query->FieldByName("card_no")->AsString;
	}
	return true;
}

bool		Charge(TADOQuery *query, String username, String card_no, vector<String> *outputMsg)
{
	query->SQL->Clear();
	query->SQL->Text = FormatStr("SELECT * FROM wow_card WHERE card_no = '%s'", card_no);
	if(!OpenSQL(query))
	{
		outputMsg->push_back(GBText("����(11)"));
		return false;
	}
	if(query->RecordCount == 0)
	{
		outputMsg->push_back(GBText("��ֵ�����Ŵ���"));
		GetLog()->Warn("Charge Fail! Can't Find Card %s", card_no);
		return false;
	}
	query->First();
	String cardType = query->FieldByName("card_type")->AsString;

	query->SQL->Text = FormatStr("SELECT * FROM used_wow_card WHERE card_no = '%s'", card_no);
	if(!OpenSQL(query))
	{
		outputMsg->push_back(GBText("����(16)"));
		return false;
	}
	if(query->RecordCount != 0)
	{
		outputMsg->push_back(GBText("��ֵ���Ѿ����ù�"));
		GetLog()->Warn("Charge Fail! Card Used %s", card_no);
		return false;
	}

	query->SQL->Text = FormatStr("INSERT used_wow_card (card_no, accid, card_type, use_time) VALUES('%s', '%s', '%s', '%s')",
			card_no, username, cardType, DateTimeToStr(Now()));
	if(!ExecSQL(query))
	{
		outputMsg->push_back(GBText("����(12)"));
		return false;
	}

	int cardDay = GetDaysByCardType(cardType);
	int cardMoney = GetMoneyByCardType(cardType);
	int money_time = cardDay;
	if(cardMoney == 0)
	{
		money_time = 0;
	}
	TDateTime expTime;
	if(!GetExpireTime(query, username, outputMsg, &expTime))
	{
		return false;
	}
	if (expTime < Now())
	{
		expTime = Now();
	}
	query->SQL->Text = FormatStr("UPDATE wow_user SET end_time = CAST('%s' AS DateTime) + %d, total_time = total_time + %d, "
									  "money_total_time = money_total_time + %d, money = money + %d where accid = '%s'",
									  DateTimeToStr(expTime), cardDay, cardDay, money_time, cardMoney, username);
	if(!ExecSQL(query))
	{
		outputMsg->push_back(GBText("����(13)"));
		return false;
	}

	outputMsg->push_back(FormatStr(GBText("�ѳ����ֵ��:%s"), GetCardNameByCardType(cardType)));
	return	true;
}


////////////////////////////////////////////////////////////////////////

AuthClientUser::AuthClientUser()
{
	m_Build = 0;
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	m_ADOConnection = NULL;
	m_ADOQuery = NULL;
}

AuthClientUser::~AuthClientUser()
{
	SAFE_DELETE(m_ADOConnection)
	SAFE_DELETE(m_ADOQuery)
    CoUninitialize();
}

void 			AuthClientUser::SocketReadError(String msg)
{
	Sleep(1000);
	this->SetIsClosed(1);
}

bool			AuthClientUser::OnUserFirstLogin(vector<String> *outputMsg)
{
	//1. ȡһ����ѿ�
	vector<tagCardInfo>	freeCardList;
	for(int i=0; i<GetAuthServerListener()->GetCardInfoCount(); i++)
	{
		tagCardInfo *info = GetAuthServerListener()->GetCardInfoByIndex(i);
		if(!info)
			continue;
		if(info->CardMoney != 0)
			continue;
		freeCardList.push_back(*info);
	}
	sort(freeCardList.begin(), freeCardList.end(),greater<tagCardInfo>());

	String cardType, cardNo;
	for(DWORD i=0; i<freeCardList.size(); i++)
	{
		if(GetCard(m_ADOQuery, freeCardList[i].CardType, &cardNo, outputMsg))
		{
			cardType = freeCardList[i].CardType;
			break;
		}
	}

	if(cardNo == "")
	{
		if(m_IsChargeProcess == 0)
		{
			//��ѿ��Ѿ��������
			outputMsg->push_back(GBText("��ѿ��Ѿ��������, ��ע��!"));
			SendRegMessage();
			return false;
		}
		else
		{
			outputMsg->push_back(GBText("�Բ���, ��ѿ��Ѿ��������!"));
		}
	}

	//2. �����û�
	String nowStr = DateTimeToStr(Now());
	m_ADOQuery->SQL->Text = FormatStr("INSERT wow_user ("
									  "accid, 		last_login_time, 	login_count, 	start_time, 	end_time, "
									  "total_time, 	money_total_time, 	money, 			login_ip, 		create_time, "
									  "create_ip) VALUES("
									  "'%s', 		'%s', 				%d, 			'%s', 			'%s',"
									  "%d, 			%d, 				%d, 			'%s', 			'%s',"
									  "'%s'"
									  ")",
									  m_UserName,   nowStr,				0,              nowStr,         nowStr,
									  0,			0,                  0,              m_ClientIP,     nowStr,
									  m_ClientIP
									  );

	if(!ExecSQL(m_ADOQuery))
	{
		outputMsg->push_back(GBText("����(9)"));
		return false;
	}

	m_ADOQuery->SQL->Text = FormatStr("SELECT * FROM wow_msg where msg_key = '%s'", L"first_login");
	if(!OpenSQL(m_ADOQuery))
	{
		outputMsg->push_back(GBText("����(10)"));
		return false;
	}
	if(m_ADOQuery->RecordCount)
	{
		m_ADOQuery->First();
		String	text = m_ADOQuery->FieldByName("msg")->AsString;
		outputMsg->push_back(text);
	}

	//3. ��ֵ
	if(cardNo != "")
	{
		if(!Charge(m_ADOQuery, m_UserName, cardNo, outputMsg))
		{
			return false;
		}
	}
	return true;
}

bool			AuthClientUser::CheckUserCanLogin(vector<String> *outputMsg)
{
 	m_ADOConnection = new TADOConnection(NULL);
	m_ADOQuery = new TADOQuery(NULL);
	m_ADOQuery->Connection = m_ADOConnection;

	int tryCnt = 0;
	String m_ConnectString;
    do
    {
		try
		{
			if(tryCnt > 5)
				break;
			tryCnt++;
			m_ADOConnection->LoginPrompt = false;
			m_ADOConnection->KeepConnection = true;
			m_ADOConnection->ConnectionTimeout = SQL_TIMEOUT;
			m_ADOConnection->ConnectionString= FormatStr("Provider=SQLOLEDB.1;Persist Security Info=True;User ID=%s;Password=%s;Initial Catalog=wowwg;Data Source=%s,%d",
				GetAuthServerListener()->GetDatabaseUserID(), GetAuthServerListener()->GetDatabaseUserPsw(), GetAuthServerListener()->GetDatabaseIP(), GetAuthServerListener()->GetDatabasePort());
			m_ADOConnection->Connected=true;
			Sleep(5);
		}
		catch (Exception &e)
		{
			m_RefuseLoginReason = FormatStr("SQL Error:%s", e.Message);
        }
    }
	while(m_ADOConnection->Connected == false);
	if(m_ADOConnection->Connected == false)
	{
		outputMsg->push_back(GBText("����(6)"));
		return false;
	}

	m_ADOQuery->SQL->Text = FormatStr("SELECT * FROM WOW_USER WHERE ACCID = '%s'", m_UserName);
	if(!OpenSQL(m_ADOQuery))
	{
		outputMsg->push_back(GBText("����(7)"));
		return false;
	}
	if(m_ADOQuery->RecordCount == 0)
	{
		if(!OnUserFirstLogin(outputMsg))
		{
			return false;
		}
	}
	else
	{
		//��֤�û��Ƿ���
		if(!m_IsChargeProcess)
		{
			m_ADOQuery->First();
			if(Now() > m_ADOQuery->FieldByName("end_time")->AsDateTime)
			{
				outputMsg->push_back(GBText("���ʱ���ѵ���, ��ע��!"));
				SendRegMessage();
				return false;
			}
		}
	}
	return true;
}

bool			AuthClientUser::SendRegMessage()
{
	if(!this->SocketSendBYTE(PT_S_REQ_CHARGE))
		return false;
	return	true;
}

void			AuthClientUser::OnLoginSuccess()
{
	m_ADOQuery->SQL->Text = FormatStr("UPDATE wow_user SET last_login_time='%s', login_count=login_count+1, login_ip='%s' where accid = '%s'",
			DateTimeToStr(Now()), m_ClientIP, m_UserName);
	ExecSQL(m_ADOQuery);
}

int         	AuthClientUser::ClientProcessThread(SingleThread * self)
{
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	int   timeout = SOCKET_TIMEOUT;
	setsockopt(m_ClientSocket,   SOL_SOCKET,   SO_RCVTIMEO,(char*)&timeout,   sizeof(timeout));

	vector<BYTE>	output;
	//1. Auth
	if(!this->SocketReadBuff(sizeof(tagCClientAuth), &output))
	{
		SocketReadError("tagCClientAuth SocketReadBuff Error");
		return -1;
	}
	tagCClientAuth	*clietAuth = (tagCClientAuth	*)&output[0];
	clietAuth->UserName[sizeof(clietAuth->UserName)-1] = '\0';
	m_UserName = clietAuth->UserName;
	m_UserName = m_UserName.LowerCase();
	m_UserName = m_UserName.Trim();
	string input_username = clietAuth->UserName;
	SendLogMessage(FormatStr(GBText("�û�[%s]��ʼ��֤..."), m_UserName));
	m_Build = clietAuth->Build;
	BigNumber A, s, B, K, M2;
	A.SetBinary(clietAuth->A, sizeof(clietAuth->A));
	m_IsChargeProcess = clietAuth->IsChargeProcess;
	if(!m_SRP6Server.CalcKey(input_username, input_username, A, &s, &B, &K))
	{
		SendLogMessage("����(1)");
		SocketReadError("CalcKey Error");
		return -1;
	}

	tagSClientAuth	sendAuth;
	memcpy(sendAuth.s, s.AsByteArray(), sizeof(sendAuth.s));
	memcpy(sendAuth.B, B.AsByteArray(), sizeof(sendAuth.B));
	if(!this->SendAuthMessage((BYTE *)&sendAuth, sizeof(sendAuth)))
	{
		SocketReadError("tagSClientAuth SocketSend Error");
		return -1;
	}
	//2. Auth Check
	if(!this->SocketReadBuff(sizeof(tagCAuthCheck), &output))
	{
		SocketReadError("tagCAuthCheck SocketReadBuff Error");
		return -1;
	}
	tagCAuthCheck *clientAuthCheck = (tagCAuthCheck *)&output[0];
	if(!m_SRP6Server.CheckClientAuth(input_username, clientAuthCheck->M, &M2))
	{
		SendLogMessage("����(2)");
		SocketReadError("Auth Check Error");
		return -1;
	}
	tagSAuthCheck	sendAuthCheck;
	memcpy(sendAuthCheck.M2, M2.AsByteArray(), sizeof(sendAuthCheck.M2));
	if(!this->SendAuthMessage((BYTE *)&sendAuthCheck, sizeof(sendAuthCheck)))
	{
		SocketReadError("tagSAuthCheck SocketSend Error");
		return -1;
	}
	m_AuthCrypt.Init(&K, WOWWGUserServerEncryptionKey, WOWWGUserServerDecryptionKey);

	vector<String> outputMsg;
	if(!CheckUserCanLogin(&outputMsg))
	{
		for(DWORD i=0; i<outputMsg.size(); i++)
		{
			SendLogMessage(outputMsg[i]);
		}
		SocketReadError(FormatStr("[RefuseLogin]:%s", m_RefuseLoginReason));
		return -1;
	}
	for(DWORD i=0; i<outputMsg.size(); i++)
	{
		SendLogMessage(outputMsg[i]);
	}
//	SendLogMessage(FormatStr(GBText("�û�[%s]��֤�ɹ�!"), m_UserName));

	TDateTime expTime;
	if(m_IsChargeProcess)
	{
		//��ֵ
		BYTE len = 0;
		if(!this->SocketReadBYTE(&len))
		{
			SendLogMessage("����(17)");
			SocketReadError("Charge CardNo Read Len Error");
			return -1;
		}
		if(!this->SocketReadBuff(len, &output))
		{
			SendLogMessage("����(14)");
			SocketReadError("Charge CardNo Read Error");
			return -1;
		}
		AnsiString cardNo = AnsiString((char *)&output[0], len);
		cardNo = cardNo.Trim();
		cardNo = cardNo.UpperCase();
		outputMsg.clear();
		if(!Charge(m_ADOQuery, m_UserName, cardNo, &outputMsg))
		{
			for(DWORD i=0; i<outputMsg.size(); i++)
			{
				SendLogMessage(outputMsg[i]);
			}
			SocketReadError("Charge Fail!");
			return -1;
		}

		for(DWORD i=0; i<outputMsg.size(); i++)
		{
			SendLogMessage(outputMsg[i]);
		}

		outputMsg.clear();
		if(!GetExpireTime(m_ADOQuery, m_UserName, &outputMsg, &expTime))
		{
			for(DWORD i=0; i<outputMsg.size(); i++)
			{
				SendLogMessage(outputMsg[i]);
			}
			SocketReadError("Charge GetExpireTime Fail!");
			return -1;
		}
		for(DWORD i=0; i<outputMsg.size(); i++)
		{
			SendLogMessage(outputMsg[i]);
		}

		SendLogMessage(GBText("��ֵ�ɹ�!"));
	}
	else
	{
		//��֤
		tagWOWAddrInfo	addrInfo;
		if(!FindAddrInfo(m_Build, &addrInfo))
		{
			SendLogMessage("����(3)");
			GetLog()->Error("No Build Msg. Build = %d", m_Build);
			SocketReadError("We Don't Have This Build!");
			return -1;
		}

		outputMsg.clear();
		if(!GetExpireTime(m_ADOQuery, m_UserName, &outputMsg, &expTime))
		{
			for(DWORD i=0; i<outputMsg.size(); i++)
			{
				SendLogMessage(outputMsg[i]);
			}
			SocketReadError("GetExpireTime Fail!");
			return -1;
		}

		for(DWORD i=0; i<outputMsg.size(); i++)
		{
			SendLogMessage(outputMsg[i]);
		}

		tagClientAddrInfo	info;
		info.Address = addrInfo.Addr;
		info.Offset = addrInfo.Offset;
		info.ExpireTime = expTime;
		if(!this->SendAuthMessage((BYTE *)&info, sizeof(tagClientAddrInfo)))
		{
			SocketReadError("tagClientAddrInfo SocketSend Error");
			return -1;
		}
		m_ADOQuery->SQL->Text = FormatStr("SELECT * FROM wow_msg where msg_key = '%s'", L"login");
		if(OpenSQL(m_ADOQuery))
		{
			if(m_ADOQuery->RecordCount)
			{
				m_ADOQuery->First();
				String	text = m_ADOQuery->FieldByName("msg")->AsString;
				SendLogMessage(text);
			}
		}
		SendLogMessage(GBText("��֤�ɹ�!"));
	}

	SendLogMessage(GBText(FormatStr("����ʺ�[%s]����ʱ��Ϊ:%s", m_UserName, DateTimeToStr(expTime))));
	OnLoginSuccess();
	Sleep(1000);
	this->SetIsClosed(1);
	return -1;
}



void			AuthClientUser::DecryptRecv(BYTE *lpdata, int len)
{
	m_AuthCrypt.ClientToServerDecrypt(lpdata, len);
}

void  			AuthClientUser::EncryptSend(BYTE *lpdata, int len)
{
	m_AuthCrypt.ServerToClientEncrypt(lpdata, len);
}

bool			AuthClientUser::SendLogMessage(AnsiString msg)
{
	String warnMsg = FormatStr("%s(%s) : %s", m_UserName, m_ClientIP, String(msg));
	GetLog()->Warn(warnMsg);
	GetThreadManager()->AddGUIMessage(warnMsg);

	AnsiString sendMsg = msg;
	sendMsg.Unique();
	if(!this->SocketSendBYTE(PT_MSG))
		return false;

	if(!this->SocketSendWORD(sendMsg.Length()))
		return false;

	if(!this->SocketSendBuff(sendMsg.c_str(), sendMsg.Length()))
		return false;

	if(m_ADOQuery)
	{
		m_ADOQuery->SQL->Text = FormatStr("INSERT user_error_msg (accid, cur_time, build, ip, msg) VALUES ('%s', '%s', %d, '%s', '%s')",
										m_UserName, DateTimeToStr(Now()), m_Build, m_ClientIP, String(msg));
		ExecSQL(m_ADOQuery);
	}
//	Sleep(1000);
	return true;
}

bool			AuthClientUser::SendAuthMessage(BYTE *lpdate, int len)
{
	if(!this->SocketSendBYTE(PT_AUTH))
		return false;

	if(!this->SocketSendBuff(lpdate, len))
		return false;

	return true;
}

void            AuthClientUser::Start(SOCKET client, SOCKADDR_IN clientAddr)
{
	SocketClientUser::Init(client, clientAddr);
	GetThreadManager()->ManagerCreateThread("ClientProcessThread", ClientProcessThread, false);
}

//-----------------------------------------------------------------------------
AuthServerListener::AuthServerListener()
{
	InitializeCriticalSection(&m_csLock);
	WSADATA wsaData;
    WSAStartup(MAKEWORD(2,0),&wsaData);
    m_ListenPort = 0;
    m_ListenSocket = 0;
	m_ListenThreadCount = 0;
	m_DatabasePort = 0;
}

AuthServerListener::~AuthServerListener()
{
	WSACleanup();
	DeleteCriticalSection(&m_csLock);
}

void			AuthServerListener::ProcessCloseClients()
{
    vector<SOCKET> delSocket;
	for(int i=0; i<m_Users.Count(); i++)
	{
		if(m_Users.At(i)->GetIsClosed())
		{
			delSocket.push_back(m_Users.At(i)->GetClientSocket());
		}
	}
	for(DWORD i=0; i<delSocket.size(); i++)
	{
		CloseConnect(delSocket[i]);
	}
}

void            AuthServerListener::CloseConnect(SOCKET socket)
{
    closesocket(socket);
    m_Users.Delete(socket);
}

int             AuthServerListener::ListenThread(SingleThread * self)
{
    //////////////////////////////////////////////////////////////////////////
    //��ʼ���м���
    //////////////////////////////////////////////////////////////////////////
    SOCKADDR_IN clientsocket;
    int len=sizeof(SOCKADDR);
	GetLog()->Warn("Listen Port : %d", m_ListenPort);
	while (1)
	{
        SOCKET serConn=accept(m_ListenSocket,(SOCKADDR*)&clientsocket,&len);//������ﲻ��accept����conection�Ļ������ͻ᲻�ϵļ���
        if(serConn == (SOCKET)SOCKET_ERROR)
        {
            return  -1;
        }

        if(m_Users.Find(serConn))
        {
            GetLog()->Error("SOCKET ALREADY EXIST! : %d", serConn);
            this->CloseConnect(serConn);
            continue;
        }

        AuthClientUser * curUser = new AuthClientUser();
        m_Users.Add(serConn, curUser);
        curUser->Start(serConn, clientsocket);
    }

}

bool            AuthServerListener::Start(int listenPort, int listenThreadCount)
{
    m_ListenSocket = socket(AF_INET,SOCK_STREAM,0);//�����˿�ʶ���׽���
    m_ListenPort = listenPort;
    m_ListenThreadCount = listenThreadCount;
    
    //��Ҫ�󶨵Ĳ���
    SOCKADDR_IN addr;
    addr.sin_family=AF_INET;
    addr.sin_addr.S_un.S_addr=htonl(INADDR_ANY);//ip��ַ
    addr.sin_port=htons(m_ListenPort);//�󶨶˿�
    if(bind(m_ListenSocket,(SOCKADDR*)&addr,sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
        GetLog()->Warn("Bind Error:%s", SysErrorMessage(WSAGetLastError()));
        return false;
    }
    //���еڶ������������ܹ����յ�����������
    if(listen(m_ListenSocket,m_ListenThreadCount) == SOCKET_ERROR)
    {
        GetLog()->Warn("Listen Error:%s", SysErrorMessage(WSAGetLastError()));
        return false;
    }
    
	GetThreadManager()->ManagerCreateThread("ListenThread", ListenThread, false);
	return true;
}

void            AuthServerListener::Close()
{
    closesocket(m_ListenSocket);
}

tagCardInfo	*	AuthServerListener::GetCardInfo(String key)
{
	if(m_CardTypes.find(key) == m_CardTypes.end())
		return NULL;

	return	&m_CardTypes[key];
}

void			AuthServerListener::AddCardInfo(String key, int day, int money, String name)
{
	m_CardTypes[key].CardType = key;
	m_CardTypes[key].CardDay = day;
	m_CardTypes[key].CardMoney = money;
	m_CardTypes[key].CardName = name;
}

tagCardInfo	*   AuthServerListener::GetCardInfoByIndex(int index)
{
	if(index < 0)
		return NULL;
	map<String, tagCardInfo>::iterator it = m_CardTypes.begin();
	for(int i=0; i<index; i++, it++)
	{
		if(it==m_CardTypes.end())
		{
			return NULL;
		}
	}
	return	&it->second;
}

int				AuthServerListener::GetCardInfoCount()
{
	return	m_CardTypes.size();
}
//-----------------------------------------------------------------------------

AuthServerListener  gAuthServerListener;
AuthServerListener * GetAuthServerListener()
{
	return  &gAuthServerListener;
}
