#include "GameObject.h"
#include "Log.h"
#include "TntRun.h"
#include "TntRunMgr.h"
#include "TntRunMisc.h"
#include "ScriptMgr.h"

TNTRun::TNTRun(std::vector<Player*> players, Settings settings) : _settings(settings) {
    TC_LOG_DEBUG("event.tntrun", "[TNTRunMgr] Invalid update state! %s", reason);
    ReportState(TNTRun::State::EVENT_STARTING);
    _playersAlive.insert(_playersAlive.end(), players.begin(), players.end());
    SpawnObjects();
    SummonPlayers();
}

TNTRun::~TNTRun() {
    for (auto obj : _objects) {
        obj->SetRespawnTime(0);
        obj->Delete();
    }
}

void TNTRun::SummonPlayers() {
    for (auto player : _playersAlive) {
        // ToDo: Get coords for summoning
    }
}

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
        ReportState(TNTRun::State::EVENT_NOTREADY);
        return;
    }
    // Adds the worldobject to the map
    _settings.map->AddToMap(object);
    // Keep track of the each object
    _objects.push_back(object);
}

void TNTRun::ReportState(State state) {
    sTNTRunMgr->UpdateState(state);
}
