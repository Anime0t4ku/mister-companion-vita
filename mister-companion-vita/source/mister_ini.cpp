#include "mister_ini.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>

namespace {

const char* DefaultFontLine = ";font=font/myfont.pf";
const char* AmigaVisionPresetKey = "__amigavision_preset";
const char* MenuCrtPresetKey = "__menu_crt_preset";

const std::vector<std::pair<std::string, std::string>> ResolutionMap = {
    {"0", "1280x720@60"}, {"1", "1024x768@60"}, {"2", "720x480@60"}, {"3", "720x576@50"},
    {"4", "1280x1024@60"}, {"5", "800x600@60"}, {"6", "640x480@60"}, {"7", "1280x720@50"},
    {"8", "1920x1080@60"}, {"9", "1920x1080@50"}, {"10", "1366x768@60"}, {"11", "1024x600@60"},
    {"12", "1920x1440@60"}, {"13", "2048x1536@60"}, {"14", "2560x1440@60"},
};

const std::vector<std::pair<std::string, std::string>> ScalingMap = {
    {"0", "Disabled"}, {"1", "Low Latency"}, {"2", "Exact Refresh"},
};

const std::vector<std::string> AmigaVisionPresetHeaderLines = {
    "[Amiga", "+Amiga500", "+Amiga500HD", "+Amiga600HD", "+AmigaCD32]",
};

const std::vector<std::string> AmigaVisionPresetBodyLines = {
    "video_mode_ntsc=8 ; These two use the recommended setting of 1080p60 and",
    "video_mode_pal=9  ; 1080p50, adjust if you want a different resolution",
    "vscale_mode=0",
    "vsync_adjust=1 ; You can set this to 2 if your display can handle it",
    "custom_aspect_ratio_1=40:27",
    "bootscreen=0",
};

std::vector<std::string> amigaVisionPresetBlockLines() {
    std::vector<std::string> lines = AmigaVisionPresetHeaderLines;
    lines.insert(lines.end(), AmigaVisionPresetBodyLines.begin(), AmigaVisionPresetBodyLines.end());
    return lines;
}

const std::map<std::string, std::vector<std::string>> MenuCrtPresets = {
    {"NTSC, Large Text", {"[Menu]", "vga_scaler=1", "video_mode=384,16,37,63,224,10,3,24,7830"}},
    {"NTSC, Small Text", {"[Menu]", "vga_scaler=1", "video_mode=640,30,60,70,240,4,4,14,12587"}},
    {"PAL, Large Text", {"[Menu]", "vga_scaler=1", "video_mode=320,13,31,52,288,4,3,18,6510"}},
    {"PAL, Small Text", {"[Menu]", "vga_scaler=1", "video_mode=640,30,60,70,288,6,4,14,12587"}},
};

std::string trim(std::string value) {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\r' || value.front() == '\n' || value.front() == '\t')) value.erase(value.begin());
    while (!value.empty() && (value.back() == ' ' || value.back() == '\r' || value.back() == '\n' || value.back() == '\t')) value.pop_back();
    return value;
}

std::string basename(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    return pos == std::string::npos ? path : path.substr(pos + 1);
}

std::vector<std::string> splitLines(const std::string& rawText) {
    std::string text = rawText;
    std::replace(text.begin(), text.end(), '\r', '\n');

    std::vector<std::string> lines;
    std::stringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) lines.push_back(line);
    return lines;
}

std::string normalizedLine(const std::string& line) {
    return trim(line);
}

std::string lineWithoutComment(const std::string& rawLine) {
    std::string line = trim(rawLine);
    size_t pos = line.find(';');
    if (pos != std::string::npos) line = trim(line.substr(0, pos));
    return line;
}

bool isSectionStart(const std::string& line) {
    std::string stripped = normalizedLine(line);
    return !stripped.empty() && stripped.front() == '[';
}

bool isSingleLineSection(const std::string& line) {
    std::string stripped = normalizedLine(line);
    return !stripped.empty() && stripped.front() == '[' && stripped.back() == ']';
}

bool isMisterSectionHeader(const std::string& line) {
    return normalizedLine(line) == "[MiSTer]";
}

bool isMenuSectionHeader(const std::string& line) {
    return normalizedLine(line) == "[Menu]";
}

bool isAmigaVisionHeaderAt(const std::vector<std::string>& lines, int index) {
    if (index < 0 || index + static_cast<int>(AmigaVisionPresetHeaderLines.size()) > static_cast<int>(lines.size())) return false;
    for (size_t offset = 0; offset < AmigaVisionPresetHeaderLines.size(); offset++) {
        if (normalizedLine(lines[index + offset]) != AmigaVisionPresetHeaderLines[offset]) return false;
    }
    return true;
}

int amigaVisionBlockEndIndex(const std::vector<std::string>& lines, int startIndex) {
    int index = startIndex + static_cast<int>(AmigaVisionPresetHeaderLines.size());
    while (index < static_cast<int>(lines.size())) {
        std::string stripped = normalizedLine(lines[index]);
        if (index > startIndex + static_cast<int>(AmigaVisionPresetHeaderLines.size()) && isSectionStart(lines[index])) break;
        index++;
        if (lineWithoutComment(stripped) == "bootscreen=0") break;
    }
    return index;
}

bool hasAmigaVisionPreset(const std::string& text) {
    std::vector<std::string> lines = splitLines(text);
    for (int index = 0; index < static_cast<int>(lines.size()); index++) {
        if (!isAmigaVisionHeaderAt(lines, index)) continue;
        int end = amigaVisionBlockEndIndex(lines, index);
        std::map<std::string, std::string> found;
        for (int i = index; i < end; i++) {
            std::string clean = lineWithoutComment(lines[i]);
            size_t eq = clean.find('=');
            if (eq == std::string::npos) continue;
            found[trim(clean.substr(0, eq))] = trim(clean.substr(eq + 1));
        }
        if (found["video_mode_ntsc"] == "8" && found["video_mode_pal"] == "9" && found["vscale_mode"] == "0" &&
            found["vsync_adjust"] == "1" && found["custom_aspect_ratio_1"] == "40:27" && found["bootscreen"] == "0") return true;
    }
    return false;
}

std::vector<std::string> removeExistingAmigaVisionPresetBlocks(const std::vector<std::string>& lines) {
    std::vector<std::string> cleaned;
    int index = 0;
    while (index < static_cast<int>(lines.size())) {
        if (isAmigaVisionHeaderAt(lines, index)) {
            index = amigaVisionBlockEndIndex(lines, index);
            while (!cleaned.empty() && trim(cleaned.back()).empty()) cleaned.pop_back();
            while (index < static_cast<int>(lines.size()) && trim(lines[index]).empty()) index++;
            continue;
        }
        cleaned.push_back(lines[index]);
        index++;
    }
    return cleaned;
}

std::pair<int, int> menuSectionBounds(const std::vector<std::string>& lines) {
    for (int index = 0; index < static_cast<int>(lines.size()); index++) {
        if (!isMenuSectionHeader(lines[index])) continue;
        int start = index;
        index++;
        while (index < static_cast<int>(lines.size()) && !isSectionStart(lines[index])) index++;
        return {start, index};
    }
    return {-1, -1};
}

std::map<std::string, std::string> readSectionSettings(const std::vector<std::string>& lines, int start, int end) {
    std::map<std::string, std::string> settings;
    if (start < 0 || end < 0) return settings;
    for (int i = start + 1; i < end; i++) {
        std::string clean = lineWithoutComment(lines[i]);
        size_t eq = clean.find('=');
        if (eq == std::string::npos) continue;
        settings[trim(clean.substr(0, eq))] = trim(clean.substr(eq + 1));
    }
    return settings;
}

std::string detectMenuCrtPreset(const std::string& text) {
    std::vector<std::string> lines = splitLines(text);
    auto bounds = menuSectionBounds(lines);
    if (bounds.first < 0) return "Disabled";
    auto settings = readSectionSettings(lines, bounds.first, bounds.second);
    if (settings["vga_scaler"] != "1") return "Disabled";
    std::string videoMode = settings["video_mode"];
    for (const auto& preset : MenuCrtPresets) {
        std::string expected;
        for (const std::string& line : preset.second) {
            std::string clean = lineWithoutComment(line);
            if (clean.rfind("video_mode=", 0) == 0) {
                expected = trim(clean.substr(11));
                break;
            }
        }
        if (videoMode == expected) return preset.first;
    }
    return "Disabled";
}

std::vector<std::string> removeExistingMenuSection(const std::vector<std::string>& lines) {
    auto bounds = menuSectionBounds(lines);
    if (bounds.first < 0 || bounds.second < 0) return lines;
    std::vector<std::string> cleaned;
    cleaned.insert(cleaned.end(), lines.begin(), lines.begin() + bounds.first);
    cleaned.insert(cleaned.end(), lines.begin() + bounds.second, lines.end());
    while (!cleaned.empty() && trim(cleaned.back()).empty()) cleaned.pop_back();
    return cleaned;
}

std::string valueForKey(const std::vector<std::pair<std::string, std::string>>& items, const std::string& key, const std::string& fallback) {
    for (const auto& item : items) if (item.first == key) return item.second;
    return fallback;
}

std::string keyForValue(const std::vector<std::pair<std::string, std::string>>& items, const std::string& value, const std::string& fallback) {
    for (const auto& item : items) if (item.second == value) return item.first;
    return fallback;
}

std::map<std::string, std::string> parseMisterIni(const std::string& text) {
    std::map<std::string, std::string> settings;
    settings["amigavision_preset"] = hasAmigaVisionPreset(text) ? "Enabled" : "Disabled";
    settings["menu_crt_preset"] = detectMenuCrtPreset(text);

    std::string currentSection;
    for (const std::string& rawLine : splitLines(text)) {
        std::string line = trim(rawLine);
        if (line.empty()) continue;

        if (isSectionStart(line)) {
            if (isMisterSectionHeader(line)) currentSection = "MiSTer";
            else if (isSingleLineSection(line)) currentSection = line.substr(1, line.size() - 2);
            else currentSection.clear();
            continue;
        }

        if (currentSection != "MiSTer") continue;

        if (!line.empty() && line.front() == ';') {
            std::string commentBody = trim(line.substr(1));
            size_t eq = commentBody.find('=');
            if (eq != std::string::npos) {
                std::string key = trim(commentBody.substr(0, eq));
                std::string value = trim(commentBody.substr(eq + 1));
                if (key == "font") settings["font_commented"] = value;
            }
            continue;
        }

        size_t semi = line.find(';');
        if (semi != std::string::npos) line = trim(line.substr(0, semi));
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        settings[trim(line.substr(0, eq))] = trim(line.substr(eq + 1));
    }
    return settings;
}

std::pair<std::string, std::string> splitAssignmentLine(const std::string& line) {
    std::string stripped = trim(line);
    if (!stripped.empty() && stripped.front() == ';') stripped = trim(stripped.substr(1));
    size_t eq = stripped.find('=');
    if (eq == std::string::npos) return {"", ""};
    return {trim(stripped.substr(0, eq)), trim(stripped.substr(eq + 1))};
}

std::string lineIndent(const std::string& line) {
    size_t pos = line.find_first_not_of(" \t");
    return pos == std::string::npos ? "" : line.substr(0, pos);
}

std::string formatSettingLine(const std::string& existingLine, const std::string& key, const std::string& value, bool commented = false) {
    return lineIndent(existingLine) + (commented ? ";" : "") + key + "=" + value;
}

void appendMissingSettings(std::vector<std::string>& newLines, const std::map<std::string, std::string>& updatedSettings, std::vector<std::string>& replacedKeys) {
    auto isReplaced = [&](const std::string& key) {
        return std::find(replacedKeys.begin(), replacedKeys.end(), key) != replacedKeys.end();
    };
    auto addReplaced = [&](const std::string& key) {
        if (!isReplaced(key)) replacedKeys.push_back(key);
    };

    for (const auto& item : updatedSettings) {
        const std::string& key = item.first;
        const std::string& value = item.second;
        if (!key.empty() && key.rfind("__", 0) == 0) continue;
        if (isReplaced(key)) continue;
        if (key == "font_commented") {
            if (isReplaced("font") || isReplaced("font_commented")) continue;
            newLines.push_back(DefaultFontLine);
            addReplaced("font");
            addReplaced("font_commented");
        } else {
            newLines.push_back(key + "=" + value);
            addReplaced(key);
        }
    }
}

void appendAmigaVisionPresetBlock(std::vector<std::string>& newLines, const std::map<std::string, std::string>& updatedSettings) {
    auto it = updatedSettings.find(AmigaVisionPresetKey);
    if (it == updatedSettings.end() || it->second != "Enabled") return;
    if (!newLines.empty() && !trim(newLines.back()).empty()) newLines.push_back("");
    std::vector<std::string> block = amigaVisionPresetBlockLines();
    newLines.insert(newLines.end(), block.begin(), block.end());
}

void appendMenuCrtPresetBlock(std::vector<std::string>& newLines, const std::map<std::string, std::string>& updatedSettings) {
    auto it = updatedSettings.find(MenuCrtPresetKey);
    if (it == updatedSettings.end()) return;
    auto preset = MenuCrtPresets.find(it->second);
    if (preset == MenuCrtPresets.end()) return;
    if (!newLines.empty() && !trim(newLines.back()).empty()) newLines.push_back("");
    newLines.insert(newLines.end(), preset->second.begin(), preset->second.end());
}

std::map<std::string, std::string> buildEasyModeSettings(const MisterIniEasyValues& values) {
    std::map<std::string, std::string> settings;
    settings["direct_video"] = values.hdmiMode == "Direct Video (CRT / Scaler)" ? "1" : "0";
    settings["video_mode"] = keyForValue(ResolutionMap, values.resolution, "8");
    settings["vsync_adjust"] = keyForValue(ScalingMap, values.scaling, "1");
    settings["dvi_mode"] = values.hdmiAudio == "Enabled" ? "0" : "1";
    settings["hdr"] = values.hdr == "Enabled" ? "1" : "0";
    settings["hdmi_limited"] = values.hdmiLimited == "Limited Range" ? "1" : "0";

    if (values.analogue == "RGBS (SCART)") {
        settings["vga_mode"] = "rgb"; settings["composite_sync"] = "1"; settings["vga_sog"] = "0"; settings["vga_scaler"] = "0"; settings["forced_scandoubler"] = "0";
    } else if (values.analogue == "RGBHV (VGA 15 kHz)") {
        settings["vga_mode"] = "rgb"; settings["composite_sync"] = "0"; settings["vga_sog"] = "0"; settings["vga_scaler"] = "0"; settings["forced_scandoubler"] = "0";
    } else if (values.analogue == "RGsB (Sync-on-Green)") {
        settings["vga_mode"] = "rgb"; settings["composite_sync"] = "1"; settings["vga_sog"] = "1"; settings["vga_scaler"] = "0"; settings["forced_scandoubler"] = "0";
    } else if (values.analogue == "YPbPr (Component)") {
        settings["vga_mode"] = "ypbpr"; settings["composite_sync"] = "0"; settings["vga_sog"] = "1"; settings["vga_scaler"] = "0"; settings["forced_scandoubler"] = "0";
    } else if (values.analogue == "S-Video") {
        settings["vga_mode"] = "svideo"; settings["composite_sync"] = "1"; settings["vga_sog"] = "0"; settings["vga_scaler"] = "0"; settings["forced_scandoubler"] = "0";
    } else if (values.analogue == "Composite (CVBS)") {
        settings["vga_mode"] = "cvbs"; settings["composite_sync"] = "1"; settings["vga_sog"] = "0"; settings["vga_scaler"] = "0"; settings["forced_scandoubler"] = "0";
    } else if (values.analogue == "VGA Scaler (31 kHz+)") {
        settings["vga_mode"] = "rgb"; settings["composite_sync"] = "0"; settings["vga_sog"] = "0"; settings["vga_scaler"] = "1"; settings["forced_scandoubler"] = "0";
    }

    settings["logo"] = values.logo == "Enabled" ? "1" : "0";
    if (!values.font.empty() && values.font != "Default") settings["font"] = "font/" + values.font;
    else settings["font_commented"] = "font/myfont.pf";
    settings[AmigaVisionPresetKey] = values.amigavisionPreset == "Enabled" ? "Enabled" : "Disabled";
    settings[MenuCrtPresetKey] = MenuCrtPresets.count(values.menuCrtPreset) ? values.menuCrtPreset : "Disabled";
    return settings;
}

} // namespace

namespace MisterIni {

std::vector<std::string> resolutionOptions() { std::vector<std::string> out; for (const auto& item : ResolutionMap) out.push_back(item.second); return out; }
std::vector<std::string> scalingOptions() { return {"Disabled", "Low Latency", "Exact Refresh"}; }
std::vector<std::string> hdmiModeOptions() { return {"HD Output (Default)", "Direct Video (CRT / Scaler)"}; }
std::vector<std::string> hdmiAudioOptions() { return {"Enabled", "Disabled (DVI Mode)"}; }
std::vector<std::string> hdrOptions() { return {"Disabled", "Enabled"}; }
std::vector<std::string> hdmiRangeOptions() { return {"Full Range", "Limited Range"}; }
std::vector<std::string> analogueOptions() { return {"RGBS (SCART)", "RGBHV (VGA 15 kHz)", "RGsB (Sync-on-Green)", "YPbPr (Component)", "S-Video", "Composite (CVBS)", "VGA Scaler (31 kHz+)"}; }
std::vector<std::string> enabledDisabledOptions() { return {"Enabled", "Disabled"}; }
std::vector<std::string> menuCrtPresetOptions() { return {"Disabled", "NTSC, Large Text", "NTSC, Small Text", "PAL, Large Text", "PAL, Small Text"}; }

std::string normalizeIniFilename(const std::string& filename) {
    std::string name = basename(trim(filename));
    if (name == "MiSTer.ini") return name;
    if (name.rfind("MiSTer_", 0) == 0 && name.size() > 9 && name.substr(name.size() - 4) == ".ini") return name;
    return "";
}

std::vector<std::string> sortIniFiles(const std::vector<std::string>& files) {
    std::vector<std::string> unique;
    for (const std::string& file : files) {
        std::string name = normalizeIniFilename(file);
        if (!name.empty() && std::find(unique.begin(), unique.end(), name) == unique.end()) unique.push_back(name);
    }
    std::sort(unique.begin(), unique.end(), [](const std::string& a, const std::string& b) {
        if (a == "MiSTer.ini" && b != "MiSTer.ini") return true;
        if (a != "MiSTer.ini" && b == "MiSTer.ini") return false;
        std::string la = a; std::string lb = b;
        std::transform(la.begin(), la.end(), la.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        std::transform(lb.begin(), lb.end(), lb.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        return la < lb;
    });
    return unique;
}

std::string normalizeIniText(const std::string& rawText, bool ensureTrailingNewline) {
    std::vector<std::string> lines = splitLines(rawText);
    for (std::string& line : lines) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) line.pop_back();
    }
    std::string out;
    bool previousBlank = false;
    for (const std::string& line : lines) {
        bool blank = trim(line).empty();
        if (blank && previousBlank) continue;
        out += line;
        out += "\n";
        previousBlank = blank;
    }
    while (!out.empty() && out.back() == '\n') out.pop_back();
    if (ensureTrailingNewline) out += "\n";
    return out;
}

MisterIniEasyValues easyValuesFromIniText(const std::string& text) {
    std::map<std::string, std::string> settings = parseMisterIni(text);
    MisterIniEasyValues values;
    values.hdmiMode = (settings["direct_video"] == "1" || settings["direct_video"] == "2") ? "Direct Video (CRT / Scaler)" : "HD Output (Default)";
    values.resolution = valueForKey(ResolutionMap, settings["video_mode"], "1920x1080@60");
    values.scaling = valueForKey(ScalingMap, settings["vsync_adjust"].empty() ? "1" : settings["vsync_adjust"], "Low Latency");
    values.hdmiAudio = settings["dvi_mode"] == "1" ? "Disabled (DVI Mode)" : "Enabled";
    values.hdr = settings["hdr"] == "1" ? "Enabled" : "Disabled";
    values.hdmiLimited = settings["hdmi_limited"] == "1" ? "Limited Range" : "Full Range";

    std::string vgaMode = settings["vga_mode"].empty() ? "rgb" : settings["vga_mode"];
    std::transform(vgaMode.begin(), vgaMode.end(), vgaMode.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    std::string compositeSync = settings["composite_sync"].empty() ? "1" : settings["composite_sync"];
    std::string vgaSog = settings["vga_sog"].empty() ? "0" : settings["vga_sog"];
    std::string vgaScaler = settings["vga_scaler"].empty() ? "0" : settings["vga_scaler"];
    std::string forcedScandoubler = settings["forced_scandoubler"].empty() ? "0" : settings["forced_scandoubler"];

    if (vgaMode == "rgb" && compositeSync == "1" && vgaSog == "0" && vgaScaler == "0" && forcedScandoubler == "0") values.analogue = "RGBS (SCART)";
    else if (vgaMode == "rgb" && compositeSync == "0" && vgaSog == "0" && vgaScaler == "0" && forcedScandoubler == "0") values.analogue = "RGBHV (VGA 15 kHz)";
    else if (vgaMode == "rgb" && compositeSync == "1" && vgaSog == "1" && vgaScaler == "0" && forcedScandoubler == "0") values.analogue = "RGsB (Sync-on-Green)";
    else if (vgaMode == "ypbpr" && compositeSync == "0" && vgaSog == "1" && vgaScaler == "0" && forcedScandoubler == "0") values.analogue = "YPbPr (Component)";
    else if (vgaMode == "svideo" && compositeSync == "1" && vgaSog == "0" && vgaScaler == "0" && forcedScandoubler == "0") values.analogue = "S-Video";
    else if (vgaMode == "cvbs" && compositeSync == "1" && vgaSog == "0" && vgaScaler == "0" && forcedScandoubler == "0") values.analogue = "Composite (CVBS)";
    else if (vgaMode == "rgb" && compositeSync == "0" && vgaSog == "0" && vgaScaler == "1" && forcedScandoubler == "0") values.analogue = "VGA Scaler (31 kHz+)";
    else values.analogue = "Custom";

    values.logo = settings["logo"] == "0" ? "Disabled" : "Enabled";
    std::string fontValue = settings["font"];
    if (fontValue.rfind("font/", 0) == 0) values.font = fontValue.substr(5);
    else values.font = "Default";
    values.amigavisionPreset = settings["amigavision_preset"].empty() ? "Disabled" : settings["amigavision_preset"];
    values.menuCrtPreset = settings["menu_crt_preset"].empty() ? "Disabled" : settings["menu_crt_preset"];
    return values;
}

std::string updateIniText(const std::string& iniText, const MisterIniEasyValues& values) {
    std::map<std::string, std::string> updatedSettings = buildEasyModeSettings(values);
    std::string text = iniText;
    std::replace(text.begin(), text.end(), '\r', '\n');
    bool hadTrailingNewline = !text.empty() && text.back() == '\n';
    std::vector<std::string> lines = splitLines(text);
    if (hadTrailingNewline && !lines.empty() && lines.back().empty()) lines.pop_back();

    lines = removeExistingAmigaVisionPresetBlocks(lines);
    lines = removeExistingMenuSection(lines);

    std::vector<std::string> newLines;
    bool inMisterSection = false;
    bool foundMisterSection = false;
    bool postMisterBlocksInserted = false;
    std::vector<std::string> replacedKeys;
    auto isReplaced = [&](const std::string& key) {
        return std::find(replacedKeys.begin(), replacedKeys.end(), key) != replacedKeys.end();
    };
    auto addReplaced = [&](const std::string& key) {
        if (!isReplaced(key)) replacedKeys.push_back(key);
    };

    for (const std::string& line : lines) {
        std::string stripped = trim(line);
        if (isSectionStart(stripped)) {
            if (inMisterSection && !isMisterSectionHeader(stripped)) {
                appendMissingSettings(newLines, updatedSettings, replacedKeys);
                if (!postMisterBlocksInserted) {
                    appendAmigaVisionPresetBlock(newLines, updatedSettings);
                    appendMenuCrtPresetBlock(newLines, updatedSettings);
                    postMisterBlocksInserted = true;
                }
                if (!newLines.empty() && !trim(newLines.back()).empty()) newLines.push_back("");
            }
            inMisterSection = isMisterSectionHeader(stripped);
            if (inMisterSection) foundMisterSection = true;
            newLines.push_back(line);
            continue;
        }

        if (inMisterSection) {
            auto assignment = splitAssignmentLine(line);
            const std::string& key = assignment.first;
            if (!key.empty()) {
                if (key == "font") {
                    auto fontIt = updatedSettings.find("font");
                    if (fontIt != updatedSettings.end()) {
                        newLines.push_back(formatSettingLine(line, "font", fontIt->second, false));
                        addReplaced("font"); addReplaced("font_commented");
                        continue;
                    }
                    auto fontCommentIt = updatedSettings.find("font_commented");
                    if (fontCommentIt != updatedSettings.end()) {
                        newLines.push_back(DefaultFontLine);
                        addReplaced("font"); addReplaced("font_commented");
                        continue;
                    }
                }
                auto it = updatedSettings.find(key);
                if (it != updatedSettings.end() && key != "font_commented") {
                    newLines.push_back(formatSettingLine(line, key, it->second, false));
                    addReplaced(key);
                    continue;
                }
            }
        }
        newLines.push_back(line);
    }

    if (!foundMisterSection) {
        if (!newLines.empty() && !trim(newLines.back()).empty()) newLines.push_back("");
        newLines.push_back("[MiSTer]");
        appendMissingSettings(newLines, updatedSettings, replacedKeys);
        appendAmigaVisionPresetBlock(newLines, updatedSettings);
        appendMenuCrtPresetBlock(newLines, updatedSettings);
    } else if (inMisterSection) {
        appendMissingSettings(newLines, updatedSettings, replacedKeys);
        if (!postMisterBlocksInserted) {
            appendAmigaVisionPresetBlock(newLines, updatedSettings);
            appendMenuCrtPresetBlock(newLines, updatedSettings);
        }
    }

    std::string out;
    for (size_t i = 0; i < newLines.size(); i++) {
        out += newLines[i];
        out += "\n";
    }
    while (!out.empty() && out.back() == '\n') out.pop_back();
    out += "\n";
    return out;
}

} // namespace MisterIni
