#include "GameObject.h"
#include "Log.h"
#include "TntRun.h"
#include "TntRunMgr.h"
#include "TntRunMisc.h"
#include "ScriptMgr.h"
#include "WorldSession.h"
#include "ScriptedGossip.h"
#include "World.h"

// --- Event control related methods ---
// Start the event, called by TNTRunMgr
void TNTRun::Start(std::vector<Player*> players) {
    _info.players.insert(_info.players.end(), players.begin(), players.end());
    SpawnObjects();
    SummonPlayers();
    warmupTimer = 0;
}

// Stop the event, called by TNTRunMgr
void TNTRun::Stop() {
    for (auto obj : _objects) {
        obj->SetRespawnTime(0);
        obj->Delete();
    }
    for (auto player : _info.players) {
        RecallPlayer(player);
    }
    for (auto player : _info.spectators) {
        RecallPlayer(player);
    }
    _info.players.clear();
    _info.spectators.clear();
    _objects.clear();
}

// Updates settings, called by TNTRunMgr
void TNTRun::UpdateSettings(Settings settings, Playground playground) {
    _settings = settings;
    _playground = playground;
}

// Summon participants, called by TNTRun
void TNTRun::SummonPlayers() {
    for (auto player : _info.players) {
        player->SaveRecallPosition();
        Position pos = _playground.center;
        pos.m_positionZ += _settings.offsetZ * (_settings.levels - 1) + 2;
        player->TeleportTo(WorldLocation(_settings.map->GetEntry()->MapID, pos));
    }
}

void TNTRun::RecallPlayer(Player* player) {
    player->Recall();
    if (player->isDead())
        player->ResurrectPlayer(1.0f);
}

// Spawn platforms, called by TNTRun
void TNTRun::SpawnObjects() {
    for (uint8 _Z = 0; _Z < _settings.levels; ++_Z)
        for (uint8 _Y = 0; _Y < _settings.sizeX; ++_Y)
            for (uint8 _X = 0; _X < _settings.sizeY; ++_X) {
                // Create a Position using the current platform height, row and column offsets
                Position offsetd(_X * _settings.offsetX, _Y * _settings.offsetY, _Z * _settings.offsetZ, 0);
                // Sum up the base spawn point coords to the offset to get the offseted value for the current platform
                offsetd.m_positionX += _settings.origin.GetPositionX();
                offsetd.m_positionY += _settings.origin.GetPositionY();
                offsetd.m_positionZ += _settings.origin.GetPositionZ();
                // Spawn platform using the offset'ed spawnPoint value
                SpawnObjectAt(offsetd);
            }
}

// Spawn platform object at given Position, called by TNTRun
void TNTRun::SpawnObjectAt(Position position) {
    // Create a new gameobject
    GameObject* object = new GameObject;
    // Generate a new map GUID for it
    uint32 guidLow = _settings.map->GenerateLowGuid<HighGuid::GameObject>();
    QuaternionData rot = QuaternionData::fromEulerAnglesZYX(_settings.origin.GetOrientation(), 0.f, 0.f);
    // Create object and update fields based on gameobject data
    if (!object->Create(guidLow, _settings.objectId, _settings.map, PHASEMASK_NORMAL, position, rot, 0, GO_STATE_READY))
    {
        // Deallocate the object in case of failure and return
        delete object;
        ReportState(TNTRun::State::EVENT_NOTREADY, "Failed to create object!");
        return;
    }
    // Adds the worldobject to the map
    _settings.map->AddToMap(object);
    // Keep track of the each object
    _objects.push_back(object);
}

// Report state change to TNTRunMgr, called by TNTRun and TNTRunMgr
void TNTRun::ReportState(State state, std::string reason) {
    sTNTRunMgr->UpdateState(state, reason);
}

// Called every world tick
void TNTRun::Update(uint32 diff) {
    RunChecks(diff);
}

// Checks warmup time and messages participants about the remaining time until the event starts
void TNTRun::CheckWarmup(uint32 diff) {
    static uint32 secCount = 0;
    warmupTimer += diff;
    secCount += diff;
    if (secCount >= 1000) {
        std::string msg;
        AnnounceToParticipants(Trinity::StringFormat("TNT Run starting in %u...", (_settings.warmupTime - warmupTimer) / 1000 + 1));
        secCount = 0;
    }
    if (warmupTimer >= _settings.warmupTime) {
        ReportState(State::EVENT_INPROGRESS, "Warmup finished.");
        AnnounceToParticipants("TNT Run has started!");
    }
}

// Announce a message to all participants and also spectators
void TNTRun::AnnounceToParticipants(std::string msg) {
    for (auto p : _info.players) {
        p->GetSession()->SendAreaTriggerMessage(msg.c_str());
    }
    for (auto p : _info.spectators) {
        p->GetSession()->SendAreaTriggerMessage(msg.c_str());
    }
}

//void TNTRun::Check

// 
void TNTRun::RunChecks(uint32 diff) {
    // Warmup time checks
    if (sTNTRunMgr->GetEventState() == State::EVENT_STARTING) {
        CheckWarmup(diff);
        // Teleport players back on top
        for (auto it = _info.players.begin(); it != _info.players.end(); ++it) {
            // Ignore if player is being teleported
            if ((*it)->IsBeingTeleported())
                continue;
            if ((*it)->GetPositionZ() < _settings.origin.GetPositionZ()) {
                Position pos = _playground.center;
                pos.m_positionZ += _settings.offsetZ * (_settings.levels - 1) + 2;
                (*it)->TeleportTo(WorldLocation(_settings.map->GetEntry()->MapID, pos));
            }
        }
    }
    else if (sTNTRunMgr->GetEventState() == State::EVENT_INPROGRESS) {
        CheckWinCondition();
        CheckPlayersPos();
    }
}

void TNTRun::CheckWinCondition() {
    // Return if there's not exactly 1 player left
    if (_info.players.size() != 1)
        return;
    auto it = _info.players.begin();
    AnnounceToParticipants(Trinity::StringFormat("|cffff0000%s|r won the event!", (*it)->GetName()));
    sWorld->SendWorldText(3, Trinity::StringFormat("|cffff0000%s|r won the TNT Run event!", (*it)->GetName()));
    (*it)->AddItem(_settings.reward_entry, _settings.reward_count);
    ReportState(State::EVENT_FINISHED, "Event completed.");
}

void TNTRun::CheckLeaversCount() {
    if (_info.leaversCount + 1 >= _info.playerCount / 2) {
        AnnounceToParticipants("Too many players left! Event cancelled.");
        ReportState(State::EVENT_FINISHED, "Too many players left.");
    }
}
// Checks the postions of participants, will 
void TNTRun::CheckPlayersPos() {
    auto it = _info.spectators.begin();
    while (it != _info.spectators.end()) {
        if (!IsOnPlayground(*it)) {
            RecallPlayer(*it);
            it = _info.spectators.erase(it);
        }
        else
            ++it;
    }
    it = _info.players.begin();
    while (it != _info.players.end()) {
        if (!IsOnPlayground(*it)) {
            AnnounceToParticipants(Trinity::StringFormat("|cffff0000%s|r left the playground!", (*it)->GetName().c_str()));
            RecallPlayer(*it);
            it = _info.players.erase(it);
        }
        else if ((*it)->GetPositionZ() < _settings.map->GetHeight(_settings.origin) + 1) {
            AnnounceToParticipants(Trinity::StringFormat("%s just fell to his death!", (*it)->GetName().c_str()));
            /*ClearGossipMenuFor((*it));
            AddGossipItemFor((*it), 1, "", (*it)->GetGUID().GetCounter(), 0, "Would you like to spectate?|n|cffff0000Warning:|r Going to far away from the platforms will remove you from the event!", 0, false);
            (*it)->PlayerTalkClass->GetGossipMenu().SetMenuId(537);
            SendGossipMenuFor((*it), DEFAULT_GOSSIP_MESSAGE, (*it)->GetGUID());*/
            RecallPlayer(*it);
            it = _info.players.erase(it);
        }
        else
            ++it;
    }
}

void TNTRun::HandlePlayerLogout(Player* player) {
    bool participant = false;
    if (auto it_alive = GetAliveIterFor(player); it_alive != _info.players.end()) {
        _info.players.erase(it_alive);
        ++_info.leaversCount;
        participant = true;
    }
    if (auto it_dead = GetSpectatorIterFor(player); it_dead != _info.spectators.end()) {
        _info.spectators.erase(it_dead);
        participant = true;
    }
    if (!participant)
        return;
    WorldLocation loc(player->m_homebindMapId, player->m_homebindX, player->m_homebindY, player->m_homebindZ, player->GetOrientation());
    SQLTransaction dummy;
    Player::SavePositionInDB(loc, player->m_homebindAreaId, player->GetGUID(), dummy);
}

std::vector<Player*>::const_iterator TNTRun::GetAliveIterFor(Player* player) {
    
    return std::find(_info.players.begin(), _info.players.end(), player);
}

std::vector<Player*>::const_iterator TNTRun::GetSpectatorIterFor(Player* player) {
    return std::find(_info.spectators.begin(), _info.spectators.end(), player);
}

bool TNTRun::IsOnPlayground(Player* player) {
    if ((player->GetMap() != _settings.map) || (!player->IsWithinBox(_playground.center, _playground.maxX, _playground.maxY, _playground.maxZ)))
        return false;
    return true;
}
