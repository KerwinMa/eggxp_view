//---------------------------------------------------------------------------
#pragma hdrstop
#include <memory>
using namespace std;
#include "CommFunc.h"
#include "scriptrun.h"
#include "Scriptstack.h"
#include "pasparse.h"
#include "cppparse.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)

char    *JumpOP[] = { "JBE", "JAE", "JA", "JB", "JE", "JNE", "JMP", 0 };

//---------------------------------------------------------------------------

//�ű�����������
__fastcall WY_ScriptEngine::WY_ScriptEngine(String RootPath)
{
    User_OPStr = new TStringList;
    User_ScriptCmd = new TStringList;
    ScriptSrc = new TMemoryStream;
    ScrStack = new CScriptStack;
    PascalScript = new IPascalScript;
    CppScript = new ICppScript;
    //OnRunCmdLine = NULL;
    OnBeforeRunLine = NULL;
    OnGetParValue   = NULL;
    ScriptRootPath = RootPath;
    ScriptState = scrNotRun;
    //    PascalScript->SetIdentWord(IdentWordList);
    //	PascalScript->SetSysFunc(SysFuncList);
    //	CppScript->SetIdentWord(IdentWordList);
    //	CppScript->SetSysFunc(SysFuncList);
    ActiveParser = PascalScript;
    UserFunction = new  AList<WY_Function>(true,100);
    RunTimer = new TTimer(NULL);
    RunTimer->Enabled = false;
    RunTimer->Interval = 200;
    RunTimer->OnTimer =   OnTimer;

}
//---------------------------------------------------------------------------
__fastcall WY_ScriptEngine::~WY_ScriptEngine()
{
    RunTimer->Enabled = true;
    delete UserFunction;
    delete RunTimer;
    delete User_OPStr;
    delete User_ScriptCmd;
    delete ScriptSrc;
    delete ScrStack;
    delete PascalScript;
    delete CppScript;
}
//---------------------------------------------------------------------------
void    WY_ScriptEngine::AddUserIdentWord(String IdentWord)
{
    PascalScript->AddUserIdentWord(IdentWord.UpperCase());
    CppScript->AddUserIdentWord(IdentWord.UpperCase());
}
//---------------------------------------------------------------------------
bool __fastcall WY_ScriptEngine::IsJumpOP(String OPCode)
{
    int i;
    for(i = 0; JumpOP[i]; i++)
    {
        if(std::strcmp(OPCode.c_str(), JumpOP[i]) == 0)
            return true;
    }

    return false;
}

//---------------------------------------------------------------------------
String __fastcall WY_ScriptEngine::GetOpCmd(String CmdLine)
{
    String          CmdHead, tmpStr;
    TReplaceFlags   Flag;
    tmpStr = CmdLine;
    CmdHead = ReadToken(tmpStr, " ");

    //SplitStr(CmdLine," ",CmdData);
    if(CmdHead != "")
    {
        SplitStr(tmpStr, ",", User_OPStr);

        //ȥ��"��
        for(int i = 0; i < User_OPStr->Count; i++)
        {
            User_OPStr->Strings[i] = StringReplace(User_OPStr->Strings[i],
                                                   "\"",
                                                   "",
                                                   Flag << rfReplaceAll);
        }
    }

    return CmdHead;
}

//-------------------------------------------------------------------------------
int __fastcall WY_ScriptEngine::GetParValue(const String &Value)
{
    if(Value == "NULL" || Value == "FALSE")
        return 0;
    else if(Value == "TRUE")
        return 1;
    else
    {
        int Par1;
        if(OnGetParValue)                           //�л�ȡ��������
        {
            if(ActiveParser->IsIdentWord(Value))    //�ǹؼ���
                Par1 = OnGetParValue(Value);
            else
                Par1 = StrToIntDef(Value, 0);       //���ǹؼ���
        }
        else                                        //û�л�ȡ��������
        {
            Par1 = StrToIntDef(Value, 0);
        }

        return Par1;
    }
}

//-------------------------------------------------------------------------------
void __fastcall WY_ScriptEngine::Scr_Call()
{
    String  SubScriptFile;
    if(User_OPStr->Count < 1)
    {
        LastError = "ִ��CALL���������,δָ������";
        ScriptEIP = -1;
        return;
    }

    SubScriptFile = User_OPStr->Strings[0];
    if(!FileExists(SubScriptFile))  //��������·��
    {
        SubScriptFile = ScriptRootPath + SubScriptFile;
    }

    if(!FileExists(SubScriptFile))  //��������·�������޷��ҵ��ļ�,ʧ��
    {
        LastError = "ִ��CALL���������,��Ӧ�ű�������";
        ScriptEIP = -1;
        return;
    }

    SCRIPTSTACK ScriptStack;
    ScriptEIP++;
    ScriptStack.ScrCmd = User_ScriptCmd;
    ScriptStack.EIP = ScriptEIP;
    ScriptStack.Name = ScriptName;
    ScriptStack.Info = ScriptInfo;
    ScriptStack.File = ScriptFile;
    ScrStack->Push(ScriptStack);
    ScriptState = scrNotRun;
    RunScript(SubScriptFile);
}

//------------------------------------------------------------------------------
void __fastcall WY_ScriptEngine::Scr_CMP()
{
    if(User_OPStr->Count < 2)
    {
        ScriptEIP++;
        return;
    }

    int Par1, Par2;

    //���ݱ������õ�������ʵ����ֵ
    Par1 = GetParValue(User_OPStr->Strings[0]);
    Par2 = GetParValue(User_OPStr->Strings[1]);
    Flag_Z = (Par1 == Par2);
    Flag_C = ((Par1 - Par2) < 0);
    ScriptEIP++;
    return;
}

//------------------------------------------------------------------------------
void __fastcall WY_ScriptEngine::Scr_JMP(String JmpCmd)
{
    String  DesLabel;
    bool    CanJmp = false;
    if(User_OPStr->Count < 1)
    {
        ScriptEIP++;
        return;
    }

    String  tmpOPStr = User_OPStr->Strings[0];
    DesLabel = ReadToken(tmpOPStr, " ");
    if(DesLabel == "")
    {
        DesLabel = User_OPStr->Strings[0];
    }

    //"JBE","JAE","JA","JB","JE","JNE","JMP",
    if(JmpCmd == "JMP")
    {
        CanJmp = true;
    }
    else if(JmpCmd == "JBE")    //С�ڵ���
    {
        if(Flag_Z || Flag_C)
            CanJmp = true;
    }
    else if(JmpCmd == "JB")     //С��
    {
        if(Flag_C)
            CanJmp = true;
    }
    else if(JmpCmd == "JAE")    //���ڵ���
    {
        if(!Flag_C || Flag_Z)
            CanJmp = true;
    }
    else if(JmpCmd == "JA")     //����
    {
        if(!Flag_C)
            CanJmp = true;
    }
    else if(JmpCmd == "JE")     //����
    {
        if(Flag_Z)
            CanJmp = true;
    }
    else if(JmpCmd == "JNE")    //������
    {
        if(!Flag_Z)
            CanJmp = true;
    }
    else
        CanJmp = false;

    if(CanJmp)
    {
        ScriptEIP = FindLabel(DesLabel);
    }
    else
    {
        ScriptEIP++;
    }

    return;
}

//------------------------------------------------------------------------------
int __fastcall WY_ScriptEngine::FindLabel(const String &LabelName)
{
    String  NewLab;
    NewLab = LabelName + ":";
    for(int i = 0; i < User_ScriptCmd->Count; i++)
    {
        if(User_ScriptCmd->Strings[i].Pos(NewLab) > 0)
            return i;
    }

    return User_ScriptCmd->Count + 1;
}

//------------------------------------------------------------------------------
bool __fastcall WY_ScriptEngine::GetScriptInfo(TMemoryStream *SrcCodeData,
        String &Name, String &Info,
        bool CanDelInfo)
{
	auto_ptr < TStringList > pScriptData(new TStringList);
    pScriptData->LoadFromStream(SrcCodeData);
    if(pScriptData->Count < 2)
    {
        Name = "undefine";
        Info = "undefine";
        return true;
    }
    if (pScriptData->Strings[0].UpperCase().Pos("NAME") ==0 || pScriptData->Strings[1].UpperCase().Pos("INFO") ==0)
    {
        Name = "undefine";
        Info = "undefine";
        return true;
    }
    String  LineStr;
    auto_ptr < TStringList > CmdLine(new TStringList);
    for(int i = 0; i < 2; i++)  //�ű�ǰ����Ϊ��Ϣ�ֶ�,ȡ������ɾ��
    {
        LineStr = pScriptData->Strings[0];
        SplitStr(LineStr, "=", CmdLine.get());
        if(CmdLine->Count == 2)
        {
            if(CmdLine->Strings[0].UpperCase() == "NAME")
            {
                Name = CmdLine->Strings[1];
                pScriptData->Delete(0);
            }
            else if(CmdLine->Strings[0].UpperCase() == "INFO")
            {
                Info = CmdLine->Strings[1];
                pScriptData->Delete(0);
                if(CanDelInfo)
                {
                    SrcCodeData->Clear();
                    pScriptData->SaveToStream(SrcCodeData);
                }
            }
        }
    }

    return true;
}

//------------------------------------------------------------------------------
bool __fastcall WY_ScriptEngine::RunScript(String ScriptFileName)
{
    if(!FileExists(ScriptFileName))
    {
        LastError = "�ű��ļ�δ�ҵ�";
        return false;
    }

	auto_ptr<TMemoryStream>		loadStream(new TMemoryStream);
	loadStream->LoadFromFile(ScriptFileName);
    ScriptFile = ScriptFileName;

    return RunScript(loadStream.get());
}

//�������нű�
bool __fastcall		WY_ScriptEngine::RunScript(TMemoryStream * SrcStream)
{
	if(IsRunning())
	{
		LastError = "�ű�����������";
		return false;
	}

	bool    Result = false;
	ScriptSrc->Clear();
	ScriptSrc->Position = 0;
	ScriptSrc->LoadFromStream(SrcStream);
	ScriptSrc->Position = 0;

    if(GetScriptInfo(ScriptSrc,ScriptName, ScriptInfo, true))
    {
        if(ActiveParser->GoScript(ScriptSrc, User_ScriptCmd))
        {
            ScriptState = scrRun;
            ScriptEIP = 0;
            RunTimer->Enabled = true;
            Result = true;
        }
        else
        {
            ScriptEIP = -1;
            LastError = "�ű��ļ��д���!ԭ��:" + ActiveParser->ErrorInfo;
        }
    }
    else
    {
        ScriptEIP = -1;
        LastError = "�ű��ļ��д���!�ű���Ϣͷ��ȫ";
    }

    return Result;
}

//���ַ����б����нű�
bool __fastcall		WY_ScriptEngine::RunScript(TStrings * SrcStrings)
{
	auto_ptr<TMemoryStream>		loadStream(new TMemoryStream);
	SrcStrings->SaveToStream(loadStream.get());
	return RunScript(loadStream.get());
}

//------------------------------------------------------------------------------
void __fastcall WY_ScriptEngine::StopScript()
{
    ScriptEIP = -1;
    ScriptState = scrNotRun;
    User_ScriptCmd->Clear();
    User_OPStr->Clear();
    ScrStack->Clear();
    ScriptSrc->Clear();
    ScriptName = "";
    ScriptInfo = "";
    ScriptFile = "";
    RunTimer->Enabled = false;
}

//------------------------------------------------------------------------------
bool __fastcall WY_ScriptEngine::DisposeCmdLine()
{
    String  CmdLine, OPCmd;
    if(ScriptEIP < User_ScriptCmd->Count && ScriptEIP >= 0)
    {
        CmdLine = User_ScriptCmd->Strings[ScriptEIP];
    }
    else                                    //��ǰ�ű�����
    {
        if(ScrStack->GetLevel() < 1)        //û���ϲ������
        {
            StopScript();
            LastError = "�ű���������";
            return false;
        }
        else                                //��ǰ�ű����ӵ��ã���Ҫ�ָ��ϲ���������л���
        {
            SCRIPTSTACK CurScript;
            CurScript.ScrCmd = User_ScriptCmd;
            ScrStack->Pop(CurScript);
            ScriptName = CurScript.Name;    //��ǰ�ű���
            ScriptInfo = CurScript.Info;    //��ǰ�ű���Ϣ
            ScriptFile = CurScript.File;    //��ǰ�ű��ļ�
            ScriptEIP = CurScript.EIP;
            return true;
        }

    }

    OPCmd = GetOpCmd(CmdLine).UpperCase();
    if(OPCmd == "CMP")                      //�Ƚϲ�����
    {
        Scr_CMP();
    }
    else if(IsJumpOP(OPCmd))                //��ת
    {
        Scr_JMP(OPCmd);
    }
    else if(OPCmd == "CALL")
    {
        Scr_Call();
        if(ScriptEIP == -1)
            return false;
    }
    else if(OPCmd == "EXIT")                //�����ű�����
    {
        LastError.sprintf("�û�ʹ��EXITָ��ǿ�ƽ����ű�,������־:%s",
                          User_OPStr->Strings[0]);
        ScriptEIP = -1;
        return false;
    }
    else if(OPCmd == "RESTART")             //��ͷ���нű�
    {
        ScriptEIP = 0;
        return true;
    }
    else
    {
        if(!ActiveParser->IsSysFunc(OPCmd)) //δ֪������ؼ���,ֱ�Ӻ���
        {
            ScriptEIP++;
            return true;
        }
        else
        {
            WY_Function * CurrentFunction = GetUserFunctionByName(OPCmd);
            if (CurrentFunction && CurrentFunction->UserFunctionPtr != NULL)
            {
                //�д˽ű������Ĵ�������
                if (!CurrentFunction->Params.ReadFromStrings(User_OPStr))
                {
                    LastError = FormatStr("Line:%d:����:%s�Ĳ�����������!",ScriptEIP,CurrentFunction->Name);
                    StopScript();
                    return false;
                }
                CurrentFunction->Run();
                ScriptEIP++;
                return true;
            }
            else
            {
                LastError = FormatStr("Line:%d:δָ���Զ�������:%s",ScriptEIP,OPCmd);
                StopScript();
                return false;
            }
        }
    }
    return true;
}

//------------------------------------------------------------------------------
int __fastcall WY_ScriptEngine::GetStackLv()
{   //�ӳ����ջ����
    if(ScriptState == scrNotRun)
        return -1;
    return ScrStack->GetLevel();
}

//------------------------------------------------------------------------------
String __fastcall WY_ScriptEngine::GetCurCmdLine()
{
    if(ScriptState == scrNotRun)
        return -1;
    if(ScriptEIP < User_ScriptCmd->Count && ScriptEIP >= 0)
    {
        return User_ScriptCmd->Strings[ScriptEIP];
    }
    else
    {
        return "�ű�δ����";
    }

}

//------------------------------------------------------------------------------
void WY_ScriptEngine::GetSysFuncList(TStrings *SysFunc)
{
    ActiveParser->GetSysFuncList(SysFunc);
}

//------------------------------------------------------------------------------
void WY_ScriptEngine::GetSysIdentWordList(TStrings *IdentWord)
{
    ActiveParser->GetSysIdentWordList(IdentWord);
}

//------------------------------------------------------------------------------
void WY_ScriptEngine::GetUserFuncList(TStrings *SysFunc)
{
    ActiveParser->GetUserFuncList(SysFunc);
}

//------------------------------------------------------------------------------
void WY_ScriptEngine::GetUserIdentWordList(TStrings *IdentWord)
{
    ActiveParser->GetUserIdentWordList(IdentWord);
}

//------------------------------------------------------------------------------
bool WY_ScriptEngine::GetCmdCodeList(TStrings *DesCmdList)
{
    if(!DesCmdList)
        return false;
    DesCmdList->Assign(User_ScriptCmd);
    return true;
}

//------------------------------------------------------------------------------
void WY_ScriptEngine::SetParserType(int Type)
{
    if(Type)
    {
        ActiveParser = CppScript;
    }
    else
    {
        ActiveParser = PascalScript;
    }

}
//------------------------------------------------------------------------------
void __fastcall WY_ScriptEngine::OnTimer(System::TObject* Sender)
{
    if (ScriptState == scrNotRun)
    {
        RunTimer->Enabled = false;
        return;
    }
    if (ScriptState == scrPause) //�ű���ͣ
    {
        return;
    }
    if (OnBeforeRunLine)
    {
        bool CanRunNextLine = true;
        OnBeforeRunLine(CanRunNextLine);
        if (!CanRunNextLine) //�û�ָ��,����������һ��,����ȴ�
        {
            return;
        }
    }

    if (!DisposeCmdLine())
    {
        StopScript();
    }
}
//------------------------------------------------------------------------------
int 	WY_ScriptEngine::ErrorPos()
{
    return ActiveParser->ErrorPos();
}
//------------------------------------------------------------------------------
int WY_ScriptEngine::GetUserFunctionIndex(const String &FuncName)
{
    for (int i = 0; i <UserFunction->Count(); i++)
    {
        if(UserFunction->At(i)->Name == FuncName)
        {
            return i;
        }
    }
    return -1;
}
//------------------------------------------------------------------------------
WY_Function * WY_ScriptEngine::GetUserFunction(int Index)
{
    return     UserFunction->At(Index);
}
//------------------------------------------------------------------------------
String __fastcall WY_ScriptEngine::ParseFunctionStr(const String &FunctionDef,WY_Params * Params)
{
	 String FunctionName = LeftString(FunctionDef,"(");
	 if (FunctionName =="")
	 {
		//����������
		return "";
	 }
	 String ParamDef 	 = RightString(FunctionDef,"(");
	 TReplaceFlags Flag;
	 Flag  << rfReplaceAll;
	 ParamDef = StringReplace(ParamDef,"(","",Flag);
	 ParamDef = LeftString(ParamDef,")");
	 auto_ptr < TStringList > ParamList(new TStringList);
	 SplitStr(ParamDef,",",ParamList.get());
	 //ShowDebugInfo("ParamStr:"+ParamDef);
	 if (ParamList->Count <=0)
	 {
		//������������
		return "";
	 }
	 String ParamType;
	 String ParamName;
	 Params->Clear();
	 for (int i = 0; i < ParamList->Count; i++)
	 {
		 ParamType = LeftString(ParamList->Strings[i]," ").Trim();
		 ParamName = RightString(ParamList->Strings[i]," ").Trim();
		 //ShowDebugInfo("Paramtype,name:"+ParamType+","+ParamName);
		 if (ParamType =="" || ParamName=="")
		 {
		   //��������
		   return "";
		 }
		 if (ParamType.LowerCase() == "string")
		 {
			 Params->Add(ParamName,ParamName," ",1);
		 }
		 else
		 {
			 Params->Add(ParamName,ParamName,0,0);
		 }
	 }
	 return FunctionName;
}
//------------------------------------------------------------------------------
bool    WY_ScriptEngine::AddUserFunc(String FuncDef,String Info,TUserFunction UserFuncPtr)
{
	 WY_Params  Params;
	 String FunctionName = 	 ParseFunctionStr(FuncDef,&Params);
	 if (FunctionName == "")
	 {
		return false;
	 }
	 return AddUserFunc(FunctionName,Info,&Params,UserFuncPtr);
}
//------------------------------------------------------------------------------
bool    WY_ScriptEngine::AddUserFunc(String FuncName,String Info,WY_Params *Params,TUserFunction UserFuncPtr)
{
    if (GetUserFunctionIndex(FuncName.UpperCase()) ==-1)
    {
        //�˺���������,��������
		WY_Function * Function = new  WY_Function;
        Function->Params.Assign(Params);
        Function->Name   =   FuncName.UpperCase();
        Function->Info   =   Info;
        Function->UserFunctionPtr  =  UserFuncPtr;
        UserFunction->Add(Function);
        PascalScript->AddUserFunc(Function->Name);
		CppScript->AddUserFunc(Function->Name);
		return true;
	}
	return false;
}

String	 WY_ScriptEngine::GetStataInfo()
{
    switch (ScriptState)
    {
    case scrNotRun:
        return "δ����";
    case scrRun:
        return "������";
    case scrPause:
        return "��ͣ";
    case scrRestart:
        return "����";
    default:
        return "δ֪";
    }
}


