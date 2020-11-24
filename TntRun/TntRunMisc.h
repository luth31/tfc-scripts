#ifndef __TNTRUNMISC_H__
#define __TNTRUNMISC_H__
#include "Chat.h"
#include "GameObjectAI.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "TntRun.h"

class TNTRunObject : public GameObjectScript {
public:
    struct Settings {
        double searchRadius;
        double collisionX;
        double collisionY;
        double collisionZ;
        uint32 timer;
    };
    TNTRunObject() : GameObjectScript("tnt_platform") { }
    struct tntrun_object_ai : public GameObjectAI {
        tntrun_object_ai(GameObject* go);
        void UpdateAI(uint32 diff) override;
        bool _triggered;
        TNTRunObject::Settings _settings;
        uint32 _timer;
    };
    GameObjectAI* GetAI(GameObject* go) const override { return new tntrun_object_ai(go); }
};

class TNTRunQueueNPC : public CreatureScript {
public:
    TNTRunQueueNPC() : CreatureScript("tntrun_queuer") { }
    struct tntrun_queuer_ai : public ScriptedAI {
        tntrun_queuer_ai(Creature* creature) : ScriptedAI(creature) { }
        bool GossipHello(Player* player) override;
        bool GossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override;
    };
    CreatureAI* GetAI(Creature* creature) const override { return new tntrun_queuer_ai(creature); }
};

class TNTRunCommands : public CommandScript {
public:
    TNTRunCommands() : CommandScript("tntrun_commands") { }
    std::vector<ChatCommand> GetCommands() const override;
    static bool HandleSettingsCmd(ChatHandler* handler, char const* /*args*/);
    static bool HandleStatusCmd(ChatHandler* handler, char const* /*args*/);
    static bool HandleStartCmd(ChatHandler* handler, char const* /*args*/);
    static bool HandleStopCmd(ChatHandler* handler, char const* /*args*/);
};

class TNTRunQueueHelper : public PlayerScript {
public:
    TNTRunQueueHelper() : PlayerScript("tntrun_queue_helper") { }
    void OnLogout(Player* player) override;
};

#endif
