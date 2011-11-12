//---------------------------------------------------------------------------


#pragma hdrstop

#include "wydbf.h"
#include "CommFunc.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)
void WY_Field::SetType(int Type)
{
    m_Type = Type;
    switch(m_Type)
    {
        case ftBYTE:
            m_Length = 1;
            break;

        case ftWORD:
            m_Length = 2;
            break;

        case ftDWORD:
            m_Length = 4;
            break;

        case ftBUFFER:
            m_Length = 1;
            break;

        case ftSTRING:
            m_Length = 1;
            break;

        default:
            m_Length = 0;
    }

}
//------------------------------------------------------------------------------
void WY_Field::SetName(String Name)
{
   memset(m_Name,0,MAX_FIELD_NAME_LEN);
   int Len = Name.Length();
   if (Len >  MAX_FIELD_NAME_LEN-1)
   {
      Len = MAX_FIELD_NAME_LEN-1;
   }
   CopyMemory(m_Name,Name.c_str(),Len);
}
//------------------------------------------------------------------------------
void WY_Field::SetLength(int Length)
{
    //�����ֶγ���,��ftBUFFER��ftSTRING���͵��ô˺�����Ч
    if (m_Type == ftBUFFER || m_Type == ftSTRING)
    {
       m_Length = Length;
    }
}
//------------------------------------------------------------------------------
int  WY_Field::WriteToBuffer(char * lpDesData)
{
    int Pos=0;
    WriteBuf(lpDesData,Pos,m_Name,MAX_FIELD_NAME_LEN);
    WriteDWORD(lpDesData,Pos,m_Type);
    WriteDWORD(lpDesData,Pos,m_Length);
    return Pos;
}
//------------------------------------------------------------------------------
int  WY_Field::ReadFromBuffer(char * lpDesData)
{
    int Pos=0;
    ReadBuf(lpDesData,Pos,m_Name,MAX_FIELD_NAME_LEN);
    m_Type = ReadDWORD(lpDesData,Pos);
    m_Length = ReadDWORD(lpDesData,Pos);
    return Pos;
}
//------------------------------------------------------------------------------
void  WY_Field::WriteToStream(TMemoryStream * Stream)
{
    Stream->Write(m_Name,MAX_FIELD_NAME_LEN);
    Stream->Write(&m_Type,sizeof(m_Type));
    Stream->Write(&m_Length,sizeof(m_Length));
}


//------------------------------------------------------------------------------
//class WYDB_File
//------------------------------------------------------------------------------
__fastcall WYDB_File::WYDB_File()
{
     memset(&Head,0,sizeof(Head));
     lpFieldList = new  AList<WY_Field>(true,100);
     lpDataStream = new TMemoryStream();
     lpDataRecord = new TStringList();
     m_IsOpened = false;
     m_CurrentRecordNO = 0;
}
//------------------------------------------------------------------------------
__fastcall WYDB_File::~WYDB_File()
{
     delete lpDataRecord;
     delete lpDataStream;
     delete lpFieldList;
}
//------------------------------------------------------------------------------
bool __fastcall WYDB_File::LoadFromFile(String FileName)
{
   if (!FileExists(FileName))
   {
     return false;
   }
  bool Result=false;
  try
  {
     lpDataStream->LoadFromFile(FileName);
     ReadDataToMem();
     Result = true;
     m_IsOpened = true;
  }
  catch(...)
  {
     Result = false;
  }
   return Result;
}
//------------------------------------------------------------------------------
bool __fastcall WYDB_File::SaveToFile(String FileName)
{
  bool Result=false;
  try
  {
     SaveMemToStream();
     lpDataStream->SaveToFile(FileName);
     Result = true;
  }
  catch(...)
  {
     Result = false;
  }
  return Result;

}
//------------------------------------------------------------------------------
bool __fastcall WYDB_File::LoadFromStream(TMemoryStream * DataStream)
{
  bool Result=false;
  try
  {
     lpDataStream->LoadFromStream(DataStream);
     ReadDataToMem();
     Result = true;
     m_IsOpened = true;
  }
  catch(...)
  {
     Result = false;
  }
  return Result;
}
//------------------------------------------------------------------------------
bool __fastcall WYDB_File::SaveToStream(TMemoryStream * DataStream)
{
  bool Result=false;
  try
  {
     SaveMemToStream();
     lpDataStream->SaveToStream(DataStream);
     Result = true;
  }
  catch(...)
  {
     Result = false;
  }
  return Result;
}
//------------------------------------------------------------------------------
int   WYDB_File::GetRecordLength()
{
     WY_Field * lpCurrentField;
     int RecordSize=0;
     for(int i=0;i<lpFieldList->Count();i++)
     {
         lpCurrentField =lpFieldList->At(i);
         RecordSize+=lpCurrentField->GetLength();
     }
     return RecordSize;
}
//------------------------------------------------------------------------------
void  WYDB_File::ReadDataToMem()
{
     lpDataStream->Position = 0;
     //��ȡͷ��Ϣ
     lpDataStream->Read(&Head,sizeof(Head));
      //У��ͷ��Ϣ
     if (Head.Flag !=FLAG)
     {
         LastError = "�ļ�ͷ��־������";
         memset(&Head,0,sizeof(Head));
         return;
     }
     //У��汾��־
     if (Head.VerFlag !=VER_FLAG)
     {
         LastError = "�ļ��汾����";
         memset(&Head,0,sizeof(Head));
         return;
     }
     //�ֶο�ʼ����ָ��
     char * lpData =  (char *)lpDataStream->Memory;
     lpData += lpDataStream->Position;
     //��ȡ�ֶ���Ϣ
     WY_Field * lpCurrentField;
     lpFieldList->Clear();
     for(DWORD i=0;i<Head.FieldCount;i++)
     {
         lpCurrentField = new  WY_Field;
         lpData+=lpCurrentField->ReadFromBuffer(lpData);
         lpFieldList->Add(lpCurrentField);
     }
     //��ȡʵ��������Ϣ
     int RecordSize = GetRecordLength(); //��ȡÿ����¼�Ĵ�С
     lpDataRecord->Clear();
     for(DWORD i=0;i<Head.RecordCount;i++)
     {
        lpDataRecord->Add(String(lpData,RecordSize));
        lpData+=RecordSize;
     }

     lpDataStream->Clear(); //��ȡ����ͷ����������Խ�ʡ�ڴ�
}
//------------------------------------------------------------------------------
void  WYDB_File::SaveMemToStream()
{
    lpDataStream->Clear();
    //д���ļ�ͷ��Ϣ
    Head.Flag = FLAG;
    Head.VerFlag = VER_FLAG;
    Head.FieldCount = lpFieldList->Count();
    Head.RecordCount = lpDataRecord->Count;
    lpDataStream->Write(&Head,sizeof(Head));
     WY_Field * lpCurrentField;
     for(int i=0;i<lpFieldList->Count();i++)
     {
         lpCurrentField =lpFieldList->At(i);
         lpCurrentField->WriteToStream(lpDataStream);
     }
}
//------------------------------------------------------------------------------
int  WYDB_File::IndexOfFieldName(String FieldName)
{
     WY_Field * lpCurrentField;
     for(int i=0;i<lpFieldList->Count();i++)
     {
         lpCurrentField =lpFieldList->At(i);
         if (UpperCase(lpCurrentField->GetName()) ==  UpperCase(FieldName))
         {
             return i;
         }
     }
     return -1;
}
//------------------------------------------------------------------------------
bool WYDB_File::AddField(String Name,int Type,int Length)
{
   int FieldIndex = IndexOfFieldName(Name);
   if (FieldIndex==-1)
   {
     WY_Field * lpCurrentField;
     lpCurrentField = new  WY_Field;
     lpCurrentField->SetName(Name);
     lpCurrentField->SetType(Type);
     lpCurrentField->SetLength(Length);
     lpFieldList->Add(lpCurrentField);
     return true;
   }
   else
   {
      return false;
   }
}
//------------------------------------------------------------------------------
void  WYDB_File::SetDBName(String DBName)
{
   memset(Head.DB_Name,0,MAX_DBNAME_LEN);
   int Len = DBName.Length();
   if (Len >  MAX_DBNAME_LEN-1)
   {
      Len = MAX_DBNAME_LEN-1;
   }
   CopyMemory(Head.DB_Name,DBName.c_str(),Len);
}
//------------------------------------------------------------------------------
String  WYDB_File::GetRecordAsString(String FieldName,int NO)
{
    int FieldIndex = IndexOfFieldName(FieldName);
    return GetRecordAsString(FieldIndex,NO);
}

//------------------------------------------------------------------------------
String  WYDB_File::GetRecordAsString(int FieldIndex,int NO)
{
    WY_Field * Field = lpFieldList->At(FieldIndex);
    if (Field==NULL)
     return "";
    String Data;
    Data.SetLength(Field->GetLength());
    GetRecordAsBuffer(FieldIndex,Data.c_str(),NO);
    //���޸�
    return Data;
}
//------------------------------------------------------------------------------
int     WYDB_File::GetRecordAsInteger(String FieldName,int NO)
{
; //��ȡ��ǰ��¼ĳ���ֶε�����,��int��ʽ
}
//------------------------------------------------------------------------------
int     WYDB_File::GetRecordAsInteger(int FieldIndex,int NO)
{
;
}
//------------------------------------------------------------------------------
int     WYDB_File::GetRecordAsBuffer(String FieldName,char * lpOutBuffer,int NO)
{
    int FieldIndex = IndexOfFieldName(FieldName);
    return GetRecordAsBuffer(FieldIndex,lpOutBuffer,NO);
}
//------------------------------------------------------------------------------
int     WYDB_File::GetRecordAsBuffer(int FieldIndex,char * lpOutBuffer,int NO)
{
   int TrueNO;
   if (NO < 0)
   {
      TrueNO = m_CurrentRecordNO;
   }
   else
   {
      TrueNO =  NO;
   }
   WY_Field * Field = lpFieldList->At(FieldIndex);
   if (TrueNO< 0 || TrueNO>=GetRecordCount() || Field==NULL)
   {
      return 0;
   }
   int PosStart = GetFieldBufferStartPos(FieldIndex);
   String Data = lpDataRecord->Strings[TrueNO];
   if (PosStart >= Data.Length())
   {
      return 0;
   }
   memcpy(lpOutBuffer,Data.c_str()+PosStart,Field->GetLength());
   return  Field->GetLength();
}
//------------------------------------------------------------------------------
bool    WYDB_File::SetRecordAsString(String FieldName,String Data,int NO)
{
;  //ˢ�µ�ǰ��¼ĳ���ֶε�����,��String��ʽ
}
//------------------------------------------------------------------------------
bool    WYDB_File::SetRecordAsInteger(String FieldName,int Data,int NO)
{
; //ˢ�µ�ǰ��¼ĳ���ֶε�����,��int��ʽ
}
//------------------------------------------------------------------------------
bool    WYDB_File::SetRecordAsBuffer(String FieldName,char * lpInBuffer,int Len,int NO)
{

}
//------------------------------------------------------------------------------
void WYDB_File::Append()
{
    if (!m_IsOpened)
    {
       return;
    }
    String NewData;
    int RecordSize = GetRecordLength(); //��ȡÿ����¼�Ĵ�С
    NewData.SetLength(RecordSize);
    memset(NewData.c_str(),0,RecordSize);
    lpDataRecord->Add(NewData);
    Last();
}
//------------------------------------------------------------------------------
void    WYDB_File::Last()
{
    if (!m_IsOpened)
    {
       return;
    }
   m_CurrentRecordNO = lpDataRecord->Count-1;
}
//------------------------------------------------------------------------------
void    WYDB_File::Next()
{
    if (!m_IsOpened)
    {
       return;
    }
    m_CurrentRecordNO++;
    if (m_CurrentRecordNO>=lpDataRecord->Count)
    {
        Last();
    }
}
//------------------------------------------------------------------------------
void    WYDB_File::Prev()
{
    if (!m_IsOpened)
    {
       return;
    }
    m_CurrentRecordNO--;
    if (m_CurrentRecordNO <0)
    {
       m_CurrentRecordNO=0;
    }
}
//------------------------------------------------------------------------------
void    WYDB_File::First()
{
    if (!m_IsOpened)
    {
       return;
    }
   m_CurrentRecordNO = 0;
}
//------------------------------------------------------------------------------
