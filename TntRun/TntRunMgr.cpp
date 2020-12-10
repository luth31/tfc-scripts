#include "../CustomConfig/CustomConfig.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "MapManager.h"
#include "GameTime.h"
#include "TntRunMgr.h"
#include "World.h"

TNTRunMgr* TNTRunMgr::instance() {
    static TNTRunMgr instance;
    return &instance;
}

TNTRunMgr::TNTRunMgr() : _badConfig(false) {
    LoadSettings();
    _queue.Init(_settings);
}

void TNTRunMgr::LoadSettings() {
    _settings.objectId = sCustomCfg->GetUInt32("tntrun.object.id", 800000);
    Map* map = sMapMgr->CreateBaseMap(sCustomCfg->GetUInt32("tntrun.mapid", 13));
    _settings.map = map;
    _settings.origin = Position(sCustomCfg->GetDouble("tntrun.origin.x", 65.f),
        sCustomCfg->GetDouble("tntrun.origin.y", 95.f),
        sCustomCfg->GetDouble("tntrun.origin.z", -100.f),
        sCustomCfg->GetDouble("tntrun.origin.o", 0.f));
    _settings.sizeX = (uint8)sCustomCfg->GetUInt32("tntrun.size.x", 10);
    _settings.sizeY = (uint8)sCustomCfg->GetUInt32("tntrun.size.y", 10);
    _settings.levels = (uint8)sCustomCfg->GetUInt32("tntrun.size.levels", 3);
    _settings.offsetX = sCustomCfg->GetDouble("tntrun.offset.x", -1.5f);
    _settings.offsetY = sCustomCfg->GetDouble("tntrun.offset.y", -1.5f);
    _settings.offsetZ = sCustomCfg->GetDouble("tntrun.offset.z", 10.f);
    _settings.minPlayers = sCustomCfg->GetUInt32("tntrun.players.min", 5);
    _settings.maxPlayers = sCustomCfg->GetUInt32("tntrun.players.max", 10);
    _settings.queueDelay = sCustomCfg->GetUInt32("tntrun.queue.delay", 60000);
    _settings.warmupTime = sCustomCfg->GetUInt32("tntrun.warmup", 10000);
    _settings.reward_count = sCustomCfg->GetUInt32("tntrun.reward.count", 1);
    _settings.reward_entry = sCustomCfg->GetUInt32("tntrun.reward.entry", 300000);
    _settings.pacify_spell = sCustomCfg->GetUInt32("tntrun.pacify.id", 60778);
    _settings.anti_afk = sCustomCfg->GetUInt32("tntrun.antiafk.timer", 3000);
    _playground.maxX = (_settings.sizeX * _settings.offsetX / 2) + sCustomCfg->GetDouble("tntrun.bounds.x", 150);
    _playground.maxY = (_settings.sizeY * _settings.offsetY / 2) + sCustomCfg->GetDouble("tntrun.bounds.y", 150);;
    _playground.maxZ = (_settings.levels * _settings.offsetZ) + sCustomCfg->GetDouble("tntrun.bounds.z", 150);;
    Position temp = _settings.origin;
    temp.m_positionX += _settings.offsetX * (_settings.sizeX - 1);
    temp.m_positionY += _settings.offsetY * (_settings.sizeY - 1);
    temp.m_positionZ += _settings.offsetZ * (_settings.levels -1);
    _playground.center = Position(
        (_settings.origin.GetPositionX() + temp.GetPositionX()) / 2,
        (_settings.origin.GetPositionY() + temp.GetPositionY()) / 2,
        (_settings.origin.GetPositionZ() + temp.GetPositionZ()) / 2,
        0.f);
    ValidateSettings();
}

void TNTRunMgr::ValidateSettings() {
    _badConfig = false;
    if (_settings.map == nullptr) {
        TC_LOG_ERROR("event.tntrun", "[TNTRunMgr::ValidateSettings] Map is nullptr, expected %u", sCustomCfg->GetUInt32("tntrun.mapid", 13));
        _badConfig = true;
    }
    if (sObjectMgr->GetGameObjectTemplate(_settings.objectId) == nullptr) {
        TC_LOG_ERROR("event.tntrun", "[TNTRunMgr::ValidateSettings] GameObject is nullptr, expected %u", sCustomCfg->GetUInt32("tntrun.object.id", 800000));
        _badConfig = true;
    }
    if (_settings.minPlayers > _settings.maxPlayers || _settings.maxPlayers == 0) {
        TC_LOG_ERROR("event.tntrun", "[TNTRunMgr::ValidateSettings] Min players > Max players OR Max players == 0", sCustomCfg->GetUInt32("tntrun.mapid", 13));
        _badConfig = true;
    }
    if (_settings.levels == 0) {
        TC_LOG_ERROR("event.tntrun", "[TNTRunMgr::ValidateSettings] Event can't have 0 levels!", sCustomCfg->GetUInt32("tntrun.mapid", 13));
        _badConfig = true;
    }
    if (_badConfig) {
        UpdateState(TNTRun::State::EVENT_NOTREADY, "Invalid settings");
    }
    UpdateState(TNTRun::State::EVENT_READY, "Valid settings");
    _event.UpdateSettings(_settings, _playground);
}

bool TNTRunMgr::IsInQueue(Player* player) {
    return _queue.IsInQueue(player);
}

TNTRunQueue::Result TNTRunMgr::AddToQueue(Player* player) {
    switch (_state) {
    case TNTRun::State::EVENT_STARTING:
    case TNTRun::State::EVENT_INPROGRESS:
    case TNTRun::State::EVENT_FINISHED:
        return TNTRunQueue::Result::QUEUE_ALREADYSTARTED;
    case TNTRun::State::EVENT_READY:
        return _queue.AddToQueue(player);
    case TNTRun::State::EVENT_NOTREADY:
    default:
        return TNTRunQueue::Result::QUEUE_DISABLED;
    }
}

uint32 TNTRunMgr::GetQueueRemainingSeconds() {
    return _queue.GetRemainingSeconds();
}

bool TNTRunMgr::IsQueueTimerRunning() {
    return _queue.IsTimerRunning();
}

uint32 TNTRunMgr::GetAliveCount() {
    return _event.GetAliveCount();
}

time_t TNTRunMgr::GetEventStartTime() {
    return _startTime;
}

TNTRunQueue::Result TNTRunMgr::RemoveFromQueue(Player* player) {
    return _queue.RemoveFromQueue(player);
}

TNTRun::Settings TNTRunMgr::GetSettings() {
    return _settings;
}

TNTRun::State TNTRunMgr::GetEventState() {
    return _state;
}

void TNTRunMgr::UpdateState(TNTRun::State state, std::string reason) {
    _state = state;
    switch (_state) {
    case TNTRun::State::EVENT_NOTREADY:
        TC_LOG_ERROR("event.tntrun", "[TNTRunMgr] Event state changed to EVENT_NOTREADY. Reason: %s", reason);
        break;
    case TNTRun::State::EVENT_STARTING:
        TC_LOG_INFO("event.tntrun", "[TNTRunMgr] Event state changed to EVENT_STARTING. Reason: %s", reason);
        break;
    case TNTRun::State::EVENT_INPROGRESS:
        TC_LOG_INFO("event.tntrun", "[TNTRunMgr] Event state changed to EVENT_INPROGRESS. Reason: %s", reason);
        break;
    case TNTRun::State::EVENT_FINISHED:
        TC_LOG_INFO("event.tntrun", "[TNTRunMgr] Event state changed to EVENT_FINISHED. Reason: %s", reason);
        StopEvent();
        LoadSettings(); // Reload settings, will change state to READY or NOTREADY
        break;
    case TNTRun::State::EVENT_READY:
        TC_LOG_INFO("event.tntrun", "[TNTRunMgr] Event state changed to EVENT_READY. Reason: %s", reason);
        break;
    default:
        TC_LOG_ERROR("event.tntrun", "[TNTRunMgr] Invalid update state! %s", reason);
    }
}

void TNTRunMgr::StartEvent(std::vector<Player*> players) {
    LoadSettings();
    if (_badConfig) {
        TC_LOG_ERROR("event.tntrun", "[TNTRunMgr::StartEvent()] _badConfig is true, this shouldn't happen!");
        sWorld->SendWorldText(3, "[TNTRun] Failed to start event. Report this error.");
        return;
    }
    _startTime = GameTime::GetGameTime();
    _event.Start(players);
    UpdateState(TNTRun::State::EVENT_STARTING, "TNTRunMgr::StartEvent()");
}

void TNTRunMgr::StopEvent() {
    _event.Stop();
    UpdateState(TNTRun::State::EVENT_READY, "Stopped");
}

void TNTRunMgr::Update(uint32 diff) {
    if (_state == TNTRun::State::EVENT_NOTREADY)
        return;
    _queue.Update(diff);
    if (_state == TNTRun::State::EVENT_READY)
        return;
    _event.Update(diff);
}

void TNTRunMgr::HandlePlayerLogout(Player* player) {
    _event.HandlePlayerLogout(player);
}

uint32 TNTRunMgr::GetQueueSize() {
    return _queue.GetQueueSize();
}

time_t TNTRunMgr::GetQueueTimeFor(Player* player) {
    return _queue.GetQueueTime(player);
}
