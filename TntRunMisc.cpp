#include "custom_config.h"
#include "GameObject.h"
#include "GridNotifiersImpl.h"
#include "RBAC.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "TntRunMisc.h"
#include "TntRunMgr.h"

TNTRunObject::tntrun_object_ai::tntrun_object_ai(GameObject* go) : GameObjectAI(go) {
    _triggered = false;
    _settings.searchRadius = sCustomCfg->GetDouble("tntrun.object.searchradius", 0.7f);
    _settings.collisionX = sCustomCfg->GetDouble("tntrun.object.collisionX", 1.16f);
    _settings.collisionY = sCustomCfg->GetDouble("tntrun.object.collisionY", 1.14f);
    _settings.collisionY = sCustomCfg->GetDouble("tntrun.object.collisionZ", 0.5f);
    _settings.timer = sCustomCfg->GetInt32("tntrun.object.decaytime", 1000);
}

void TNTRunObject::tntrun_object_ai::UpdateAI(uint32 diff) {
    if (_triggered) {
        if (sTNTRunMgr->GetStatus() != EVENT_INPROGRESS)
            return;
        if (_settings.timer < (int32)diff) {
            me->SetPhaseMask(-1, true);
            sTNTRunEvent->ReportDespawn(me);
        }
        _settings.timer -= diff;
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

bool TNTRunQueueNPC::tntrun_queuer_ai::GossipHello(Player* player) {
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "[Debug]TNTRun", GOSSIP_SENDER_MAIN, 0);
    if (sTNTRunEvent->GetStatus() == TNTRunEvent::EventState::EVENT_INPROGRESS || sTNTRunEvent->GetStatus() == TNTRunEvent::EventState::EVENT_STARTING || sTNTRunEvent->GetStatus() == TNTRunEvent::EventState::EVENT_FINISHED)
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "TNT Run in progress!|nElapsed time: NOT IMPLEMENTED", GOSSIP_SENDER_MAIN, 1);
    else if (sTNTRunEvent->GetStatus() == TNTRunEvent::EventState::EVENT_BADSETTINGS || sTNTRunEvent->GetStatus() == TNTRunEvent::EventState::EVENT_NOTREADY)
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cffff0000TNT Run is not available right now. Try again later.|r", GOSSIP_SENDER_MAIN, 2);

    else
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Queue for TNT Run.", GOSSIP_SENDER_MAIN, 3);
    player->TalkedToCreature(me->GetEntry(), me->GetGUID());
    SendGossipMenuFor(player, 1, me->GetGUID());
    return true;
}

bool TNTRunQueueNPC::tntrun_queuer_ai::GossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) {
    uint32 const action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
    ClearGossipMenuFor(player);
    if (action == 3) {
        if (sTNTRunEvent->QueuePlayer(player))
            player->GetSession()->SendAreaTriggerMessage("You have been queued for TNT Run!");
        else
            player->GetSession()->SendAreaTriggerMessage("|cffff0000Couldn't add you to TNT Run queue. Please try again later.|r");
    }
    else {
        player->GetSession()->SendAreaTriggerMessage("|cffff0000Unhandled case.|r");
    }
    CloseGossipMenuFor(player);
    return true;
}

std::vector<ChatCommand> TNTRunCommands::GetCommands() const {
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

bool TNTRunCommands::HandleSettingsCmd(ChatHandler* handler, char const* /*args*/) {
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

bool TNTRunCommands::HandleStatusCmd(ChatHandler* handler, char const* /*args*/) {
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
    case TNTRunEvent::EventState::EVENT_BADSETTINGS:
        status += "|cff00ff00BADSETTINGS|r";
        break;
    default:
        status += "|cffff0000UNDEFINED - REPORT TO DEV|r";
    }
    handler->PSendSysMessage(status.c_str());
    return true;
}

bool TNTRunCommands::HandleStartCmd(ChatHandler* handler, char const* /*args*/) {
    sTNTRunEvent->Start();
    return true;
}

bool TNTRunCommands::HandleStopCmd(ChatHandler* handler, char const* /*args*/) {
    sTNTRunEvent->Stop();
    return true;
}

void TNTRunQueueHelper::OnLogout(Player* player) {
    sTNTRunMgr->RemoveFromQueue(player);
}

// ---------------- WIP ----------------
class TNTRunEventHelper : public WorldScript {
public:
    TNTRunEventHelper() : WorldScript("tntrunevent_helper") { }

    void OnUpdate(uint32 diff) override {
        sTNTRunEvent->Update(diff);
    }
};

void AddSC_tntrun_misc()
{
    new TNTRunCommands();
    new TNTRunObject();
    new TNTRunQueueNPC();
    new TNTRunQueueHelper();
}
