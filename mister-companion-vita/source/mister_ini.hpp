#pragma once

#include <map>
#include <string>
#include <vector>

struct MisterIniEasyValues {
    std::string hdmiMode = "HD Output (Default)";
    std::string resolution = "1920x1080@60";
    std::string scaling = "Low Latency";
    std::string hdmiAudio = "Enabled";
    std::string hdr = "Disabled";
    std::string hdmiLimited = "Full Range";
    std::string analogue = "RGBS (SCART)";
    std::string logo = "Enabled";
    std::string font = "Default";
    std::string amigavisionPreset = "Disabled";
    std::string menuCrtPreset = "Disabled";
};

namespace MisterIni {
    std::vector<std::string> resolutionOptions();
    std::vector<std::string> scalingOptions();
    std::vector<std::string> hdmiModeOptions();
    std::vector<std::string> hdmiAudioOptions();
    std::vector<std::string> hdrOptions();
    std::vector<std::string> hdmiRangeOptions();
    std::vector<std::string> analogueOptions();
    std::vector<std::string> enabledDisabledOptions();
    std::vector<std::string> menuCrtPresetOptions();

    std::string normalizeIniFilename(const std::string& filename);
    std::vector<std::string> sortIniFiles(const std::vector<std::string>& files);
    std::string normalizeIniText(const std::string& text, bool ensureTrailingNewline = true);

    MisterIniEasyValues easyValuesFromIniText(const std::string& text);
    std::string updateIniText(const std::string& iniText, const MisterIniEasyValues& values);
}
