#include "custom_config.h"
#include "DatabaseEnv.h"
#include "ScriptMgr.h"
#include <unordered_map>
#include <boost/lexical_cast.hpp>

CustomConfigManager* CustomConfigManager::getInstance() {
    static CustomConfigManager instance;
    return &instance;
}

uint32 CustomConfigManager::GetUInt32(std::string key, uint32 defaultValue) {
    auto value = GetValueForKey(key);
    if (value == configMap.end())
        return defaultValue;
    try {
        return boost::lexical_cast<uint32>(value->second);
    }
    catch (boost::bad_lexical_cast& /*e*/) {
        return defaultValue;
    }
}

int32 CustomConfigManager::GetInt32(std::string key, uint32 defaultValue) {
    auto value = GetValueForKey(key);
    if (value == configMap.end())
        return defaultValue;
    try {
        return boost::lexical_cast<int32>(value->second);
    }
    catch (boost::bad_lexical_cast& /*e*/) {
        return defaultValue;
    }
}


std::string CustomConfigManager::GetString(std::string key, std::string defaultValue) {
    auto value = GetValueForKey(key);
    if (value == configMap.end())
        return defaultValue;
    return value->second;
}

double CustomConfigManager::GetDouble(std::string key, double defaultValue) {
    auto value = GetValueForKey(key);
    if (value == configMap.end())
        return defaultValue;
    try {
        return boost::lexical_cast<double>(value->second);
    }
    catch (boost::bad_lexical_cast& /*e*/) {
        return defaultValue;
    }
}

void CustomConfigManager::ReloadConfig() {
    configMap.clear();
    QueryResult result = WorldDatabase.PQuery("SELECT name,value FROM custom_config");
    if (result) {
        do {
            Field* fields = result->Fetch();
            std::pair<std::string, std::string> configEntry;
            configEntry.first = fields[0].GetString();
            configEntry.second = fields[1].GetString();
            configMap.insert(configEntry);
        } while (result->NextRow());
    }
}

ConfigMap::const_iterator CustomConfigManager::GetValueForKey(std::string key) {
    return configMap.find(key);
}

// Hook OnConfigLoad of TC
class CustomConfigLoader: public WorldScript {
public:
    CustomConfigLoader() : WorldScript("customconfig_loader") { }

    void OnConfigLoad(bool /*reload*/) override {
        sCustomCfg->ReloadConfig();
    }
};


void AddSC_CustomConfig()
{
    new CustomConfigLoader();
}
