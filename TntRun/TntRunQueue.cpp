#include "Chat.h"
#include "ScriptMgr.h"
#include "TntRunQueue.h"
#include "TntRun.h"
#include "TntRunMgr.h"
#include "GameTime.h"

TNTRunQueue::Result TNTRunQueue::AddToQueue(Player* player) {
    if (IsInQueue(player))
        return Result::QUEUE_ALREADYIN;
    if (_queue.size() >= _maxPlayers)
        return Result::QUEUE_FULL;
    _queue.insert({ player, GameTime::GetGameTime() });
    return Result::QUEUE_JOINED;
}

TNTRunQueue::Result TNTRunQueue::RemoveFromQueue(Player* player) {
    if (!IsInQueue(player))
        return Result::QUEUE_NOTJOINED;
    _queue.erase(GetQueueIterFor(player));
    return Result::QUEUE_LEFT;
}

bool TNTRunQueue::IsInQueue(Player* player) {
    if (GetQueueIterFor(player) != _queue.end())
        return true;
    return false;
}

void TNTRunQueue::Init(TNTRun::Settings settings) {
    _minPlayers = settings.minPlayers;
    _maxPlayers = settings.maxPlayers;
    _queueDelay = settings.queueDelay * 1000; // Seconds to microseconds
    _active = true;
    _timer.elapsed = 0;
    _timer.enabled = false;
}

void TNTRunQueue::Update(uint32 diff) {
    if (!_active)
        return;
    if (_timer.enabled)
        _timer.elapsed += diff;
    Handle();
}

std::unordered_map<Player*, uint32>::const_iterator TNTRunQueue::GetQueueIterFor(Player* player) {
    return _queue.find(player);
}

void TNTRunQueue::Handle() {
    if (_queue.size() >= _minPlayers) {
        _timer.enabled = true;
        if (_timer.elapsed >= _queueDelay)
            ReportQueueReady();
    }
    else {
        // Disable timer and reset it in case it was already started
        _timer.enabled = false;
        _timer.elapsed = 0;
    }
}

uint32 TNTRunQueue::GetQueueTime(Player* player) {
    auto it = GetQueueIterFor(player);
    if (it == _queue.end())
        return 0;
    return it->second;
}

void TNTRunQueue::ReportQueueReady() {
    std::vector<Player*> players;
    for (auto const& kvp : _queue)
        players.push_back(kvp.first);
    sTNTRunMgr->StartEvent(players);
    _queue.clear();
}

uint32 TNTRunQueue::GetQueueSize() {
    return _queue.size();
}
