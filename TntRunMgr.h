#ifndef __TNTRUNMGR_H__
#define __TNTRUNMGR_H__
#include "TntRun.h"

class TNTRunMgr {
public:
    enum QueueResult {
        // Joining queue states
        QUEUE_JOINED,
        QUEUE_ALREADYIN,
        QUEUE_FULL,
        QUEUE_ALREADYSTARTED,
        QUEUE_DISABLED,
        // Leaving queue states
        QUEUE_NOTJOINED,
        QUEUE_LEFT
    };
    TNTRunMgr(TNTRunMgr& const) = delete;
    TNTRunMgr& operator=(TNTRunMgr& const) = delete;
    static TNTRunMgr* instance();
    // Queue methods
    void AddToQueue(Player* player);
    void RemoveFromQueue(Player* player);
    void AddToQueue(Player* player, QueueResult& result);
    void RemoveFromQueue(Player* player, QueueResult& result);
    bool IsInQueue(Player* player);
    void UpdateState(TNTRun::State state, std::string reason);
private:
    TNTRunMgr();
    void LoadSettings();
    void ValidateSettings();
    void CheckRequiredPlayers();
    void StartEvent();
    TNTRun::Settings _settings;
    bool _badConfig;
    std::vector<Player*> _queue;
    TNTRun* _event;
    TNTRun::State _state;
};

#define sTNTRunMgr TNTRunMgr::instance()

#endif
