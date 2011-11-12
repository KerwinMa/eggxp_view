//---------------------------------------------------------------------------


#pragma hdrstop

#include "TicketManager.h"
#include "Log4me.h"
#include <Mmsystem.h>

//---------------------------------------------------------------------------

#pragma package(smart_init)


TickObj::TickObj()
{
    RefreshTick();
    m_Interval = 1000;      //Ĭ�ϼ��Ϊ1��
}

TickObj::~TickObj()
{
}

void        TickObj::SetInterval(DWORD interval)
{
    m_Interval = interval;
}

int         TickObj::CanTick(DWORD   curTick)
{
    if(m_Interval == 0)
        return 1;
    if(curTick < m_Tick)
    {
        //������, RefreshTick��ʱ��, �õ�����GetTickCount, ������ͳһtick
        //�����ڶ��̵߳�ʱ��, ������ܻ�����������
        return  0;
    }
    DWORD   distTick = curTick - m_Tick;
    if(distTick > m_Interval)
    {                      
        DWORD   count = distTick / m_Interval;

        m_Tick += count * m_Interval;

        return  count;
    }
    return false;
}

void        TickObj::RefreshTick()
{
    m_Tick = timeGetTime();   
}

//=====================================================
TickManager::TickManager()
{
}

TickManager::~TickManager()
{
}

void         TickManager::AddTick(int  ttType, DWORD interval)
{
    int index = (int)ttType;
    TickObj *   curObj;
    while(m_Ticks.Count() <= index)
    {
        curObj = new TickObj;
        m_Ticks.Add(curObj);
    }
    m_Ticks[index]->SetInterval(interval);
}

TickObj     *       TickManager::GetTick(int  ttType)
{
    if(m_Ticks[ttType] == NULL)
    {
        GetLog()->Error("TickManager::GetTick ���� : TickType = %d", ttType);
    }
    return  m_Ticks[ttType];
}

void                TickManager::ResetAll()
{
    for(int i=0; i<m_Ticks.Count(); i++)
    {
        m_Ticks[i]->RefreshTick();
    }
}
