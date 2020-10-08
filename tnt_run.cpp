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

class TNTRunEvent {
public:
    struct EventSize {
        uint8 sizeX;
        uint8 sizeY;
        uint8 levels;
        float platformOffsetX;
        float platformOffsetY;
        float levelOffsetZ;
    };

    enum EventState {
        EVENT_NOTSTARTED,
        EVENT_STARTING,
        EVENT_INPROGRESS,
        EVENT_FINISHED
    };
    TNTRunEvent(uint32 platformEntry, Position const& pos, uint32 mapId, EventSize size, std::list<Player*> players) : baseObjectEntry(platformEntry), spawnPoint(pos), sizes(size), eventParticipants(players) {
        map = sMapMgr->FindMap(mapId, 0);
        state = EVENT_NOTSTARTED;
    }

    void Start() {
        if (state == EVENT_NOTSTARTED) {
            SpawnPlatforms();
            state = EVENT_INPROGRESS;
        }
    }

    // Method called by each platform object before despawning; removes the object from the platforms internal list
    void ReportDespawn(GameObject* go) {
        auto it = std::find(platformsList.begin(), platformsList.end(), go);
        if (it != platformsList.end())
            platformsList.erase(it);
    }

    ~TNTRunEvent() {
        for (auto p : platformsList) {
            // Disable object respawn; stops some bugs with the map object removelist
            p->SetRespawnTime(0);
            // Queue the object for deletion and deallocation
            p->Delete();
        }
    }
private:
    // Generate the platforms
    void SpawnPlatforms() {
        for (uint8 _Z = 0; _Z < sizes.levels; ++_Z)
            for (uint8 _Y = 0; _Y < sizes.sizeX; ++_Y)
                for (uint8 _X = 0; _X < sizes.sizeY; ++_X) {
                    // Create a Position using the current platform height, row and column offsets
                    Position offsetd(_X * sizes.platformOffsetX, _Y * sizes.platformOffsetY, _Z * sizes.levelOffsetZ, 0);
                    // Sum up the base spawn point coords to the offset to get the offseted value for the current platform
                    offsetd.m_positionX += spawnPoint.GetPositionX();
                    offsetd.m_positionY += spawnPoint.GetPositionY();
                    offsetd.m_positionZ += spawnPoint.GetPositionZ();
                    // Spawn platform using the offset'ed spawnPoint value
                    SpawnPlatform(offsetd);
                } 
    }

    void SpawnPlatform(Position const& pos) {
        // Create a new gameobject
        GameObject* object = new GameObject;
        // Generate a new map GUID for it
        uint32 guidLow = map->GenerateLowGuid<HighGuid::GameObject>();
        QuaternionData rot = QuaternionData::fromEulerAnglesZYX(spawnPoint.GetOrientation(), 0.f, 0.f);
        // Create object and update fields based on gameobject data
        if (!object->Create(guidLow, baseObjectEntry, map, PHASEMASK_NORMAL, pos, rot, 0, GO_STATE_READY))
        {
            // Deallocate the object in case of failure and return
            delete object;
            return;
        }
        // Adds the worldobject to the map
        map->AddToMap(object);
        // Keep track of the each platform in platformList
        platformsList.push_back(object);
    }

    float heightOffset; // The height between the 3 platforms
    Position spawnPoint; // The center of the upmost left platform; starting point of event grids generation
    Map* map; // MapID where event will be located
    uint32 baseObjectEntry; // entryID of the gameobject used to generate the platforms
    std::vector<GameObject*> platformsList; // List of all 
    EventSize sizes; // Struct containing all sizes
    EventState state; // Current event state
    std::list<Player*> eventParticipants;
};

static TNTRunEvent* tntRunEvent = nullptr;

class TNTPlatform : public GameObjectScript {
public:
    TNTPlatform() : GameObjectScript("tnt_platform") { }

    struct tnt_platform_ai : public GameObjectAI {
        bool triggered;
        int32 timer;
        tnt_platform_ai(GameObject* go) : GameObjectAI(go) {
            timer = 1000;
            triggered = false;
        }

        void UpdateAI(uint32 diff) override {
            if (triggered) {
                if (timer < (int32)diff) {
                    me->SetRespawnTime(0);
                    me->Delete();
                    t->RegisterDespawn(me);
                }
                timer -= diff;
                return;
            }
            std::list<Player*> nearbyPlayers;
            Trinity::AnyPlayerInObjectRangeCheck check(me, 0.7f, true);
            Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(me, nearbyPlayers, check);
            Cell::VisitWorldObjects(me, searcher, 0.7f);
            if (nearbyPlayers.empty())
                return;
            for (std::list<Player*>::const_iterator it = nearbyPlayers.begin(); it != nearbyPlayers.end(); ++it) {
                // Good variations: (1.11f, 1.09f, 0.5f)
                if ((*it)->IsWithinBox(me->GetPosition(), 1.16f, 1.14f, 0.5f)) { 
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

class tnt_run_commandscript : public CommandScript {
public:
    tnt_run_commandscript() : CommandScript("tnt_run_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override {
        static std::vector<ChatCommand> tntCommandTable = {
            { "start",	rbac::RBAC_PERM_COMMAND_ACCOUNT, true,  &HandleTntRunStart,	  ""},
            { "stop",	rbac::RBAC_PERM_COMMAND_ACCOUNT, true,  &HandleTntRunStop,	  ""},
        };
        static std::vector<ChatCommand> commandTable = {
            { "tntrun", rbac::RBAC_PERM_COMMAND_ACCOUNT, true,  nullptr,              "",  tntCommandTable },
        };
        return commandTable;
    }

    static bool HandleTntRunStart(ChatHandler* handler, char const* args) {
        handler->PSendSysMessage("Starting TNT Run...");
        if (t != nullptr) {
            handler->PSendSysMessage("TNT Run already started!!!");
            return false;
        }
        // First param is platformEntry; Change Position parameters to change coords, the param after Position is the mapID, next one is number of platforms per side (10 would mean 10^2=100 platforms)
        t = new TNTRun(800000, Position(65.f, 95.f, -100.f, 0.f), 13, 10);
        t->Start();
        handler->PSendSysMessage("TNT Run started!");
        return true;
    }

    static bool HandleTntRunStop(ChatHandler* handler, char const* args) {
        handler->PSendSysMessage("Stopping TNT Run...");
        if (t == nullptr) {
            handler->PSendSysMessage("TNT Run already stopped!!!");
            return false;
        }
        t->Stop();
        delete t;
        t = nullptr;
        handler->PSendSysMessage("TNT Run stopped!");
        return true;
    }
private:
};

void AddSC_tntrun()
{
    new TNTPlatform();
    new tnt_run_commandscript();
}
