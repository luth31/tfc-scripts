#include "custom_config.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "MapManager.h"
#include "TntRunMgr.h"
#include "World.h"

TNTRunMgr* TNTRunMgr::instance() {
    static TNTRunMgr instance;
    return &instance;
}

TNTRunMgr::TNTRunMgr() : _badConfig(false), _event(nullptr) {
    LoadSettings();
}

void TNTRunMgr::LoadSettings() {
    _settings.objectId = sCustomCfg->GetUInt32("tntrun.object.id", 800000);
    _settings.map = sMapMgr->FindMap(sCustomCfg->GetUInt32("tntrun.mapid", 13), 0);
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
    if (_badConfig)
        UpdateState(TNTRun::State::EVENT_NOTREADY, "Invalid settings");
    else
        UpdateState(TNTRun::State::EVENT_READY, "Valid settings");
}

void TNTRunMgr::AddToQueue(Player* player) {
    QueueResult temp;
    AddToQueue(player, temp);
}

void TNTRunMgr::RemoveFromQueue(Player* player) {
    QueueResult temp;
    RemoveFromQueue(player, temp);
}

void TNTRunMgr::AddToQueue(Player* player, QueueResult& result) {
    switch (_state) {
    case TNTRun::EVENT_READY:
        if (IsInQueue(player)) {
            result = QueueResult::QUEUE_ALREADYIN;
            return;
        }
        if (_queue.size() >= _settings.maxPlayers) {
            result = QueueResult::QUEUE_FULL;
            return;
        }
        _queue.push_back(player);
        result = QueueResult::QUEUE_JOINED;
        CheckRequiredPlayers();
        break;
    case TNTRun::State::EVENT_STARTING:
    case TNTRun::State::EVENT_INPROGRESS:
    case TNTRun::State::EVENT_FINISHED:
        result = QueueResult::QUEUE_ALREADYSTARTED;
        break;
    default:
        result = QueueResult::QUEUE_DISABLED;
    }
}

void TNTRunMgr::RemoveFromQueue(Player* player, QueueResult& result) {
    if (!IsInQueue(player)) {
        result = QueueResult::QUEUE_NOTJOINED;
        return;
    }
    _queue.erase(std::find(_queue.begin(), _queue.end(), player));
    result = QueueResult::QUEUE_LEFT;
}

bool TNTRunMgr::IsInQueue(Player* player) {
    auto result = std::find(_queue.begin(), _queue.end(), player);
    if (result == _queue.end())
        return false;
    return true;
}

void TNTRunMgr::UpdateState(TNTRun::State state, std::string reason = "") {
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
        break;
    case TNTRun::State::EVENT_READY:
        TC_LOG_INFO("event.tntrun", "[TNTRunMgr] Event state changed to EVENT_READY. Reason: %s", reason);
        break;
    default:
        TC_LOG_ERROR("event.tntrun", "[TNTRunMgr] Invalid update state! %s", reason);
    }
}

void TNTRunMgr::CheckRequiredPlayers() {
    if (_queue.size() >= _settings.maxPlayers)
        StartEvent();
    if (_queue.size() >= _settings.minPlayers)
        // start timer
        int x;
}

void TNTRunMgr::StartEvent() {
    if (_badConfig) {
        TC_LOG_ERROR("event.tntrun", "[TNTRunMgr::StartEvent()] _badConfig is true, this shouldn't happen!");
        return;
    }
    _event = new TNTRun(_queue, _settings);
}
