#include "CustomConfig.h"
#include "DatabaseEnv.h"
#include "ScriptMgr.h"
#include <unordered_map>
#include <boost/lexical_cast.hpp>

using std::vector;
using std::pair;
using std::string;

CustomConfigManager* CustomConfigManager::getInstance() {
    static CustomConfigManager instance;
    return &instance;
}

uint32 CustomConfigManager::GetUInt32(string key, uint32 defaultValue) {
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

int32 CustomConfigManager::GetInt32(string key, uint32 defaultValue) {
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


string CustomConfigManager::GetString(string key, string defaultValue) {
    auto value = GetValueForKey(key);
    if (value == configMap.end())
        return defaultValue;
    return value->second;
}

double CustomConfigManager::GetDouble(string key, double defaultValue) {
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

vector<pair<string,string>> CustomConfigManager::BatchGetString(vector<string> keys) {
    vector<pair<string, string>> result;
    for (auto it = keys.begin(); it != keys.end(); ++it) {
        auto value = configMap.find(*it);
        if (value == configMap.end())
            result.push_back({ *it, "" });
        else
            result.push_back({ *it, value->second });
    }
    return result;
}

void CustomConfigManager::ReloadConfig() {
    configMap.clear();
    QueryResult result = WorldDatabase.PQuery("SELECT name,value FROM custom_config");
    if (result) {
        do {
            Field* fields = result->Fetch();
            pair<string, string> configEntry;
            configEntry.first = fields[0].GetString();
            configEntry.second = fields[1].GetString();
            configMap.insert(configEntry);
        } while (result->NextRow());
    }
}

ConfigMap::const_iterator CustomConfigManager::GetValueForKey(string key) {
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
