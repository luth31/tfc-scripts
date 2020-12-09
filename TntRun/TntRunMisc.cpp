#include "../CustomConfig/CustomConfig.h"
#include "GameTime.h"
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
    _settings.collisionZ = sCustomCfg->GetDouble("tntrun.object.collisionZ", 1.f);
    _settings.timer = sCustomCfg->GetInt32("tntrun.object.decaytime", 1000);
    _timer = 0;
}

void TNTRunObject::tntrun_object_ai::UpdateAI(uint32 diff) {
    if (sTNTRunMgr->GetEventState() != TNTRun::State::EVENT_INPROGRESS)
        return;
    if (_triggered) {
        if (_timer > _settings.timer) {
            me->SetPhaseMask(0, true);
        }
        _timer += diff;
    }
    std::list<Player*> nearbyPlayers;
    Trinity::AnyPlayerInObjectRangeCheck check(me, _settings.searchRadius, true);
    Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(me, nearbyPlayers, check);
    Cell::VisitWorldObjects(me, searcher, _settings.searchRadius);
    if (nearbyPlayers.empty())
        return;
    for (std::list<Player*>::const_iterator it = nearbyPlayers.begin(); it != nearbyPlayers.end(); ++it) {

        if ((*it)->IsWithinBox(me->GetPosition(), _settings.collisionX, _settings.collisionY, _settings.collisionZ)) {
            _triggered = true;
            return;
        }
    }
}

bool TNTRunQueueNPC::tntrun_queuer_ai::GossipHello(Player* player) {
    auto settings = sTNTRunMgr->GetSettings();

    AddGossipItemFor(player, GOSSIP_ICON_CHAT, Trinity::StringFormat("TNT Run\nIn queue: |cff3333ff%u/%u|r\nMinimum: |cffffff00%u|r\n", sTNTRunMgr->GetQueueSize(), settings.maxPlayers, settings.minPlayers), GOSSIP_SENDER_MAIN, 0);
    switch (sTNTRunMgr->GetEventState()) {
    case TNTRun::State::EVENT_STARTING:
    case TNTRun::State::EVENT_INPROGRESS:
    case TNTRun::State::EVENT_FINISHED:
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, Trinity::StringFormat("TNT Run in progress!|nElapsed time: NOT IMPLEMENTED"), GOSSIP_SENDER_MAIN, 2);
        if (player->IsGameMaster())
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cffff0000[GM]Stop event.|r", GOSSIP_SENDER_MAIN, 10);
        break;
    case TNTRun::State::EVENT_READY:
        if (sTNTRunMgr->IsInQueue(player)) {
            std::string queueMsg = Trinity::StringFormat("In queue for |cffff0000%u|r seconds.|n|cffff0000Click to leave.|r", (GameTime::GetGameTime() - sTNTRunMgr->GetQueueTimeFor(player)));
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, queueMsg, GOSSIP_SENDER_MAIN, 100);
        }
        else
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Join queue.", GOSSIP_SENDER_MAIN, 1);
        break;
    case TNTRun::State::EVENT_NOTREADY:
    default:
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cffff0000TNT Run is not available right now. Try again later.|r", GOSSIP_SENDER_MAIN, 3);
    }
    player->TalkedToCreature(me->GetEntry(), me->GetGUID());
    SendGossipMenuFor(player, 1, me->GetGUID());
    return true;
}

bool TNTRunQueueNPC::tntrun_queuer_ai::GossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) {
    uint32 const action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
    ClearGossipMenuFor(player);
    TNTRunQueue::Result result;
    switch (action) {
    case 0:
        break;
    case 1:
        result = sTNTRunMgr->AddToQueue(player);
        if (result == TNTRunQueue::Result::QUEUE_JOINED)
            player->GetSession()->SendAreaTriggerMessage("You have been queued for TNT Run!");
        else
            player->GetSession()->SendAreaTriggerMessage("|cffff0000Couldn't queue you! Report this message.|r");
        break;
    case 10:
        if (player->IsGameMaster()) {
            player->GetSession()->SendAreaTriggerMessage("|cffff0000Stopping event...|r");
            sTNTRunMgr->StopEvent();
        }
        break;
    case 100:
        result = sTNTRunMgr->RemoveFromQueue(player);
        if (result == TNTRunQueue::Result::QUEUE_LEFT)
            player->GetSession()->SendAreaTriggerMessage("You have been removed from TNT Run queue!");
        else
            player->GetSession()->SendAreaTriggerMessage("|cffff0000mCouldn't unqueue you! Report this message.|r");
        break;
    default:
        player->GetSession()->SendAreaTriggerMessage("|cffff0000Unhandled case.|r");
    }
    CloseGossipMenuFor(player);
    return true;
}

std::vector<ChatCommand> TNTRunCommands::GetCommands() const {
    static std::vector<ChatCommand> TNTRunCommands =
    {
        {"settings", rbac::RBAC_PERM_COMMAND_ACCOUNT, true, &HandleSettingsCmd, "Show current TNTRun settings."},
    };
    static std::vector<ChatCommand> commandTable =
    {
        { "tntrun", rbac::RBAC_PERM_COMMAND_ACCOUNT, false,  nullptr, "Show TNTRun subcommands.", TNTRunCommands},
    };
    return commandTable;
}

bool TNTRunCommands::HandleSettingsCmd(ChatHandler* handler, char const* /*args*/) {
    sTNTRunMgr->LoadSettings();
    auto settings = sTNTRunMgr->GetSettings();
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
    switch (sTNTRunMgr->GetEventState()) {
    case TNTRun::State::EVENT_READY:
        status += "|cffffff00READY|r";
        break;
    case TNTRun::State::EVENT_STARTING:
        status += "|cff00ccccSTARTING|r";
        break;
    case TNTRun::State::EVENT_INPROGRESS:
        status += "|cff00ccccIN PROGRESS|r";
        break;
    case TNTRun::State::EVENT_FINISHED:
        status += "|cff00ccccFINISHED|r";
        break;
    case TNTRun::State::EVENT_NOTREADY:
        status += "|cff00ff00NOT READY|r";
        break;
    default:
        status += "|cffff0000UNDEFINED - REPORT TO DEV|r";
    }
    handler->PSendSysMessage(status.c_str());
    return true;
}

void TNTRunQueueHelper::OnLogout(Player* player) {
    sTNTRunMgr->RemoveFromQueue(player);
    sTNTRunMgr->HandlePlayerLogout(player);
}

class TNTRunEventHelper : public WorldScript {
public:
    TNTRunEventHelper() : WorldScript("tntrunevent_helper") { }

    void OnUpdate(uint32 diff) override {
        sTNTRunMgr->Update(diff);
    }
};

class TNTRunPlayerSpectator : public PlayerScript {
public:
    TNTRunPlayerSpectator() : PlayerScript("tntrun_spectator") { }

    void OnGossipSelect(Player* player, uint32 menu_id, uint32 /*sender*/, uint32 action) override {
        if (menu_id != 537)
            return;
        if (action == 100) {
            player->BuildPlayerRepop();
            WorldPacket data(SMSG_PRE_RESURRECT, player->GetPackGUID().size());
            data << player->GetPackGUID();
            player->SendDirectMessage(&data);
            player->setDeathState(DEAD);
            player->SetHealth(1);
            player->CastSpell(player, 8326, true);
            // Make player fly
            WorldPacket flyPacket(12);
            flyPacket.SetOpcode(SMSG_MOVE_SET_CAN_FLY);
            flyPacket << player->GetPackGUID();
            flyPacket << uint32(0);
            player->SendMessageToSet(&flyPacket, true);
            player->SetSpeedRate(MOVE_FLIGHT, 3.f);
        }
        CloseGossipMenuFor(player);
    }
};

void AddSC_tntrun_misc()
{
    new TNTRunCommands();
    new TNTRunObject();
    new TNTRunQueueNPC();
    new TNTRunQueueHelper();
    new TNTRunEventHelper();
    //new TNTRunPlayerSpectator();
}
