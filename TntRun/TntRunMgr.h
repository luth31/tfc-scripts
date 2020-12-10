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
    time_t GetQueueTimeFor(Player* player);
    void LoadSettings();
    uint32 GetQueueRemainingSeconds();
    bool IsQueueTimerRunning();
    uint32 GetAliveCount();
    time_t GetEventStartTime();
private:
    TNTRunMgr();
    void ValidateSettings();
    time_t _startTime;
    TNTRunQueue _queue;
    TNTRun::Settings _settings;
    TNTRun::Playground _playground;
    bool _badConfig;
    TNTRun _event;
    TNTRun::State _state;
    
};

#define sTNTRunMgr TNTRunMgr::instance()

#endif
