#include "config.hpp"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>

static constexpr const char* kConfigDir = "ux0:/data/mister-companion-vita";
static constexpr const char* kConfigPath = "ux0:/data/mister-companion-vita/config.ini";

static std::string trim(const std::string& value) {
    size_t start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

AppConfig ConfigStore::load() {
    AppConfig config;
    FILE* file = fopen(kConfigPath, "r");
    if (!file) return config;

    char line[512];
    int activeProfile = -1;

    while (fgets(line, sizeof(line), file)) {
        std::string text = trim(line);
        if (text.empty()) continue;

        if (text.front() == '[' && text.back() == ']') {
            std::string section = trim(text.substr(1, text.size() - 2));
            activeProfile = -1;
            const std::string prefix = "profile.";
            if (section.rfind(prefix, 0) == 0) {
                int index = std::atoi(section.substr(prefix.size()).c_str());
                if (index >= 0) {
                    if (static_cast<int>(config.profiles.size()) <= index) config.profiles.resize(index + 1);
                    activeProfile = index;
                }
            }
            continue;
        }

        size_t separator = text.find('=');
        if (separator == std::string::npos) continue;

        std::string key = trim(text.substr(0, separator));
        std::string value = trim(text.substr(separator + 1));

        if (activeProfile >= 0 && activeProfile < static_cast<int>(config.profiles.size())) {
            ConnectionProfile& profile = config.profiles[activeProfile];
            if (key == "name") profile.name = value;
            else if (key == "host") profile.host = value;
            else if (key == "username") profile.username = value.empty() ? "root" : value;
            else if (key == "password") profile.password = value.empty() ? "1" : value;
            continue;
        }

        if (key == "name") config.name = value;
        else if (key == "host") config.host = value;
        else if (key == "username") config.username = value.empty() ? "root" : value;
        else if (key == "password") config.password = value.empty() ? "1" : value;
    }

    fclose(file);

    std::vector<ConnectionProfile> validProfiles;
    for (ConnectionProfile profile : config.profiles) {
        profile.name = trim(profile.name);
        profile.host = trim(profile.host);
        profile.username = trim(profile.username);
        profile.password = trim(profile.password);
        if (profile.name.empty() || profile.host.empty()) continue;
        if (profile.username.empty()) profile.username = "root";
        if (profile.password.empty()) profile.password = "1";
        validProfiles.push_back(profile);
    }
    config.profiles = validProfiles;

    return config;
}

bool ConfigStore::save(const AppConfig& config, std::string& error) {
    mkdir("ux0:/data", 0777);
    mkdir(kConfigDir, 0777);

    FILE* file = fopen(kConfigPath, "w");
    if (!file) {
        error = "Unable to open config file for writing.";
        return false;
    }

    fprintf(file, "name=%s\n", config.name.c_str());
    fprintf(file, "host=%s\n", config.host.c_str());
    fprintf(file, "username=%s\n", config.username.c_str());
    fprintf(file, "password=%s\n", config.password.c_str());

    for (size_t i = 0; i < config.profiles.size(); ++i) {
        const ConnectionProfile& profile = config.profiles[i];
        if (trim(profile.name).empty() || trim(profile.host).empty()) continue;
        fprintf(file, "\n[profile.%zu]\n", i);
        fprintf(file, "name=%s\n", profile.name.c_str());
        fprintf(file, "host=%s\n", profile.host.c_str());
        fprintf(file, "username=%s\n", profile.username.c_str());
        fprintf(file, "password=%s\n", profile.password.c_str());
    }

    fclose(file);
    return true;
}
