//---------------------------------------------------------------------------


#pragma hdrstop
#include <stdio.h>
#include "big2gbk.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)
//---------------------------------------------------------------------------
// ������תGBK�룺
// ���ؤH���@�M�� --> ���A���񹲺͇�
void __fastcall BIG52GBK(char *szBuf)
{
   int nStrLen = strlen(szBuf);
  wchar_t *pws = new wchar_t[nStrLen + 1];
  try
  {
    int nReturn = MultiByteToWideChar(950, 0, szBuf, nStrLen, pws, nStrLen + 1);
    BOOL bValue = false;
    nReturn = WideCharToMultiByte(936, 0, pws, nReturn, szBuf, nStrLen + 1, "?", &bValue);
    szBuf[nReturn] = 0;
  }
  __finally
  {
    delete[] pws;
  }
}
//---------------------------------------------------------------------------
// GBKת������
// ���A���񹲺͇� --> ���ؤH���@�M��
void __fastcall GBK2BIG5(char    *szBuf)
{
    if(!strcmp(szBuf, ""))
        return;

    int     nStrLen = strlen(szBuf);
    wchar_t *pws = new wchar_t[nStrLen + 1];
    try
    {
        MultiByteToWideChar(936, 0, szBuf, nStrLen, pws, nStrLen + 1);

        BOOL    bValue = false;
        WideCharToMultiByte(950,
                            0,
                            pws,
                            nStrLen,
                            szBuf,
                            nStrLen + 1,
                            "?",
                            &bValue);
        szBuf[nStrLen] = 0;
    }

    __finally
    {
        delete[] pws;
    }
}
//----------------------------------------------------------------------------
// GB2312��תGBK��
// �л����񹲺͹� --> ���A���񹲺͇�
void __fastcall GB2GBK(char    *szBuf)
{
    if(!strcmp(szBuf, ""))
        return;

    int     nStrLen = strlen(szBuf);
    WORD    wLCID = MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),
                             SORT_CHINESE_PRC);
    int     nReturn = LCMapString(wLCID,
                                  LCMAP_TRADITIONAL_CHINESE,
                                  szBuf,
                                  nStrLen,
                                  NULL,
                                  0);
    if(!nReturn)
        return;

    char    *pcBuf = new char[nReturn + 1];
    try
    {
        wLCID = MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),
                         SORT_CHINESE_PRC);
        LCMapString(wLCID,
                    LCMAP_TRADITIONAL_CHINESE,
                    szBuf,
                    nReturn,
                    pcBuf,
                    nReturn + 1);
        strncpy(szBuf, pcBuf, nReturn);
    }

    __finally
    {
        delete[] pcBuf;
    }
}
//---------------------------------------------------------------------------
// GBK��תGB2312��
// ���A���񹲺͇� --> �л����񹲺͹�
void __fastcall GBK2GB(char    *szBuf)
{
    if(!strcmp(szBuf, ""))
        return;

    int     nStrLen = strlen(szBuf);
    WORD    wLCID = MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),
                             SORT_CHINESE_BIG5);
    int     nReturn = LCMapString(wLCID,
                                  LCMAP_SIMPLIFIED_CHINESE,
                                  szBuf,
                                  nStrLen,
                                  NULL,
                                  0);
    if(!nReturn)
        return;

    char    *pcBuf = new char[nReturn + 1];
    try
    {
        wLCID = MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),
                         SORT_CHINESE_BIG5);
        LCMapString(wLCID,
                    LCMAP_SIMPLIFIED_CHINESE,
                    szBuf,
                    nReturn,
                    pcBuf,
                    nReturn + 1);
        strncpy(szBuf, pcBuf, nReturn);
    }

    __finally
    {
        delete[] pcBuf;
    }
}
//------------------------------------------------------------------------------
String GB2Big5(String InStr)
{
    if (InStr.Length() <= 0)
      return "";
     GB2GBK(InStr.c_str());
    GBK2BIG5(InStr.c_str());
    return InStr;
}
//------------------------------------------------------------------------------
String Big52GB(String InStr)
{
    if (InStr.Length() <= 0)
      return "";
    
    String CopyStr = InStr;
    CopyStr.Unique(); 
    BIG52GBK(CopyStr.c_str());
    GBK2GB(CopyStr.c_str());
    return CopyStr;
}

