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
    };
    TNTRun(std::vector<Player*> players, Settings settings);
    ~TNTRun();
private:
    void SummonPlayers();
    void SpawnObjects();
    void SpawnObjectAt(Position position);
    void ReportState(State state);
    std::vector<Player*> _playersAlive;
    std::vector<Player*> _playersDead;
    std::vector<GameObject*> _objects;
    Settings _settings;
};

#endif
