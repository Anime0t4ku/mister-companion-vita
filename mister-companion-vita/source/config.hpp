#pragma once

#include <string>
#include <vector>

struct ConnectionProfile {
    std::string name;
    std::string host;
    std::string username = "root";
    std::string password = "1";
};

struct AppConfig {
    std::string name = "MiSTer";
    std::string host;
    std::string username = "root";
    std::string password = "1";
    std::vector<ConnectionProfile> profiles;
};

class ConfigStore {
public:
    static AppConfig load();
    static bool save(const AppConfig& config, std::string& error);
};
