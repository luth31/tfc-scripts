#ifndef __CUSTOM_CONFIG_H__
#define __CUSTOM_CONFIG_H__

typedef std::unordered_map<std::string, std::string> ConfigMap;

class CustomConfigManager {
public:
    // Delete
    CustomConfigManager(CustomConfigManager const&) = delete;
    CustomConfigManager& operator=(CustomConfigManager const&) = delete;

    // Instance getter, singleton pattern
    static CustomConfigManager* getInstance();

    // Config data getters
    uint32 GetUInt32(std::string key, uint32 defaultValue);
    int32 GetInt32(std::string key, uint32 defaultValue);
    std::string GetString(std::string key, std::string defaultValue);
    double GetDouble(std::string key, double defaultValue);

    // Reloading
    void ReloadConfig();
private:
    CustomConfigManager() { }
    ConfigMap::const_iterator GetValueForKey(std::string key);
    ConfigMap configMap;
};

#define sCustomCfg CustomConfigManager::getInstance()
#endif
