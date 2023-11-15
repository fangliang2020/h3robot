#include "DateModel.h"
#include <mutex>
#include <iostream>
std::mutex CDataModel::m_instMutex;
CDataModel* CDataModel::m_pInstance = nullptr;
CDataModel::CDataModel()
{
    m_state = E_MASTER_STATE(ESTATE_IDLE);
    clean_state = CLEAN_STATE(CLEAN_STATE_NULL);
    fault_state = 0;

}
CDataModel::~CDataModel()
{
    if (m_pInstance)
    {
        delete(m_pInstance);
        m_pInstance = nullptr;
    }
    

}
CDataModel* CDataModel::getDataModel()
{
    if (m_pInstance == nullptr )
    {
        m_instMutex.lock();
        if (m_pInstance == nullptr )
        {
            m_pInstance = new CDataModel();
        }
        m_instMutex.unlock();
    }
    return m_pInstance;
}

int CDataModel::getMQueueSize()
{
    return MstateData.size();
}
int CDataModel::getMQueueFort()
{
    return MstateData.front();
}
int CDataModel::getMQueueBack()
{
    return MstateData.back();
}
int CDataModel::getCQueueSize()
{
    return CstateData.size();
}
int CDataModel::getCQueueFort()
{
    return CstateData.front();
}
int CDataModel::getCQueueBack()
{
    return CstateData.back();
}
void CDataModel::deletMQueue()
{
    MstateData.pop();
}
void CDataModel::deletCQueue()
{
    CstateData.pop();
}
void CDataModel::getCurrentTime()
{
    DayTime daytime;
    time_t now_time = time(NULL);
    tm* t_time = localtime(&now_time);
    {
    m_instMutex.lock();
    daytime.time_year = 1900 + t_time->tm_year;
    daytime.time_mon = t_time->tm_mon;
    daytime.time_mday = t_time->tm_mday;
    daytime.time_wday = t_time->tm_wday;
    daytime.time_hour = t_time->tm_hour;
    daytime.time_min = t_time->tm_min;
    daytime.now_time = t_time->tm_hour*60 + t_time->tm_min;
    //daytime.time_sec = t_time->tm_sec;
    m_instMutex.unlock();
    }

}
CLEAN_STATE CDataModel::getCleanState()
{
    int cstate = clean_state;
    if (CstateData.empty() == true)
    {
        /* code */
        return (CLEAN_STATE)cstate;
    }
    else
    {
        auto cstate = CstateData.back();
        return (CLEAN_STATE)cstate;
    }

}
void CDataModel::setCleanState(CLEAN_STATE cmst)
{
    if (clean_state == cmst)
    {
        return;
    }
    clean_state = cmst;
    CstateData.push(cmst);
    std::cout << "cleanstate is"<< cmst << std::endl;
    if (CstateData.size() > 2)
    {
        CstateData.pop();
    }
    
    

}
void CDataModel::setMState(E_MASTER_STATE mst)
{
    if (m_state == mst)
    {
        /* code */
        return;
    }
    m_state = mst;
    std::cout << "Mstate is" << m_state <<std::endl;
    MstateData.push(mst);
    if (MstateData.size() > 2)
    {
        MstateData.pop();
        //MstateData.push(mst);

    }

}
E_MASTER_STATE CDataModel::getMState()
{
    int state = m_state;
    if (MstateData.empty() == true)
    {
        /* code */
        return (E_MASTER_STATE)state;
    }
    else
    {
        auto state = MstateData.back();
        return (E_MASTER_STATE)state;
    }
    
    //auto data = MstateData.back();
    //m_state = data;
    //return (E_MASTER_STATE)data;
}
std::vector<int> &CDataModel::getFaultVector()
{
    return FstateData;
     
}
void CDataModel::setFState(int fmst)
{
    if (fault_state == fmst)
    {
        return;
    }
    fault_state = fmst;
    FstateData.emplace_back(fmst);
    
}