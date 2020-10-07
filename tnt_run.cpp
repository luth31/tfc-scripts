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

class TNTRun {
public:
    TNTRun(uint32 platformEntry, Position const& pos, uint32 mapId, uint32 size) : platformEntry(platformEntry), posX(pos.GetPositionX()), posY(pos.GetPositionY()), posZ(pos.GetPositionZ()), posO(pos.GetOrientation()), size(size) {
        map = sMapMgr->FindMap(mapId, 0);
        started = false;
    }

    void Start() {
        if (started == true)
            return;
        SpawnPlatforms();
        started = true;
    }

    void Stop() {
        if (started == false)
            return;
        for (auto p : platforms) {
            if (!p)
                continue;
            p->SetRespawnTime(0);
            p->Delete();
        }
        platforms.clear();
        started = false;
    }

    void RegisterDespawn(GameObject* go) {
        auto it = std::find(platforms.begin(), platforms.end(), go);
        if (it != platforms.end())
            platforms.erase(it);
    }

    ~TNTRun() {
        Stop();
    }
private:
    void SpawnPlatforms() {
        for (uint8 i = 0; i < size; ++i) {
            for (uint8 j = 0; j < size; ++j) {
                // Change the 0, 10, 20 to change height between platforms
                SpawnPlatform(-(1.5 * j), -(1.5 * i), 0);
                SpawnPlatform(-(1.5 * j), -(1.5 * i), 10);
                SpawnPlatform(-(1.5 * j), -(1.5 * i), 20);
            }
        }
    }
    void SpawnPlatform(float offsetX, float offsetY, float offsetZ) {
        GameObject* object = new GameObject;
        uint32 guidLow = map->GenerateLowGuid<HighGuid::GameObject>();
        QuaternionData rot = QuaternionData::fromEulerAnglesZYX(posO, 0.f, 0.f);
        if (!object->Create(guidLow, platformEntry, map, PHASEMASK_NORMAL, Position(posX + offsetX, posY + offsetY, posZ + offsetZ, posO), rot, 0, GO_STATE_READY))
        {
            delete object;
            return;
        }
        map->AddToMap(object);
        platforms.push_back(object);
    }
    float posX;
    float posY;
    float posZ;
    float posO;
    bool started;
    Map* map;
    uint32 platformEntry;
    std::vector<GameObject*> platforms;
    uint32 size;
};

TNTRun* t = nullptr;

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
