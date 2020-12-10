#ifndef __TNTRUNQUEUE_H__
#define __TNTRUNQUEUE_H__
#include "ScriptMgr.h"
#include "TntRun.h"
#include <unordered_map>

class TNTRunQueue {
public:
    enum class Result {
        QUEUE_JOINED, // Played added to queue
        QUEUE_ALREADYIN, // Player already in queue
        QUEUE_FULL, // Queue is full
        QUEUE_ALREADYSTARTED, // Match already started
        QUEUE_DISABLED, // Queue is disabled
        QUEUE_NOTJOINED, // Player not in queue
        QUEUE_LEFT // Player left queue
    };
    Result AddToQueue(Player* player);
    Result RemoveFromQueue(Player* player);
    bool IsInQueue(Player* player);
    void Init(TNTRun::Settings settings);
    void Update(uint32 diff);
    uint32 GetQueueSize();
    time_t GetQueueTime(Player* player);
    int GetRemainingSeconds();
    bool IsTimerRunning();
private:
    struct Timer {
        uint32 elapsed;
        bool enabled;
    };
    std::unordered_map<Player*, time_t>::const_iterator GetQueueIterFor(Player* player);
    void Handle();
    void ReportQueueReady();
    std::unordered_map<Player*, time_t> _queue;
    uint32 _minPlayers;
    uint32 _maxPlayers;
    uint32 _queueDelay;
    Timer _timer;
    bool _active;
};

#endif
