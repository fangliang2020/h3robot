#ifndef __CDATAMODEL_H
#define __CDATAMODEL_H
#include <queue>
#include <mutex>
#include <vector>
#include "DeviceStates.h"
#include <time.h>
#include "DayTime.h"
class CDataModel
{
    public:
        CDataModel();
        ~CDataModel();
        static CDataModel* getDataModel();
        static std::mutex m_instMutex;
        E_MASTER_STATE getMState();
        CLEAN_STATE getCleanState();
        std::vector<int> &getFaultVector();
        void setMState(E_MASTER_STATE mst);
        void setCleanState(CLEAN_STATE cmst);
        void setFState(int fmst);
        int getMQueueSize();
        int getMQueueFort();
        int getMQueueBack();
        int getCQueueSize();
        int getCQueueFort();
        int getCQueueBack();
        void deletMQueue();
        void deletCQueue();
        void getCurrentTime();
    private:
        static CDataModel* m_pInstance;
        std::queue<int> MstateData;
        std::queue<int> CstateData;
        std::vector<int> FstateData;
        int m_state;
        int clean_state;
        int fault_state;


};
#endif