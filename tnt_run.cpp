/*
	Tnt Run event script prototype
	Copyright Luth (github.com/luth31)
	Usage not allowed
	All rights reserved
*/

#include "Chat.h"
#include "Language.h"
#include "Log.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "World.h"
#include "WorldSession.h"
#include "GameObjectData.h"
#include "GameObject.h"
#include "MapManager.h"
#include "ObjectMgr.h"
#include "GameObjectAI.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include <iostream>
#include <unordered_map>

#include "custom_config.h"

class TNTRunEvent {
public:
    struct EventSettings {
        uint32 objectId; // the entry Id of the object that will be used as a "floor" for the platforms
        Map* map;
        Position origin;
        uint8 sizeX;
        uint8 sizeY;
        uint8 levels;
        double offsetX;
        double offsetY;
        double offsetZ;
        uint32 minPlayers;
        uint32 maxPlayers;
    };

    enum EventState {
        EVENT_NOTREADY,
        EVENT_READY,
        EVENT_INPROGRESS,
        EVENT_BADSETTINGS
    };

    static TNTRunEvent* getInstance() {
        static TNTRunEvent instance;
        return &instance;
    }

    void Start() {
        if (state == EVENT_READY) {
            LoadSettings();
            SpawnPlatforms();
            state = EVENT_INPROGRESS;
        }
    }

    void Stop() {
        if (state != EVENT_INPROGRESS)
            return;
        for (auto p : platformsList) {
            p->SetRespawnTime(0);
            p->Delete();
        }
        state = EVENT_READY;
    }

    EventState GetStatus() {
        return state;
    }

    // Method called by each platform object before despawning; removes the object from the platforms internal list
    void ReportDespawn(GameObject* go) {
        auto it = std::find(platformsList.begin(), platformsList.end(), go);
        if (it != platformsList.end())
            platformsList.erase(it);
    }

    const EventSettings GetSettings() {
        LoadSettings(); // Reload settings before returning a copy, otherwise old settings will get returned until the event is started
        return _settings;
    }

    void Update(uint32 diff) {
        return;
    }

    void QueuePlayer(Player* player) {

    }

    void RemoveFromQueue(Player* player) {

    }

    bool IsInQueue(Player* player) {

    }

private:
    TNTRunEvent() : state(EVENT_NOTREADY) {
        LoadSettings();
    }

    void ValidateSettings() {
        bool valid = true;
        // If map is invalid
        if (_settings.map == nullptr)
            valid = false;
        if (sObjectMgr->GetGameObjectTemplate(_settings.objectId) == nullptr)
            valid = false;
        if (_settings.minPlayers > _settings.maxPlayers || _settings.minPlayers == 0)
            valid = false;
        if (valid)
            state = EVENT_READY;
        else
            state = EVENT_BADSETTINGS;
    }

    void LoadSettings() {
        state = EVENT_NOTREADY; // Make event not ready until we load all the settings, let the validator set the state after that
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

    // Generate the platforms
    void SpawnPlatforms() {
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
                    SpawnPlatform(offsetd);
                } 
    }

    void SpawnPlatform(Position const& pos) {
        // Create a new gameobject
        GameObject* object = new GameObject;
        // Generate a new map GUID for it
        uint32 guidLow = _settings.map->GenerateLowGuid<HighGuid::GameObject>();
        QuaternionData rot = QuaternionData::fromEulerAnglesZYX(_settings.origin.GetOrientation(), 0.f, 0.f);
        // Create object and update fields based on gameobject data
        if (!object->Create(guidLow, _settings.objectId, _settings.map, PHASEMASK_NORMAL, pos, rot, 0, GO_STATE_READY))
        {
            // Deallocate the object in case of failure and return
            delete object;
            return;
        }
        // Adds the worldobject to the map
        _settings.map->AddToMap(object);
        // Keep track of the each platform in platformList
        platformsList.push_back(object);
    }

    EventSettings _settings;
    std::vector<GameObject*> platformsList; // List of all 
    EventState state; // Current event state
    std::list<std::pair<Player*, bool>> eventParticipants;
};

#define sTNTRunEvent TNTRunEvent::getInstance()

class TNTPlatformObject : public GameObjectScript {
public:
    TNTPlatformObject() : GameObjectScript("tnt_platform") { }

    struct tnt_platform_ai : public GameObjectAI {
        struct ObjectSettings {
            double searchRadius;
            double collisionX;
            double collisionY;
            double collisionZ;
            int32 timer;
        } _settings;
        bool triggered;

        tnt_platform_ai(GameObject* go) : GameObjectAI(go) {
            triggered = false;
            // Another batch of good defaults: // Good variations: (0.7f, 1.11f, 1.09f, 0.5f)
            _settings.searchRadius = sCustomCfg->GetDouble("tntrun.object.searchradius", 0.7f);
            _settings.collisionX = sCustomCfg->GetDouble("tntrun.object.collisionX", 1.16f);
            _settings.collisionY = sCustomCfg->GetDouble("tntrun.object.collisionY", 1.14f);
            _settings.collisionY = sCustomCfg->GetDouble("tntrun.object.collisionZ", 0.5f);
            _settings.timer = sCustomCfg->GetInt32("tntrun.object.decaytime", 1000);
        }

        void UpdateAI(uint32 diff) override {
            if (triggered) {
                if (_settings.timer < (int32)diff) {
                    me->SetRespawnTime(0);
                    me->Delete();
                    sTNTRunEvent->ReportDespawn(me);
                }
                _settings.timer -= diff;
                return;
            }
            std::list<Player*> nearbyPlayers;
            Trinity::AnyPlayerInObjectRangeCheck check(me, _settings.searchRadius, true);
            Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(me, nearbyPlayers, check);
            Cell::VisitWorldObjects(me, searcher, _settings.searchRadius);
            if (nearbyPlayers.empty())
                return;
            for (std::list<Player*>::const_iterator it = nearbyPlayers.begin(); it != nearbyPlayers.end(); ++it) {

                if ((*it)->IsWithinBox(me->GetPosition(), _settings.collisionX, _settings.collisionY, _settings.collisionZ)) {
                    triggered = true;
                    return;
                }
            }
        }
    };

    GameObjectAI* GetAI(GameObject* go) const override
    {
        return new tnt_platform_ai(go);
    }
};

class TNTRunEvent_NPC : public CreatureScript {
public:
    TNTRunEvent_NPC() : CreatureScript("tntrun_npc") { }

    struct TNTRunEvent_NPC_AI : public ScriptedAI {
        TNTRunEvent_NPC_AI(Creature* creature) : ScriptedAI(creature) {

        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new TNTRunEvent_NPC_AI(creature);
    }
};

class TestCmd : public CommandScript {
public:
    TestCmd() : CommandScript("test_cmd") { }
    std::vector<ChatCommand> GetCommands() const override {
        static std::vector<ChatCommand> TNTRunCommands =
        {
            {"start", rbac::RBAC_PERM_COMMAND_ACCOUNT, true, &HandleStartCmd, "Start the TNTRun event manually"},
            {"stop", rbac::RBAC_PERM_COMMAND_ACCOUNT, true, &HandleStopCmd, "Start the TNTRun event manually"},
            {"status", rbac::RBAC_PERM_COMMAND_ACCOUNT, true, &HandleStatusCmd, "Shows the current status of the TNTRun event."},
            {"settings", rbac::RBAC_PERM_COMMAND_ACCOUNT, true, &HandleSettingsCmd, "Show current TNTRun settings."},

        };
        static std::vector<ChatCommand> commandTable =
        {
            { "tntrun", rbac::RBAC_PERM_COMMAND_ACCOUNT, false,  nullptr, "Show TNTRun subcommands.", TNTRunCommands},
        };
        return commandTable;
    }

    static bool HandleSettingsCmd(ChatHandler* handler, char const* /*args*/) {
        auto settings = sTNTRunEvent->GetSettings();
        handler->PSendSysMessage("TNTRun Settings:");
        handler->PSendSysMessage("Object ID: |cff0066ff%u|r", settings.objectId);
        handler->PSendSysMessage("MapID: |cff0066ff%u|r", settings.map->GetEntry()->MapID);
        handler->PSendSysMessage("Origin X: |cff0066ff%f|r", settings.origin.GetPositionX());
        handler->PSendSysMessage("Origin Y: |cff0066ff%f|r", settings.origin.GetPositionY());
        handler->PSendSysMessage("Origin Z: |cff0066ff%f|r", settings.origin.GetPositionZ());
        handler->PSendSysMessage("Origin O: |cff0066ff%f|r", settings.origin.GetOrientation());
        handler->PSendSysMessage("Size X: |cff0066ff%u|r", settings.sizeX);
        handler->PSendSysMessage("Size Y: |cff0066ff%u|r", settings.sizeY);
        handler->PSendSysMessage("Levels: |cff0066ff%u|r", settings.levels);
        handler->PSendSysMessage("Offset X: |cff0066ff%f|r", settings.offsetX);
        handler->PSendSysMessage("Offset Y: |cff0066ff%f|r", settings.offsetY);
        handler->PSendSysMessage("Offset Z: |cff0066ff%f|r", settings.offsetZ);
        return true;
    }

    static bool HandleStatusCmd(ChatHandler* handler, char const* /*args*/) {
        std::string status = "Current TNTRun status: ";
        switch (sTNTRunEvent->GetStatus()) {
            case TNTRunEvent::EventState::EVENT_READY:
                status += "|cffffff00READY|r";
                break;
            case TNTRunEvent::EventState::EVENT_INPROGRESS:
                status += "|cff00ccccIN PROGRESS|r";
                break;
            case TNTRunEvent::EventState::EVENT_NOTREADY:
                status += "|cff00ff00NOT READY|r";
                break;
            default:
                status += "|cffff0000UNDEFINED - REPORT TO DEV|r";
        }
        handler->PSendSysMessage(status.c_str());
        return true;
    }

    static bool HandleStartCmd(ChatHandler* handler, char const* /*args*/) {
        sTNTRunEvent->Start();
        return true;
    }

    static bool HandleStopCmd(ChatHandler* handler, char const* /*args*/) {
        sTNTRunEvent->Stop();
        return true;
    }
};

class TNTRunEventHelper : public WorldScript {
public:
    TNTRunEventHelper() : WorldScript("tntrunevent_helper") { }

    void OnUpdate(uint32 diff) override {
        sTNTRunEvent->Update(diff);
    }
};

class TNTRunQueueHelper : public PlayerScript {
public:
    TNTRunQueueHelper() : PlayerScript("tntrunqueue_helper") { }

    void OnLogout(Player* player) override {
        auto plr = sTNTRunEvent;
    }
};

void AddSC_tntrun()
{
    new TestCmd();
    new TNTPlatformObject();
    new TNTRunEvent_NPC();
}
