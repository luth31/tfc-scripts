#ifndef __TNTRUNMGR_H__
#define __TNTRUNMGR_H__
#include "TntRun.h"
#include "TntRunQueue.h"

class TNTRunMgr {
public:
    TNTRunMgr(TNTRunMgr&) = delete;
    TNTRunMgr& operator=(TNTRunMgr&) = delete;
    static TNTRunMgr* instance();
    // Queue methods
    TNTRunQueue::Result RemoveFromQueue(Player* player);
    TNTRunQueue::Result AddToQueue(Player* player);
    bool IsInQueue(Player* player);
    TNTRun::State GetEventState();
    void UpdateState(TNTRun::State state, std::string reason);
    TNTRun::Settings GetSettings();
    void StopEvent();
    void StartEvent(std::vector<Player*> players);
    void Update(uint32 diff);
    void HandlePlayerLogout(Player* player);
    uint32 GetQueueSize();
    uint32 GetQueueTimeFor(Player* player);
private:
    TNTRunMgr();
    void LoadSettings();
    void ValidateSettings();
    
    TNTRunQueue _queue;
    TNTRun::Settings _settings;
    bool _badConfig;
    TNTRun _event;
    TNTRun::State _state;
    
};

#define sTNTRunMgr TNTRunMgr::instance()

#endif
