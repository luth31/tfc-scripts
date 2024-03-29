#ifndef __TNTRUN_H__
#define __TNTRUN_H__
#include "Player.h"
#include "ScriptMgr.h"

class TNTRun {
public:
    enum State {
        EVENT_NOTREADY,
        EVENT_READY,
        EVENT_STARTING,
        EVENT_INPROGRESS,
        EVENT_FINISHED
    };
    struct Settings {
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
        uint32 queueDelay;
        uint32 warmupTime;
        uint32 reward_entry;
        uint32 reward_count;
        uint32 pacify_spell;
        uint32 anti_afk;
    };
    struct Playground {
        double maxX;
        double maxY;
        double maxZ;
        Position center;
    };
    struct GameInfo {
        std::vector<Player*> players;
        std::vector<Player*> spectators;
        uint32 leaversCount; // Players that left count; ignores spectators
        uint32 playerCount; // Starting player count
    };
    void Start(std::vector<Player*> players);
    void UpdateSettings(Settings settings, Playground playground);
    void Stop();
    void Update(uint32 diff);
    void HandlePlayerLogout(Player* player);
    uint32 GetAliveCount();
private:
    void SummonPlayers();
    void RecallPlayer(Player* player);
    void SpawnObjects();
    void SpawnObjectAt(Position position);
    void ReportState(State state, std::string reason = "");
    void CheckWarmup(uint32 diff);
    void RunChecks(uint32 diff);
    void AnnounceToParticipants(std::string msg);
    void CheckAFK(uint32 diff);
    bool IsOnPlayground(Player* player);
    void CheckWinCondition();
    void CheckPlayersPos();
    void CheckLeaversCount();
    std::vector<Player*>::const_iterator GetAliveIterFor(Player* player);
    std::vector<Player*>::const_iterator GetSpectatorIterFor(Player* player);
    std::vector<GameObject*> _objects;
    std::unordered_map<Player*, std::pair<Position, uint32>> AntiAFK_data;
    Settings _settings;
    Playground _playground;
    GameInfo _info;
    uint32 warmupTimer;
    uint32 _startTime;
};

#endif
