#include "app.hpp"
#include "wallpaper_db.hpp"

#include <switch.h>
#include <vita2d.h>
#include <zlib.h>
#include <jpeglib.h>
#include <setjmp.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>

static const char* RemoteScriptPath = "/media/fat/Scripts/companion_remote.sh";
static const char* RemoteScriptUrl = "https://raw.githubusercontent.com/Anime0t4ku/mister-companion/main/mister-companion/assets/companion_remote.sh";

static constexpr int ScriptCount = 11;
static const char* UpdateAllPath = "/media/fat/Scripts/update_all.sh";
static const char* ZaparooPath = "/media/fat/Scripts/zaparoo.sh";
static const char* MigrateSdPath = "/media/fat/Scripts/migrate_sd.sh";
static const char* CifsMountPath = "/media/fat/Scripts/cifs_mount.sh";
static const char* CifsUmountPath = "/media/fat/Scripts/cifs_umount.sh";
static const char* CifsConfigPath = "/media/fat/Scripts/cifs_mount.ini";
static const char* AutoTimePath = "/media/fat/Scripts/auto_time.sh";
static const char* CdGameOrganizerPath = "/media/fat/Scripts/cd_game_organizer.sh";
static const char* DavBrowserPath = "/media/fat/Scripts/dav_browser.sh";
static const char* DavBrowserConfigPath = "/media/fat/Scripts/.config/dav_browser/dav_browser.ini";
static const char* FtpSaveSyncPath = "/media/fat/Scripts/ftp_save_sync.sh";
static const char* FtpSaveSyncConfigPath = "/media/fat/Scripts/.config/ftp_save_sync/ftp_save_sync.ini";
static const char* StaticWallpaperPath = "/media/fat/Scripts/static_wallpaper.sh";
static const char* SyncthingPath = "/media/fat/Scripts/syncthing.sh";
static const char* RaViewerPath = "/media/fat/Scripts/ra_viewer.sh";
static const char* RaViewerConfigPath = "/media/fat/Scripts/.config/ra_viewer/config.ini";
static const char* UserStartupPath = "/media/fat/linux/user-startup.sh";
static const char* DefaultMisterIniUrl = "https://raw.githubusercontent.com/Anime0t4ku/mister-companion/main/assets/MiSTer_example.ini";

static const char* UrlMigrateSd = "https://raw.githubusercontent.com/Natrox/MiSTer_Utils_Natrox/main/scripts/migrate_sd.sh";
static const char* UrlCifsMount = "https://raw.githubusercontent.com/MiSTer-devel/Scripts_MiSTer/master/cifs_mount.sh";
static const char* UrlCifsUmount = "https://raw.githubusercontent.com/MiSTer-devel/Scripts_MiSTer/master/cifs_umount.sh";
static const char* UrlAutoTime = "https://raw.githubusercontent.com/Anime0t4ku/0t4ku-mister-scripts/main/Scripts/auto_time.sh";
static const char* UrlCdGameOrganizer = "https://raw.githubusercontent.com/Anime0t4ku/0t4ku-mister-scripts/main/Scripts/cd_game_organizer.sh";
static const char* UrlDavBrowser = "https://raw.githubusercontent.com/Anime0t4ku/0t4ku-mister-scripts/main/Scripts/dav_browser.sh";
static const char* UrlFtpSaveSync = "https://raw.githubusercontent.com/Anime0t4ku/0t4ku-mister-scripts/refs/heads/main/Scripts/ftp_save_sync.sh";
static const char* UrlStaticWallpaper = "https://raw.githubusercontent.com/Anime0t4ku/0t4ku-mister-scripts/main/Scripts/static_wallpaper.sh";
static const char* UrlSyncthingScript = "https://raw.githubusercontent.com/Anime0t4ku/0t4ku-mister-scripts/main/Scripts/syncthing.sh";
static const char* UrlRaViewer = "https://raw.githubusercontent.com/Anime0t4ku/0t4ku-mister-scripts/main/Scripts/ra_viewer.sh";

static constexpr int WallpaperSourceCount = 4;
static const char* StaticWallpaperConfigDir = "/media/fat/Scripts/.config/static_wallpaper";
static const char* StaticWallpaperConfigPath = "/media/fat/Scripts/.config/static_wallpaper/selected_wallpaper.txt";
static const char* StaticWallpaperTargetJpg = "/media/fat/menu.jpg";
static const char* StaticWallpaperTargetPng = "/media/fat/menu.png";
static const char* MisterMenuReloadCommand = "echo \"load_core /media/fat/menu.rbf\" > /dev/MiSTer_cmd";

static const char* RannyDbUrl = "https://raw.githubusercontent.com/Ranny-Snice/Ranny-Snice-Wallpapers/db/db.json.zip";
static const char* PcnDbUrl = "https://raw.githubusercontent.com/Anime0t4ku/MiSTerWallpapers/db/db/pcnchallenge.json.zip";
static const char* PcnPremiumDbUrl = "https://raw.githubusercontent.com/Anime0t4ku/MiSTerWallpapers/db/db/pcnpremium.json.zip";
static const char* Ot4kuDbUrl = "https://raw.githubusercontent.com/Anime0t4ku/MiSTerWallpapers/db/db/0t4kuwallpapers.json.zip";

static const char* RannyRawBase = "https://raw.githubusercontent.com/Ranny-Snice/Ranny-Snice-Wallpapers/main/";
static const char* PcnRawBase = "https://raw.githubusercontent.com/Anime0t4ku/MiSTerWallpapers/main/";
static const char* PcnPremiumRawBase = "https://raw.githubusercontent.com/Anime0t4ku/MiSTerWallpapers/main/";
static const char* Ot4kuRawBase = "https://raw.githubusercontent.com/Anime0t4ku/MiSTerWallpapers/main/";

static constexpr int ExtraCount = 3;
static const char* RaConfigPath = "/media/fat/retroachievements.cfg";



static std::vector<std::string> splitWords(const std::string& value) {
    std::stringstream stream(value);
    std::vector<std::string> parts;
    std::string part;
    while (stream >> part) parts.push_back(part);
    return parts;
}

static std::string trim(std::string value) {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\r' || value.front() == '\n' || value.front() == '\t')) value.erase(value.begin());
    while (!value.empty() && (value.back() == ' ' || value.back() == '\r' || value.back() == '\n' || value.back() == '\t')) value.pop_back();
    return value;
}


static std::string ellipsizeText(const std::string& text, size_t maxChars) {
    if (text.size() <= maxChars) return text;
    if (maxChars <= 3) return text.substr(0, maxChars);
    return text.substr(0, maxChars - 3) + "...";
}

static std::string safeText(const std::string& value, const std::string& fallback = "Not set") {
    return value.empty() ? fallback : value;
}

static std::string subnetBaseFromIp(const std::string& ip) {
    int a = 0, b = 0, c = 0, d = 0;
    if (std::sscanf(ip.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) != 4) return "";
    if (a < 0 || a > 255 || b < 0 || b > 255 || c < 0 || c > 255) return "";
    return std::to_string(a) + "." + std::to_string(b) + "." + std::to_string(c) + ".";
}

static std::string currentSubnetBase() {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return "";

    sockaddr_in remote{};
    remote.sin_family = AF_INET;
    remote.sin_port = htons(53);
    inet_pton(AF_INET, "8.8.8.8", &remote.sin_addr);
    connect(fd, reinterpret_cast<sockaddr*>(&remote), sizeof(remote));

    sockaddr_in local{};
    socklen_t len = sizeof(local);
    std::string out;
    if (getsockname(fd, reinterpret_cast<sockaddr*>(&local), &len) == 0) {
        char buffer[INET_ADDRSTRLEN] = {0};
        if (inet_ntop(AF_INET, &local.sin_addr, buffer, sizeof(buffer))) {
            out = subnetBaseFromIp(buffer);
        }
    }
    close(fd);
    return out;
}

static bool tcpPortOpen(const std::string& host, int port, int timeoutMs) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return false;

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        close(fd);
        return false;
    }

    int rc = connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (rc == 0) {
        close(fd);
        return true;
    }

    if (errno != EINPROGRESS && errno != EWOULDBLOCK) {
        close(fd);
        return false;
    }

    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(fd, &writeSet);

    timeval tv{};
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    rc = select(fd + 1, nullptr, &writeSet, nullptr, &tv);
    if (rc <= 0) {
        close(fd);
        return false;
    }

    int error = 0;
    socklen_t len = sizeof(error);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) != 0) {
        close(fd);
        return false;
    }

    close(fd);
    return error == 0;
}

static bool hostLooksLikeMister(const std::string& host, const AppConfig& baseConfig) {
    AppConfig probeConfig = baseConfig;
    probeConfig.host = host;
    if (trim(probeConfig.username).empty()) probeConfig.username = "root";
    if (trim(probeConfig.password).empty()) probeConfig.password = "1";

    SshClient probe;
    std::string message;
    if (!probe.connect(probeConfig, message)) {
        return false;
    }

    SshResult result = probe.runCommand("test -d /media/fat -a -e /dev/MiSTer_cmd && echo MISTER || echo NOT_MISTER");
    probe.disconnect();

    return result.success && trim(result.output) == "MISTER";
}

static bool contains(const std::string& value, const std::string& needle) {
    return value.find(needle) != std::string::npos;
}

static bool textMentionsReboot(const std::string& text) {
    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lower.find("reboot") != std::string::npos ||
           lower.find("restarting") != std::string::npos ||
           lower.find("restart") != std::string::npos ||
           lower.find("going down") != std::string::npos ||
           lower.find("connection reset") != std::string::npos ||
           lower.find("broken pipe") != std::string::npos;
}

static std::string shellQuote(const std::string& value) {
    std::string out = "'";
    for (char c : value) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    out += "'";
    return out;
}

static std::string downloadCommand(const std::string& url, const std::string& path) {
    return "mkdir -p /media/fat/Scripts /media/fat/Scripts/.config/update_all && "
           "if ! command -v wget >/dev/null 2>&1; then echo wget not found.; exit 1; fi; "
           "wget --no-check-certificate -O " + path + " " + shellQuote(url) + " && "
           "test -s " + path + " && "
           "chmod +x " + path;
}


static std::vector<std::string> splitLines(const std::string& value) {
    std::vector<std::string> lines;
    std::stringstream stream(value);
    std::string line;
    while (std::getline(stream, line)) lines.push_back(line);
    return lines;
}

static std::string joinLines(const std::vector<std::string>& lines) {
    std::string out;
    for (const std::string& line : lines) {
        out += line;
        out += "\n";
    }
    return out;
}

static std::vector<std::string> removeSectionFromLines(const std::vector<std::string>& lines, const std::string& section) {
    std::vector<std::string> out;
    bool skip = false;
    for (const std::string& line : lines) {
        std::string stripped = trim(line);
        if (stripped.size() >= 2 && stripped.front() == '[' && stripped.back() == ']') {
            std::string current = stripped.substr(1, stripped.size() - 2);
            skip = current == section;
        }
        if (!skip) out.push_back(line);
    }
    return out;
}

static std::vector<std::string> normalizeIniLines(std::vector<std::string> lines) {
    while (!lines.empty() && trim(lines.front()).empty()) lines.erase(lines.begin());
    while (!lines.empty() && trim(lines.back()).empty()) lines.pop_back();

    std::vector<std::string> out;
    bool previousBlank = false;
    for (const std::string& line : lines) {
        bool blank = trim(line).empty();
        if (blank && previousBlank) continue;
        out.push_back(line);
        previousBlank = blank;
    }
    return out;
}

static void appendSection(std::vector<std::string>& lines, const std::vector<std::string>& sectionLines) {
    lines = normalizeIniLines(lines);
    if (!lines.empty()) lines.push_back("");
    for (const std::string& line : sectionLines) lines.push_back(line);
}

static std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

static bool sectionEnabledInText(const std::string& text, const std::string& section) {
    for (const std::string& line : splitLines(text)) {
        std::string stripped = trim(line);
        if (stripped.empty() || stripped.front() == '#' || stripped.front() == ';') continue;
        if (stripped.size() >= 2 && stripped.front() == '[' && stripped.back() == ']') {
            if (stripped.substr(1, stripped.size() - 2) == section) return true;
        }
    }
    return false;
}

static bool iniBoolValue(const std::string& text, const std::string& key, bool defaultValue = false) {
    const std::string wanted = toLower(key);
    for (const std::string& line : splitLines(text)) {
        std::string stripped = trim(line);
        if (stripped.empty() || stripped.front() == '#' || stripped.front() == ';') continue;
        size_t eq = stripped.find('=');
        if (eq == std::string::npos) continue;
        if (toLower(trim(stripped.substr(0, eq))) != wanted) continue;
        std::string value = toLower(trim(stripped.substr(eq + 1)));
        return value == "true" || value == "1" || value == "yes" || value == "on";
    }
    return defaultValue;
}

static std::string extractSectionValue(const std::string& text, const std::string& section, const std::string& key) {
    bool inSection = false;
    for (const std::string& line : splitLines(text)) {
        std::string stripped = trim(line);
        if (stripped.size() >= 2 && stripped.front() == '[' && stripped.back() == ']') {
            inSection = stripped.substr(1, stripped.size() - 2) == section;
            continue;
        }
        if (!inSection || stripped.empty() || stripped.front() == '#' || stripped.front() == ';') continue;
        size_t eq = stripped.find('=');
        if (eq == std::string::npos) continue;
        if (trim(stripped.substr(0, eq)) == key) return trim(stripped.substr(eq + 1));
    }
    return "";
}

static bool jsonBoolValue(const std::string& json, const std::string& key) {
    std::string marker = "\"" + key + "\"";
    size_t pos = json.find(marker);
    if (pos == std::string::npos) return false;
    size_t colon = json.find(':', pos + marker.size());
    if (colon == std::string::npos) return false;
    size_t value = json.find_first_not_of(" \t\r\n", colon + 1);
    if (value == std::string::npos) return false;
    return json.compare(value, 4, "true") == 0;
}

static std::string setJsonBool(std::string json, const std::string& key, bool enabled) {
    if (trim(json).empty()) json = "{\n}\n";
    std::string marker = "\"" + key + "\"";
    std::string value = enabled ? "true" : "false";
    size_t pos = json.find(marker);
    if (pos != std::string::npos) {
        size_t colon = json.find(':', pos + marker.size());
        if (colon != std::string::npos) {
            size_t start = json.find_first_not_of(" \t\r\n", colon + 1);
            if (start != std::string::npos) {
                if (json.compare(start, 4, "true") == 0) {
                    json.replace(start, 4, value);
                    return json;
                }
                if (json.compare(start, 5, "false") == 0) {
                    json.replace(start, 5, value);
                    return json;
                }
            }
        }
    }

    size_t close = json.find_last_of('}');
    if (close == std::string::npos) return "{\n    \"" + key + "\": " + value + "\n}\n";

    std::string before = json.substr(0, close);
    std::string after = json.substr(close);
    while (!before.empty() && (before.back() == ' ' || before.back() == '\t' || before.back() == '\r' || before.back() == '\n')) before.pop_back();

    bool hasExisting = before.find(':') != std::string::npos;
    if (hasExisting && before.back() != '{') before += ",";
    before += "\n    \"" + key + "\": " + value + "\n";
    return before + after;
}


static std::string simpleConfigValue(const std::string& text, const std::string& key) {
    const std::string wanted = toLower(trim(key));
    for (const std::string& rawLine : splitLines(text)) {
        std::string line = trim(rawLine);
        if (line.empty() || line.front() == '#' || line.front() == ';') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        if (toLower(trim(line.substr(0, eq))) != wanted) continue;
        std::string value = trim(line.substr(eq + 1));
        if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') || (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.size() - 2);
        }
        return value;
    }
    return "";
}

static std::string boolText(bool value) {
    return value ? "YES" : "NO";
}

static std::string dirnameOf(const std::string& path) {
    size_t pos = path.find_last_of('/');
    if (pos == std::string::npos || pos == 0) return "/";
    return path.substr(0, pos);
}

static std::string writeTextCommand(const std::string& path, const std::string& text) {
    return "mkdir -p " + shellQuote(dirnameOf(path)) + " && cat > " + shellQuote(path) + " <<'MC_EOF'\n" + text + "MC_EOF\n";
}

static std::string fileStatusCommand(const std::string& path) {
    return "test -f " + path + " && echo YES || echo NO";
}

static std::string execStatusCommand(const std::string& path) {
    return "test -x " + path + " && echo YES || echo NO";
}

static std::string statusValue(const std::vector<std::string>& lines, const std::string& key) {
    const std::string prefix = key + ":";
    for (const std::string& line : lines) {
        if (line.rfind(prefix, 0) == 0) {
            return trim(line.substr(prefix.size()));
        }
    }
    return "";
}

static bool statusYes(const std::vector<std::string>& lines, const std::string& key) {
    return statusValue(lines, key) == "YES";
}

static std::string lowerCopyExtra(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

static std::string basenameCopy(const std::string& value) {
    size_t pos = value.find_last_of('/');
    if (pos == std::string::npos) return value;
    return value.substr(pos + 1);
}

static std::vector<std::string> splitByChar(const std::string& value, char delimiter) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : value) {
        if (c == delimiter) {
            parts.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    parts.push_back(current);
    return parts;
}

static std::string normalizeExtraVersion(std::string value) {
    value = trim(value);
    value.erase(std::remove(value.begin(), value.end(), '\r'), value.end());
    value.erase(std::remove(value.begin(), value.end(), '\n'), value.end());
    value.erase(std::remove(value.begin(), value.end(), '\t'), value.end());
    while (!value.empty() && (value.front() == '"' || value.front() == '\'')) value.erase(value.begin());
    while (!value.empty() && (value.back() == '"' || value.back() == '\'')) value.pop_back();

    const std::string tagNeedle = "/releases/tag/";
    size_t tagPos = value.find(tagNeedle);
    if (tagPos != std::string::npos) value = value.substr(tagPos + tagNeedle.size());

    const std::string refNeedle = "refs/tags/";
    size_t refPos = value.find(refNeedle);
    if (refPos != std::string::npos) value = value.substr(refPos + refNeedle.size());

    size_t queryPos = value.find('?');
    if (queryPos != std::string::npos) value = value.substr(0, queryPos);

    size_t hashPos = value.find('#');
    if (hashPos != std::string::npos) value = value.substr(0, hashPos);

    value = trim(value);

    size_t bracketPos = value.find('[');
    if (bracketPos != std::string::npos) value = value.substr(0, bracketPos);

    size_t spacePos = value.find_first_of(" ");
    if (spacePos != std::string::npos) value = value.substr(0, spacePos);

    while (!value.empty() && value.back() == '/') value.pop_back();
    value = trim(value);

    auto stripPrefix = [&](const std::string& prefix) {
        if (value.size() >= prefix.size() && lowerCopyExtra(value.substr(0, prefix.size())) == lowerCopyExtra(prefix)) {
            value = value.substr(prefix.size());
        }
    };
    stripPrefix("release-");
    stripPrefix("version-");
    if (value.size() > 1 && (value[0] == 'v' || value[0] == 'V') && std::isdigit(static_cast<unsigned char>(value[1]))) {
        value.erase(value.begin());
    }
    return trim(value);
}

static std::string jsonStringValueAfter(const std::string& json, size_t start, const std::string& field) {
    std::string needle = "\"" + field + "\"";
    size_t fieldPos = json.find(needle, start);
    if (fieldPos == std::string::npos) return "";

    size_t objectEnd = json.find('}', start);
    if (objectEnd != std::string::npos && fieldPos > objectEnd) return "";

    size_t colon = json.find(':', fieldPos + needle.size());
    if (colon == std::string::npos) return "";
    size_t firstQuote = json.find('"', colon + 1);
    if (firstQuote == std::string::npos) return "";
    size_t secondQuote = json.find('"', firstQuote + 1);
    if (secondQuote == std::string::npos) return "";
    return json.substr(firstQuote + 1, secondQuote - firstQuote - 1);
}

static std::string versionForJsonToken(const std::string& json, const std::string& token) {
    if (json.empty() || token.empty()) return "";
    std::string needle = "\"" + token + "\"";
    size_t keyPos = json.find(needle);
    if (keyPos == std::string::npos) return "";

    size_t colon = json.find(':', keyPos + needle.size());
    if (colon == std::string::npos) return "";
    size_t valueStart = json.find_first_not_of(" \r\n\t", colon + 1);
    if (valueStart == std::string::npos) return "";

    if (json[valueStart] == '"') {
        size_t endQuote = json.find('"', valueStart + 1);
        if (endQuote == std::string::npos) return "";
        return json.substr(valueStart + 1, endQuote - valueStart - 1);
    }

    std::string version = jsonStringValueAfter(json, valueStart, "version");
    if (!version.empty()) return version;
    return jsonStringValueAfter(json, valueStart, "installed_version");
}

static std::string installedRaVersionForSource(const std::string& json, const std::string& key, const std::string& title, const std::string& repo) {
    const std::string repoName = basenameCopy(repo);
    std::vector<std::string> tokens = {
        key,
        lowerCopyExtra(key),
        title,
        lowerCopyExtra(title),
        repo,
        lowerCopyExtra(repo),
        repoName,
        lowerCopyExtra(repoName)
    };

    for (const std::string& token : tokens) {
        std::string value = versionForJsonToken(json, token);
        if (!value.empty()) return value;
    }
    return "";
}

static std::string remoteManageCommand(const std::string& action) {
    return std::string(RemoteScriptPath) + " " + action + " --unattended";
}

void App::run() {
    config = ConfigStore::load();
    cachedScriptStatus.assign(ScriptCount, std::vector<std::string>{"STATUS: NOT CHECKED"});
    cachedExtraStatus.assign(ExtraCount, std::vector<std::string>{"STATUS: NOT CHECKED"});
    if (!ui.initialize()) return;

    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);
        const u64 up = padGetButtonsUp(&pad);
        const u64 held = padGetButtons(&pad);

        if (passthroughActive) {
            handlePassthroughInput(down, up, held);
            draw();
            continue;
        }

        handleInput(down);
        draw();
        performPendingExtraUpdateCheck();
    }

    if (passthroughActive) stopPassthrough();
    remote.disconnect();
    ui.shutdown();
}

void App::drawHeader() {
    ui.clear(UiRenderer::rgb(12, 10, 20));
    ui.fillRect(0, 0, UiRenderer::Width, 92, UiRenderer::rgb(20, 16, 34));
    ui.fillRect(0, 90, UiRenderer::Width, 4, UiRenderer::rgb(143, 84, 255));

    ui.drawText(42, 30, "MISTER COMPANION VITA", UiRenderer::rgb(248, 245, 255), 3);
    ui.drawStatusPill(UiRenderer::Width - 280, 28, ssh.isConnected() ? "CONNECTED" : "DISCONNECTED", ssh.isConnected());

    ui.drawTab(24, 110, 150, "CONNECTION", tab == Tab::Connection);
    ui.drawTab(184, 110, 105, "DEVICE", tab == Tab::Device);
    ui.drawTab(299, 110, 105, "REMOTE", tab == Tab::Remote);
    ui.drawTab(414, 110, 120, "SCRIPTS", tab == Tab::Scripts);
    ui.drawTab(544, 110, 130, "SETTINGS", tab == Tab::Settings);
    ui.drawTab(684, 110, 155, "WALLPAPERS", tab == Tab::Wallpapers);
    ui.drawTab(849, 110, 105, "EXTRAS", tab == Tab::Extras);
}

void App::draw() {
    ui.beginFrame();

    if (passthroughActive) {
        drawPassthrough();
        ui.endFrame();
        return;
    }

    drawHeader();

    if (tab == Tab::Connection) drawConnection();
    else if (tab == Tab::Device) drawDevice();
    else if (tab == Tab::Remote) drawRemote();
    else if (tab == Tab::Scripts) drawScripts();
    else if (tab == Tab::Settings) drawSettings();
    else if (tab == Tab::Wallpapers) drawWallpapers();
    else drawExtras();

    ui.drawMessage(lastMessage);
    if (tab == Tab::Connection) {
        if (ssh.isConnected()) {
            ui.drawFooter("× DISCONNECT    L/R TABS");
        } else {
            ui.drawFooter(connectionProfileMode ? "UP/DN PROFILE    × CONNECT    □ EDIT    SEL DELETE    LEFT/RIGHT MODE    L/R TABS" : "UP/DN SELECT    × EDIT/CONNECT    START SAVE    SEL SCAN    LEFT/RIGHT MODE    L/R TABS");
        }
    } else if (tab == Tab::Scripts) {
        ui.drawFooter("UP/DN SELECT    × CONFIRM/EDIT    LEFT/RIGHT SCRIPT    L/R TABS");
    } else if (tab == Tab::Settings) {
        ui.drawFooter("UP/DN SELECT    × CYCLE    □ SAVE    ○ CANCEL    LEFT/RIGHT INI    L/R TABS");
    } else if (tab == Tab::Wallpapers) {
        ui.drawFooter("UP/DN SELECT    × CONFIRM    LEFT/RIGHT SOURCE    SEL STATIC    L/R TABS");
    } else if (tab == Tab::Extras) {
        ui.drawFooter("UP/DN SELECT    × CONFIRM    LEFT/RIGHT EXTRA    L/R TABS");
    } else {
        ui.drawFooter("UP/DN SELECT    × CONFIRM/EDIT    L/R TABS");
    }
    ui.endFrame();
}

void App::drawConnection() {
    const bool connected = ssh.isConnected();
    const bool showProfileMode = connectionProfileMode && !connected;
    const int manualSelection = connected ? 3 : selected;

    ui.drawCard(40, 176, 580, 400, "CONNECTION STATUS");
    ui.drawCard(660, 176, 580, 400, showProfileMode ? "PROFILES" : "MANUAL CONNECTION");

    ui.drawText(76, 246, "MODE", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 246, showProfileMode ? "PROFILES" : "MANUAL CONNECT", UiRenderer::rgb(248, 245, 255), 2);

    ui.drawText(76, 294, "STATUS", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 294, status, ssh.isConnected() ? UiRenderer::rgb(112, 232, 165) : UiRenderer::rgb(248, 245, 255), 2);

    ui.drawText(76, 342, "MISTER", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 342, safeText(config.host), UiRenderer::rgb(248, 245, 255), 2);

    ui.drawText(76, 390, "USER", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 390, safeText(config.username), UiRenderer::rgb(248, 245, 255), 2);

    ui.drawText(76, 438, "PROFILE", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 438, activeProfileName.empty() ? "Not selected" : safeText(activeProfileName), UiRenderer::rgb(248, 245, 255), 2);

    ui.drawText(76, 506, "TIP", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(76, 538, connected ? "DISCONNECT BEFORE EDITING CONNECTION DETAILS" : "LEFT/RIGHT SWITCH MANUAL AND PROFILES", UiRenderer::rgb(218, 208, 238), 2);

    if (!showProfileMode) {
        ui.drawButton(696, 220, 508, 58, "HOST  " + safeText(config.host), manualSelection == 0, false, connected);
        ui.drawButton(696, 292, 508, 58, "USER  " + safeText(config.username), manualSelection == 1, false, connected);
        ui.drawButton(696, 364, 508, 58, config.password.empty() ? "PASSWORD  NOT SET" : "PASSWORD  ********", manualSelection == 2, false, connected);
        ui.drawButton(696, 436, 508, 58, connected ? "DISCONNECT" : "CONNECT", manualSelection == 3);
        ui.drawText(696, 526, connected ? "× DISCONNECT" : "START SAVE    SEL SCAN FOR MISTER", UiRenderer::rgb(218, 208, 238), 2);
        return;
    }

    const int count = static_cast<int>(config.profiles.size());
    if (count <= 0) {
        ui.drawText(696, 280, "NO PROFILES SAVED", UiRenderer::rgb(248, 245, 255), 3);
        ui.drawText(696, 340, "SWITCH TO MANUAL MODE AND PRESS START", UiRenderer::rgb(218, 208, 238), 2);
        ui.drawText(696, 526, "× CONNECT    □ EDIT    SEL DELETE", UiRenderer::rgb(218, 208, 238), 2);
        return;
    }

    const int visibleRows = 5;
    if (selectedProfile < 0) selectedProfile = 0;
    if (selectedProfile >= count) selectedProfile = count - 1;
    if (profileScroll > selectedProfile) profileScroll = selectedProfile;
    if (profileScroll < selectedProfile - visibleRows + 1) profileScroll = selectedProfile - visibleRows + 1;
    if (profileScroll < 0) profileScroll = 0;

    for (int row = 0; row < visibleRows; ++row) {
        int index = profileScroll + row;
        if (index >= count) break;
        const ConnectionProfile& profile = config.profiles[index];
        std::string label = profile.name + "  " + profile.host;
        ui.drawButton(696, 220 + row * 64, 508, 56, ellipsizeText(label, 38), selectedProfile == index);
    }

    if (count > visibleRows) {
        ui.drawText(696, 548, std::to_string(selectedProfile + 1) + " / " + std::to_string(count), UiRenderer::rgb(174, 154, 218), 2);
    }
    ui.drawText(856, 548, "× CONNECT    □ EDIT    SEL DELETE", UiRenderer::rgb(218, 208, 238), 2);
}

void App::drawDevice() {
    ui.drawCard(40, 176, 580, 400, "DEVICE STATUS");
    ui.drawCard(660, 176, 580, 400, "DEVICE ACTIONS");

    ui.drawText(76, 246, "MISTER", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 246, safeText(config.host), UiRenderer::rgb(248, 245, 255), 2);

    ui.drawText(76, 294, "STATUS", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 294, ssh.isConnected() ? "CONNECTED" : "DISCONNECTED", ssh.isConnected() ? UiRenderer::rgb(112, 232, 165) : UiRenderer::rgb(232, 130, 160), 2);

    ui.drawText(76, 342, "SD", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 342, sdStorage, UiRenderer::rgb(248, 245, 255), 2);

    ui.drawText(76, 390, "USB", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 390, usbStorage, UiRenderer::rgb(248, 245, 255), 2);

    ui.drawText(76, 438, "SMB", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 438, smbStatus, UiRenderer::rgb(248, 245, 255), 2);

    if (!nowPlaying.empty()) {
        ui.drawText(76, 500, "NOW PLAYING", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawText(76, 532, nowPlaying, UiRenderer::rgb(248, 245, 255), 2);
    }

    const bool actionsEnabled = ssh.isConnected();
    ui.drawButton(696, 252, 508, 60, "REFRESH DEVICE INFO", selected == 0 && actionsEnabled, false, !actionsEnabled);
    ui.drawButton(696, 334, 508, 60, "TOGGLE SMB STARTUP", selected == 1 && actionsEnabled, false, !actionsEnabled);
    ui.drawButton(696, 416, 508, 60, "RETURN TO MISTER MENU", selected == 2 && actionsEnabled, false, !actionsEnabled);
    ui.drawButton(696, 498, 508, 60, "REBOOT MISTER", selected == 3 && actionsEnabled, true, !actionsEnabled);
}

void App::drawRemote() {
    ui.drawCard(40, 176, 580, 400, "REMOTE STATUS");
    ui.drawCard(660, 176, 580, 400, "REMOTE ACTIONS");

    ui.drawText(76, 246, "MISTER", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(260, 246, safeText(config.host), UiRenderer::rgb(248, 245, 255), 2);

    ui.drawText(76, 294, "DAEMON", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(260, 294, remoteInstalled, UiRenderer::rgb(248, 245, 255), 2);

    ui.drawText(76, 342, "STATUS", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(260, 342, remoteRunning == "Yes" ? "RUNNING" : remoteRunning == "No" ? "STOPPED" : remoteRunning, remoteRunning == "Yes" ? UiRenderer::rgb(112, 232, 165) : UiRenderer::rgb(232, 130, 160), 2);

    ui.drawText(76, 390, "START ON BOOT", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(260, 390, remoteStartup, UiRenderer::rgb(248, 245, 255), 2);

    ui.drawText(76, 438, "PASSTHROUGH", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(260, 438, passthroughActive ? "ENABLED" : "DISABLED", passthroughActive ? UiRenderer::rgb(112, 232, 165) : UiRenderer::rgb(218, 208, 238), 2);

    ui.drawText(76, 506, "PASSTHROUGH EXIT", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(76, 538, "PRESS SELECT + L TO EXIT", UiRenderer::rgb(248, 245, 255), 2);

    const bool actionsEnabled = ssh.isConnected();
    const bool installed = remoteInstalled == "Yes";
    const bool running = remoteRunning == "Yes";
    const bool startup = remoteStartup == "Enabled";

    if (!installed) {
        ui.drawButton(696, 280, 508, 60, "INSTALL / UPDATE DAEMON", selected == 0 && actionsEnabled, false, !actionsEnabled);
        ui.drawButton(696, 362, 508, 60, "REFRESH REMOTE STATUS", selected == 1 && actionsEnabled, false, !actionsEnabled);
        return;
    }

    if (running) {
        ui.drawButton(696, 238, 508, 60, "START PASSTHROUGH MODE", selected == 0 && actionsEnabled, false, !actionsEnabled);
        ui.drawButton(696, 320, 508, 60, "STOP REMOTE DAEMON", selected == 1 && actionsEnabled, false, !actionsEnabled);
        ui.drawButton(696, 402, 508, 60, startup ? "DISABLE START ON BOOT" : "ENABLE START ON BOOT", selected == 2 && actionsEnabled, false, !actionsEnabled);
        ui.drawButton(696, 484, 508, 60, "REFRESH REMOTE STATUS", selected == 3 && actionsEnabled, false, !actionsEnabled);
        return;
    }

    ui.drawButton(696, 238, 508, 60, "START REMOTE DAEMON", selected == 0 && actionsEnabled, false, !actionsEnabled);
    ui.drawButton(696, 320, 508, 60, startup ? "DISABLE START ON BOOT" : "ENABLE START ON BOOT", selected == 1 && actionsEnabled, false, !actionsEnabled);
    ui.drawButton(696, 402, 508, 60, "UNINSTALL REMOTE DAEMON", selected == 2 && actionsEnabled, true, !actionsEnabled);
    ui.drawButton(696, 484, 508, 60, "REFRESH REMOTE STATUS", selected == 3 && actionsEnabled, false, !actionsEnabled);
}


void App::drawScripts() {
    ui.drawCard(40, 176, 580, 400, "SCRIPT STATUS");
    ui.drawCard(660, 176, 580, 400, "SCRIPT ACTIONS");

    ScriptId id = static_cast<ScriptId>(selectedScript);
    std::string title = scriptTitle(id);
    std::vector<std::string> statusLines = selectedScript >= 0 && selectedScript < static_cast<int>(cachedScriptStatus.size())
        ? cachedScriptStatus[selectedScript]
        : std::vector<std::string>{"STATUS: NOT CHECKED"};
    std::vector<std::string> actions = scriptActions(id);

    ui.drawText(76, 236, "SCRIPT", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 236, title, UiRenderer::rgb(248, 245, 255), 3);
    ui.drawText(76, 290, "SUB TAB", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 290, std::to_string(selectedScript + 1) + " / " + std::to_string(ScriptCount), UiRenderer::rgb(248, 245, 255), 2);

    int y = 346;
    for (const std::string& line : statusLines) {
        if (y > 530) break;
        ui.drawText(76, y, line, UiRenderer::rgb(218, 208, 238), 2);
        y += 42;
    }

    const bool actionsEnabled = ssh.isConnected();
    int actionY = 232;
    for (int i = 0; i < static_cast<int>(actions.size()); i++) {
        if (actionY > 520) break;
        const bool danger = actions[i].find("UNINSTALL") != std::string::npos || actions[i].find("REMOVE") != std::string::npos;
        ui.drawButton(696, actionY, 508, 54, actions[i], selected == i && actionsEnabled, danger, !actionsEnabled);
        actionY += 68;
    }
}


void App::drawExtras() {
    ui.drawCard(40, 176, 580, 400, "EXTRA STATUS");
    ui.drawCard(660, 176, 580, 400, "EXTRA ACTIONS");

    ExtraId id = static_cast<ExtraId>(selectedExtra);
    std::string title = extraTitle(id);
    std::vector<std::string> statusLines = selectedExtra >= 0 && selectedExtra < static_cast<int>(cachedExtraStatus.size())
        ? cachedExtraStatus[selectedExtra]
        : std::vector<std::string>{"STATUS: NOT CHECKED"};
    std::vector<std::string> actions = extraActions(id);

    ui.drawText(76, 236, "EXTRA", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 236, title, UiRenderer::rgb(248, 245, 255), 3);
    ui.drawText(76, 290, "SUB TAB", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 290, std::to_string(selectedExtra + 1) + " / " + std::to_string(ExtraCount), UiRenderer::rgb(248, 245, 255), 2);

    int y = 346;
    for (const std::string& line : statusLines) {
        if (id == ExtraId::RetroAchievementCores) {
            if (line.rfind("VERSION:", 0) == 0) continue;
            if (line.rfind("LATEST:", 0) == 0) continue;
            if (line.rfind("OUTDATED:", 0) == 0) continue;
            if (line.rfind("OUTDATED_KEYS:", 0) == 0) continue;
        }
        if (y > 530) break;
        ui.drawText(76, y, line, UiRenderer::rgb(218, 208, 238), 2);
        y += 42;
    }

    const bool actionsEnabled = ssh.isConnected();
    int actionY = 232;
    for (int i = 0; i < static_cast<int>(actions.size()); i++) {
        if (actionY > 520) break;
        const bool danger = actions[i].find("UNINSTALL") != std::string::npos || actions[i].find("REMOVE") != std::string::npos;
        ui.drawButton(696, actionY, 508, 54, actions[i], selected == i && actionsEnabled, danger, !actionsEnabled);
        actionY += 68;
    }
}


void App::drawSettings() {
    ui.drawCard(40, 176, 1200, 424, "MISTER SETTINGS");

    const std::string statusText = settingsDirty ? "UNSAVED CHANGES" : "READY";
    ui.drawTextCentered(460, 196, 360, statusText, settingsDirty ? UiRenderer::rgb(232, 190, 120) : UiRenderer::rgb(112, 232, 165), 2);
    ui.drawText(892, 196, "INI: " + safeText(selectedSettingsIni, "MiSTer.ini"), UiRenderer::rgb(174, 154, 218), 2);

    const bool actionsEnabled = ssh.isConnected();
    if (!actionsEnabled) {
        ui.drawText(76, 252, "CONNECT TO A MISTER FIRST", UiRenderer::rgb(232, 130, 160), 3);
        ui.drawText(76, 310, "SETTINGS EDITING IS DISABLED WHILE DISCONNECTED.", UiRenderer::rgb(218, 208, 238), 2);
        return;
    }

    if (!settingsLoaded) {
        ui.drawText(76, 252, "SETTINGS NOT LOADED", UiRenderer::rgb(232, 190, 120), 3);
        ui.drawText(76, 310, "PRESS × TO LOAD MISTER.INI SETTINGS.", UiRenderer::rgb(218, 208, 238), 2);
        ui.drawButton(76, 390, 460, 58, "LOAD SETTINGS", selectedSettings == 0, false, false);
        return;
    }

    const int count = settingsOptionCount();
    const int visible = 5;
    if (selectedSettings < settingsScroll) settingsScroll = selectedSettings;
    if (selectedSettings >= settingsScroll + visible) settingsScroll = selectedSettings - visible + 1;
    if (settingsScroll < 0) settingsScroll = 0;
    if (settingsScroll > std::max(0, count - visible)) settingsScroll = std::max(0, count - visible);

    const int rowX = 76;
    const int rowW = 1128;
    const int labelX = rowX + 28;
    const int valueX = rowX + 520;
    int y = 276;

    for (int i = settingsScroll; i < count && i < settingsScroll + visible; i++) {
        const bool isRestore = settingsLabel(i) == "Restore Defaults";
        const bool isSelected = selectedSettings == i;
        const std::string label = settingsLabel(i);
        const std::string value = settingsValue(i);

        const u32 base = isRestore ? UiRenderer::rgb(58, 30, 44) : UiRenderer::rgb(36, 31, 54);
        const u32 active = isRestore ? UiRenderer::rgb(155, 54, 80) : UiRenderer::rgb(124, 70, 220);
        const u32 border = isSelected ? active : UiRenderer::rgb(67, 57, 92);
        const u32 valueColor = value.empty() ? UiRenderer::rgb(174, 154, 218) : UiRenderer::rgb(248, 245, 255);

        ui.fillRect(rowX, y, rowW, 50, isSelected ? active : base);
        ui.drawRect(rowX, y, rowW, 50, border, isSelected ? 4 : 2);
        ui.drawText(labelX, y + 18, label, UiRenderer::rgb(218, 208, 238), 2);

        if (!value.empty()) {
            ui.fillRect(valueX - 18, y + 8, rowW - (valueX - rowX) - 18, 34, isSelected ? UiRenderer::rgb(104, 58, 190) : UiRenderer::rgb(28, 24, 42));
            ui.drawText(valueX, y + 18, value, valueColor, 2);
        }

        y += 58;
    }

    std::string counter = std::to_string(selectedSettings + 1) + " / " + std::to_string(count);
    ui.drawTextCentered(76, 576, 1128, counter, UiRenderer::rgb(174, 154, 218), 2);
}


std::string App::wallpaperSourceTitle(int index) const {
    switch (index) {
        case 0: return "RANNY SNICE";
        case 1: return "PCN CHALLENGE";
        case 2: return "PCN PREMIUM";
        case 3: return "ANIME0T4KU";
    }
    return "WALLPAPERS";
}


static std::vector<int> gWallpaperPackTotal;
static std::vector<int> gWallpaperPackInstalled;
static std::vector<int> gWallpaperPackMissing;
static int gWallpaperInstalledTotal = 0;
static int gWallpaperMissingTotal = 0;
static bool gWallpaperStatusChecking = false;
static int gWallpaperStatusSource = -1;
static int gWallpaperStatusGeneration = 0;
static std::mutex gWallpaperStatusMutex;
static int gWallpaperWorkerGeneration = 0;
static int gWallpaperWorkerSource = -1;
static std::vector<int> gWallpaperWorkerTotal;
static std::vector<int> gWallpaperWorkerInstalled;
static std::vector<int> gWallpaperWorkerMissing;
static std::vector<std::string> gWallpaperWorkerStatus;
static std::set<std::string> gWallpaperInstalledNames;
static std::string gWallpaperStaticActive = "NO";
static std::string gWallpaperStaticName = "None";
static std::string gWallpaperStatusToken;
static int gWallpaperPollDelay = 0;

static std::string lowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

static std::string baseNameOnly(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return path;
    return path.substr(pos + 1);
}

static std::set<std::string> installedWallpaperNameSet(const std::string& output) {
    std::set<std::string> installed;
    for (const std::string& rawLine : splitLines(output)) {
        std::string line = trim(rawLine);
        if (line.empty()) continue;
        installed.insert(lowerCopy(baseNameOnly(line)));
    }
    return installed;
}

static int countInstalledWallpaperEntries(const std::vector<WallpaperDb::Entry>& entries, const std::set<std::string>& installed) {
    int count = 0;
    for (const WallpaperDb::Entry& entry : entries) {
        if (installed.count(lowerCopy(entry.name))) count++;
    }
    return count;
}

static std::string wallpaperEntriesHeredoc(const std::vector<WallpaperDb::Entry>& entries, bool namesOnly) {
    std::string text;
    for (const WallpaperDb::Entry& entry : entries) {
        if (entry.name.empty()) continue;
        text += entry.name;
        if (!namesOnly) {
            text += "|";
            text += entry.url;
        }
        text += "\n";
    }
    return text;
}

static std::string wallpaperInstallCommandFromEntries(const std::vector<WallpaperDb::Entry>& entries) {
    std::string command;
    command += "mkdir -p /media/fat/wallpapers\n";
    command += "LIST=/tmp/mc_wallpaper_install_$$.txt\n";
    command += "cat > \"$LIST\" <<'MCWALLPAPERS'\n";
    command += wallpaperEntriesHeredoc(entries, false);
    command += "MCWALLPAPERS\n";
    command += R"SH(
INSTALLED_LIST=/tmp/mc_wallpaper_installed_$$.txt
find /media/fat/wallpapers -maxdepth 1 -type f 2>/dev/null | while read -r installed_path; do
    basename "$installed_path" | tr 'A-Z' 'a-z'
done | sort -u > "$INSTALLED_LIST"
is_installed_name() {
    check_name=$(printf '%s' "$1" | tr 'A-Z' 'a-z')
    grep -Fx "$check_name" "$INSTALLED_LIST" >/dev/null 2>&1
}
echo Installing wallpapers...
COUNT=0
SKIP=0
FAIL=0
while IFS='|' read -r name url; do
    [ -z "$name" ] && continue
    if is_installed_name "$name"; then
        echo "Skipped $name"
        SKIP=$((SKIP + 1))
        continue
    fi
    echo "Downloading $name..."
    if wget --no-check-certificate -q -O "/media/fat/wallpapers/$name" "$url" && [ -s "/media/fat/wallpapers/$name" ]; then
        printf '%s\n' "$name" | tr 'A-Z' 'a-z' >> "$INSTALLED_LIST"
        sort -u "$INSTALLED_LIST" -o "$INSTALLED_LIST"
        echo "Installed $name"
        COUNT=$((COUNT + 1))
    else
        echo "Failed $name"
        rm -f "/media/fat/wallpapers/$name"
        FAIL=$((FAIL + 1))
    fi
done < "$LIST"
rm -f "$LIST" "$INSTALLED_LIST"
sync
echo "Installed $COUNT wallpaper(s), skipped $SKIP, failed $FAIL."
[ "$FAIL" -eq 0 ]
)SH";
    return command;
}

static std::string wallpaperRemoveCommandFromEntries(const std::vector<WallpaperDb::Entry>& entries) {
    std::string command;
    command += "LIST=/tmp/mc_wallpaper_remove_$$.txt\n";
    command += "cat > \"$LIST\" <<'MCWALLPAPERS'\n";
    command += wallpaperEntriesHeredoc(entries, true);
    command += "MCWALLPAPERS\n";
    command += R"SH(
echo Removing installed wallpapers from selected source...
COUNT=0
installed_path_for_name() {
    check_name=$(printf '%s' "$1" | tr 'A-Z' 'a-z')
    find /media/fat/wallpapers -maxdepth 1 -type f 2>/dev/null | while read -r installed_path; do
        installed_name=$(basename "$installed_path" | tr 'A-Z' 'a-z')
        if [ "$installed_name" = "$check_name" ]; then
            printf '%s\n' "$installed_path"
            exit 0
        fi
    done | head -n 1
}
while read -r name; do
    [ -z "$name" ] && continue
    target=$(installed_path_for_name "$name")
    if [ -n "$target" ] && [ -f "$target" ]; then
        rm -f "$target" && echo "Removed $(basename "$target")" && COUNT=$((COUNT + 1))
    fi
done < "$LIST"
rm -f "$LIST"
sync
echo "Removed $COUNT wallpaper(s)."
)SH";
    return command;
}

static const char* wallpaperDbUrlForSource(int source) {
    switch (source) {
        case 0: return RannyDbUrl;
        case 1: return PcnDbUrl;
        case 2: return PcnPremiumDbUrl;
        case 3: return Ot4kuDbUrl;
    }
    return "";
}

static const char* wallpaperRawBaseForSource(int source) {
    switch (source) {
        case 0: return RannyRawBase;
        case 1: return PcnRawBase;
        case 2: return PcnPremiumRawBase;
        case 3: return Ot4kuRawBase;
    }
    return "";
}

static std::vector<unsigned char> decodeBase64Text(const std::string& text) {
    static const signed char table[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-2,-1,-1,
        -1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };

    std::vector<unsigned char> out;
    int val = 0;
    int bits = -8;
    for (unsigned char c : text) {
        if (c == '=' || c == '\r' || c == '\n' || c == ' ' || c == '\t') {
            if (c == '=') break;
            continue;
        }
        int decoded = table[c];
        if (decoded < 0) continue;
        val = (val << 6) + decoded;
        bits += 6;
        if (bits >= 0) {
            out.push_back(static_cast<unsigned char>((val >> bits) & 0xFF));
            bits -= 8;
        }
    }
    return out;
}

static bool wallpaperEntriesFromZipBytes(int source, const std::string& filterMode, const std::vector<unsigned char>& data, std::vector<WallpaperDb::Entry>& entries, std::string& error) {
    return WallpaperDb::parseEntriesFromData(data, wallpaperRawBaseForSource(source), filterMode, entries, error);
}

std::vector<std::string> App::wallpaperActions(int index) const {
    std::vector<std::string> actions;
    if (!ssh.isConnected()) {
        if (index == 0) {
            actions.push_back("INSTALL 16:9 WALLPAPERS");
            actions.push_back("INSTALL 4:3 WALLPAPERS");
        } else {
            actions.push_back("INSTALL WALLPAPERS");
        }
        return actions;
    }

    if (gWallpaperStatusChecking && gWallpaperStatusSource == index) {
        return actions;
    }

    auto addPackAction = [&](int packIndex, const std::string& installLabel, const std::string& updateLabel) {
        const int total = packIndex < static_cast<int>(gWallpaperPackTotal.size()) ? gWallpaperPackTotal[packIndex] : 0;
        const int installed = packIndex < static_cast<int>(gWallpaperPackInstalled.size()) ? gWallpaperPackInstalled[packIndex] : 0;
        const int missing = packIndex < static_cast<int>(gWallpaperPackMissing.size()) ? gWallpaperPackMissing[packIndex] : 0;

        if (total <= 0) {
            actions.push_back(installLabel);
        } else if (installed <= 0) {
            actions.push_back(installLabel);
        } else if (missing > 0) {
            actions.push_back(updateLabel);
        }
    };

    if (index == 0) {
        addPackAction(0, "INSTALL 16:9 WALLPAPERS", "UPDATE 16:9 WALLPAPERS");
        addPackAction(1, "INSTALL 4:3 WALLPAPERS", "UPDATE 4:3 WALLPAPERS");
    } else {
        addPackAction(0, "INSTALL WALLPAPERS", "UPDATE WALLPAPERS");
    }

    if (gWallpaperInstalledTotal > 0) {
        actions.push_back("REMOVE INSTALLED WALLPAPERS");
    }

    return actions;
}

void App::refreshWallpaperStatus() {
    cachedWallpaperStatus.clear();
    gWallpaperPackTotal.clear();
    gWallpaperPackInstalled.clear();
    gWallpaperPackMissing.clear();
    gWallpaperInstalledTotal = 0;
    gWallpaperMissingTotal = 0;
    gWallpaperStatusChecking = false;
    gWallpaperStatusSource = selectedWallpaperSource;
    gWallpaperStatusToken.clear();
    gWallpaperPollDelay = 0;

    if (!ssh.isConnected()) {
        cachedWallpaperStatus = {"STATUS: DISCONNECTED"};
        return;
    }

    cachedWallpaperStatus = {"STATUS: CHECKING...", "STEP: READING INSTALLED"};
    const int generation = ++gWallpaperStatusGeneration;
    const int source = selectedWallpaperSource;

    SshResult listResult = ssh.runCommand("find /media/fat/wallpapers -maxdepth 1 -type f 2>/dev/null");
    gWallpaperInstalledNames = listResult.success ? installedWallpaperNameSet(listResult.output) : std::set<std::string>();

    SshResult active = ssh.runCommand("test -f /media/fat/menu.jpg -o -f /media/fat/menu.png && echo YES || echo NO");
    gWallpaperStaticActive = active.success ? trim(active.output) : "NO";
    if (gWallpaperStaticActive.empty()) gWallpaperStaticActive = "NO";

    SshResult savedResult = ssh.runCommand("cat /media/fat/Scripts/.config/static_wallpaper/selected_wallpaper.txt 2>/dev/null | sed 's#.*/##'");
    gWallpaperStaticName = savedResult.success ? trim(savedResult.output) : "";
    if (gWallpaperStaticName.empty()) gWallpaperStaticName = "None";
    gWallpaperStaticName = ellipsizeText(gWallpaperStaticName, 34);

    cachedWallpaperStatus = {"STATUS: CHECKING...", "STEP: DOWNLOADING DATABASE"};
    gWallpaperStatusChecking = true;
    gWallpaperStatusSource = source;
    gWallpaperWorkerGeneration = generation;
    gWallpaperWorkerSource = source;
    gWallpaperStatusToken = "/tmp/mc_vita_wallpaper_db_" + std::to_string(generation) + "_" + std::to_string(source);

    const std::string token = gWallpaperStatusToken;
    const std::string dbUrl = wallpaperDbUrlForSource(source);
    std::string command;
    command += "rm -f " + shellQuote(token + ".b64") + " " + shellQuote(token + ".done") + "; ";
    command += "( ";
    command += "if command -v base64 >/dev/null 2>&1 && wget --no-check-certificate -q -O - " + shellQuote(dbUrl) + " | base64 > " + shellQuote(token + ".b64") + "; then ";
    command += "if [ -s " + shellQuote(token + ".b64") + " ]; then echo OK > " + shellQuote(token + ".done") + "; else echo FAIL > " + shellQuote(token + ".done") + "; fi; ";
    command += "else echo FAIL > " + shellQuote(token + ".done") + "; fi ";
    command += ") >/dev/null 2>&1 &";

    SshResult started = ssh.runCommand(command);
    if (!started.success) {
        gWallpaperStatusChecking = false;
        cachedWallpaperStatus = {"STATUS: CHECK FAILED", "Unable to start DB check."};
    }
}

void App::installWallpaperPack(const std::string& title, const std::string& dbUrl, const std::string& rawBase, const std::string& filterMode) {
    (void)dbUrl;
    (void)rawBase;
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }

    std::vector<WallpaperDb::Entry> entries;
    std::string error;
    SshResult dbResult = ssh.runCommand("wget --no-check-certificate -q -O - " + shellQuote(wallpaperDbUrlForSource(selectedWallpaperSource)) + " | base64");
    std::vector<unsigned char> zipData = dbResult.success ? decodeBase64Text(dbResult.output) : std::vector<unsigned char>();
    if (!dbResult.success || !wallpaperEntriesFromZipBytes(selectedWallpaperSource, filterMode, zipData, entries, error)) {
        if (error.empty()) error = dbResult.error.empty() ? "Unable to download wallpaper database." : dbResult.error;
        showOutputWindow("INSTALL " + title, "Failed to read wallpaper database.\n\n" + error);
        return;
    }

    showStreamingCommandWindow("INSTALL " + title, wallpaperInstallCommandFromEntries(entries), title + " installed.", title + " install failed.");
    refreshWallpaperStatus();
}

void App::removeWallpaperPack(const std::string& title, const std::string& dbUrl, const std::string& rawBase, const std::string& filterMode) {
    (void)dbUrl;
    (void)rawBase;
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }
    if (!confirm(("REMOVE " + title).c_str(), "REMOVE INSTALLED WALLPAPERS FROM THIS SOURCE?")) return;

    std::vector<WallpaperDb::Entry> entries;
    std::string error;
    SshResult dbResult = ssh.runCommand("wget --no-check-certificate -q -O - " + shellQuote(wallpaperDbUrlForSource(selectedWallpaperSource)) + " | base64");
    std::vector<unsigned char> zipData = dbResult.success ? decodeBase64Text(dbResult.output) : std::vector<unsigned char>();
    if (!dbResult.success || !wallpaperEntriesFromZipBytes(selectedWallpaperSource, filterMode, zipData, entries, error)) {
        if (error.empty()) error = dbResult.error.empty() ? "Unable to download wallpaper database." : dbResult.error;
        showOutputWindow("REMOVE " + title, "Failed to read wallpaper database.\n\n" + error);
        return;
    }

    showStreamingCommandWindow("REMOVE " + title, wallpaperRemoveCommandFromEntries(entries), title + " removed.", title + " remove failed.");
    refreshWallpaperStatus();
}

void App::executeWallpaperAction(int actionIndex) {
    std::vector<std::string> actions = wallpaperActions(selectedWallpaperSource);
    if (actionIndex < 0 || actionIndex >= static_cast<int>(actions.size())) return;
    const std::string action = actions[actionIndex];

    switch (selectedWallpaperSource) {
        case 0:
            if (action == "INSTALL 16:9 WALLPAPERS" || action == "UPDATE 16:9 WALLPAPERS") installWallpaperPack("RANNY 16:9 WALLPAPERS", RannyDbUrl, RannyRawBase, "169");
            else if (action == "INSTALL 4:3 WALLPAPERS" || action == "UPDATE 4:3 WALLPAPERS") installWallpaperPack("RANNY 4:3 WALLPAPERS", RannyDbUrl, RannyRawBase, "43");
            else if (action == "REMOVE INSTALLED WALLPAPERS") removeWallpaperPack("RANNY WALLPAPERS", RannyDbUrl, RannyRawBase, "all");
            break;
        case 1:
            if (action == "INSTALL WALLPAPERS" || action == "UPDATE WALLPAPERS") installWallpaperPack("PCN CHALLENGE WALLPAPERS", PcnDbUrl, PcnRawBase, "all");
            else if (action == "REMOVE INSTALLED WALLPAPERS") removeWallpaperPack("PCN CHALLENGE WALLPAPERS", PcnDbUrl, PcnRawBase, "all");
            break;
        case 2:
            if (action == "INSTALL WALLPAPERS" || action == "UPDATE WALLPAPERS") installWallpaperPack("PCN PREMIUM WALLPAPERS", PcnPremiumDbUrl, PcnPremiumRawBase, "all");
            else if (action == "REMOVE INSTALLED WALLPAPERS") removeWallpaperPack("PCN PREMIUM WALLPAPERS", PcnPremiumDbUrl, PcnPremiumRawBase, "all");
            break;
        case 3:
            if (action == "INSTALL WALLPAPERS" || action == "UPDATE WALLPAPERS") installWallpaperPack("ANIME0T4KU WALLPAPERS", Ot4kuDbUrl, Ot4kuRawBase, "all");
            else if (action == "REMOVE INSTALLED WALLPAPERS") removeWallpaperPack("ANIME0T4KU WALLPAPERS", Ot4kuDbUrl, Ot4kuRawBase, "all");
            break;
    }
}

void App::handleWallpapersInput(u64 buttons) {
    if (buttons & HidNpadButton_Left) {
        selectedWallpaperSource = (selectedWallpaperSource + WallpaperSourceCount - 1) % WallpaperSourceCount;
        selectedWallpaperAction = 0;
        refreshWallpaperStatus();
        return;
    }
    if (buttons & HidNpadButton_Right) {
        selectedWallpaperSource = (selectedWallpaperSource + 1) % WallpaperSourceCount;
        selectedWallpaperAction = 0;
        refreshWallpaperStatus();
        return;
    }
    if (buttons & HidNpadButton_Minus) {
        showStaticWallpaperMenu();
        refreshWallpaperStatus();
        return;
    }

    std::vector<std::string> actions = wallpaperActions(selectedWallpaperSource);
    const int actionCount = static_cast<int>(actions.size());
    if (actionCount <= 0) {
        selectedWallpaperAction = 0;
        return;
    }
    if (selectedWallpaperAction >= actionCount) selectedWallpaperAction = actionCount - 1;

    if (buttons & HidNpadButton_Up) selectedWallpaperAction = (selectedWallpaperAction + actionCount - 1) % actionCount;
    if (buttons & HidNpadButton_Down) selectedWallpaperAction = (selectedWallpaperAction + 1) % actionCount;
    if (!(buttons & HidNpadButton_A)) return;

    if (!ssh.isConnected()) {
        lastMessage = "Connect to a MiSTer first.";
        return;
    }

    executeWallpaperAction(selectedWallpaperAction);
}


struct PreviewImage {
    int width = 0;
    int height = 0;
    std::vector<unsigned char> rgba;
    std::string status;
};


static void shrinkPreviewImage(PreviewImage& image, int maxW = 320, int maxH = 180) {
    if (image.rgba.empty() || image.width <= 0 || image.height <= 0) return;
    if (image.width <= maxW && image.height <= maxH) return;

    const float srcAspect = static_cast<float>(image.width) / static_cast<float>(image.height);
    int newW = maxW;
    int newH = static_cast<int>(newW / srcAspect);
    if (newH > maxH) {
        newH = maxH;
        newW = static_cast<int>(newH * srcAspect);
    }
    newW = std::max(1, newW);
    newH = std::max(1, newH);

    std::vector<unsigned char> scaled(static_cast<size_t>(newW) * static_cast<size_t>(newH) * 4, 255);
    for (int y = 0; y < newH; ++y) {
        const int sy = std::min(image.height - 1, y * image.height / newH);
        for (int x = 0; x < newW; ++x) {
            const int sx = std::min(image.width - 1, x * image.width / newW);
            const unsigned char* src = image.rgba.data() + (static_cast<size_t>(sy) * image.width + sx) * 4;
            unsigned char* dst = scaled.data() + (static_cast<size_t>(y) * newW + x) * 4;
            dst[0] = src[0];
            dst[1] = src[1];
            dst[2] = src[2];
            dst[3] = src[3];
        }
    }
    image.width = newW;
    image.height = newH;
    image.rgba.swap(scaled);
}

static unsigned int readBe32(const unsigned char* p) {
    return (static_cast<unsigned int>(p[0]) << 24) |
           (static_cast<unsigned int>(p[1]) << 16) |
           (static_cast<unsigned int>(p[2]) << 8) |
           static_cast<unsigned int>(p[3]);
}

static int pngChannelsForColorType(int colorType) {
    switch (colorType) {
        case 0: return 1;
        case 2: return 3;
        case 3: return 1;
        case 4: return 2;
        case 6: return 4;
    }
    return 0;
}

static unsigned char paethPredictor(unsigned char a, unsigned char b, unsigned char c) {
    const int p = static_cast<int>(a) + static_cast<int>(b) - static_cast<int>(c);
    const int pa = std::abs(p - static_cast<int>(a));
    const int pb = std::abs(p - static_cast<int>(b));
    const int pc = std::abs(p - static_cast<int>(c));
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

static unsigned char pngSample8(const unsigned char* data, int sampleIndex, int bitDepth) {
    if (bitDepth == 16) return data[sampleIndex * 2];
    return data[sampleIndex];
}

static bool decodePngRgba(const std::vector<unsigned char>& png, PreviewImage& out) {
    static const unsigned char signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    if (png.size() < 33 || std::memcmp(png.data(), signature, 8) != 0) {
        out.status = "Not a PNG image.";
        return false;
    }

    int width = 0;
    int height = 0;
    int bitDepth = 0;
    int colorType = -1;
    int interlace = 0;
    std::vector<unsigned char> idat;
    std::vector<unsigned char> palette;
    std::vector<unsigned char> trns;

    size_t pos = 8;
    while (pos + 12 <= png.size()) {
        const unsigned int len = readBe32(&png[pos]);
        pos += 4;
        if (pos + 4 + len + 4 > png.size()) {
            out.status = "Invalid PNG chunk.";
            return false;
        }
        std::string type(reinterpret_cast<const char*>(&png[pos]), 4);
        pos += 4;
        const unsigned char* data = &png[pos];

        if (type == "IHDR") {
            if (len < 13) {
                out.status = "Invalid PNG header.";
                return false;
            }
            width = static_cast<int>(readBe32(data));
            height = static_cast<int>(readBe32(data + 4));
            bitDepth = data[8];
            colorType = data[9];
            interlace = data[12];
        } else if (type == "PLTE") {
            palette.assign(data, data + len);
        } else if (type == "tRNS") {
            trns.assign(data, data + len);
        } else if (type == "IDAT") {
            idat.insert(idat.end(), data, data + len);
        } else if (type == "IEND") {
            break;
        }
        pos += len + 4;
    }

    if (width <= 0 || height <= 0 || width > 2048 || height > 2048 || static_cast<long long>(width) * static_cast<long long>(height) > 3000000LL) {
        out.status = "Preview image is too large for Vita.";
        return false;
    }
    if ((bitDepth != 8 && bitDepth != 16) || interlace != 0) {
        out.status = "Unsupported PNG format.";
        return false;
    }

    const int channels = pngChannelsForColorType(colorType);
    if (channels <= 0 || idat.empty()) {
        out.status = "Unsupported PNG color type.";
        return false;
    }

    const int bytesPerSample = bitDepth == 16 ? 2 : 1;
    const size_t stride = static_cast<size_t>(width) * static_cast<size_t>(channels) * static_cast<size_t>(bytesPerSample);
    const size_t expected = (stride + 1) * static_cast<size_t>(height);
    std::vector<unsigned char> inflated(expected);
    uLongf destLen = static_cast<uLongf>(inflated.size());
    int zrc = uncompress(inflated.data(), &destLen, idat.data(), static_cast<uLong>(idat.size()));
    if (zrc != Z_OK || destLen < expected) {
        out.status = "Unable to decompress PNG.";
        return false;
    }

    std::vector<unsigned char> raw(stride * static_cast<size_t>(height));
    const int bpp = channels * bytesPerSample;
    size_t inPos = 0;
    for (int y = 0; y < height; y++) {
        const int filter = inflated[inPos++];
        unsigned char* row = raw.data() + static_cast<size_t>(y) * stride;
        const unsigned char* prev = y > 0 ? raw.data() + static_cast<size_t>(y - 1) * stride : nullptr;
        for (size_t x = 0; x < stride; x++) {
            const unsigned char value = inflated[inPos++];
            const unsigned char left = x >= static_cast<size_t>(bpp) ? row[x - bpp] : 0;
            const unsigned char up = prev ? prev[x] : 0;
            const unsigned char upLeft = (prev && x >= static_cast<size_t>(bpp)) ? prev[x - bpp] : 0;
            unsigned char recon = value;
            switch (filter) {
                case 0: recon = value; break;
                case 1: recon = static_cast<unsigned char>(value + left); break;
                case 2: recon = static_cast<unsigned char>(value + up); break;
                case 3: recon = static_cast<unsigned char>(value + ((static_cast<int>(left) + static_cast<int>(up)) / 2)); break;
                case 4: recon = static_cast<unsigned char>(value + paethPredictor(left, up, upLeft)); break;
                default:
                    out.status = "Unsupported PNG filter.";
                    return false;
            }
            row[x] = recon;
        }
    }

    out.width = width;
    out.height = height;
    out.rgba.assign(static_cast<size_t>(width) * static_cast<size_t>(height) * 4, 255);

    for (int y = 0; y < height; y++) {
        const unsigned char* row = raw.data() + static_cast<size_t>(y) * stride;
        for (int x = 0; x < width; x++) {
            unsigned char* dst = out.rgba.data() + (static_cast<size_t>(y) * width + x) * 4;
            if (colorType == 6) {
                const unsigned char* src = row + static_cast<size_t>(x) * 4 * bytesPerSample;
                dst[0] = pngSample8(src, 0, bitDepth);
                dst[1] = pngSample8(src, 1, bitDepth);
                dst[2] = pngSample8(src, 2, bitDepth);
                dst[3] = pngSample8(src, 3, bitDepth);
            } else if (colorType == 2) {
                const unsigned char* src = row + static_cast<size_t>(x) * 3 * bytesPerSample;
                dst[0] = pngSample8(src, 0, bitDepth);
                dst[1] = pngSample8(src, 1, bitDepth);
                dst[2] = pngSample8(src, 2, bitDepth);
                dst[3] = 255;
            } else if (colorType == 0) {
                const unsigned char* src = row + static_cast<size_t>(x) * bytesPerSample;
                const unsigned char v = pngSample8(src, 0, bitDepth);
                dst[0] = v; dst[1] = v; dst[2] = v; dst[3] = 255;
            } else if (colorType == 4) {
                const unsigned char* src = row + static_cast<size_t>(x) * 2 * bytesPerSample;
                const unsigned char v = pngSample8(src, 0, bitDepth);
                dst[0] = v; dst[1] = v; dst[2] = v; dst[3] = pngSample8(src, 1, bitDepth);
            } else if (colorType == 3) {
                const unsigned char idx = row[x];
                const size_t pi = static_cast<size_t>(idx) * 3;
                if (pi + 2 >= palette.size()) {
                    dst[0] = dst[1] = dst[2] = 0;
                    dst[3] = 255;
                } else {
                    dst[0] = palette[pi];
                    dst[1] = palette[pi + 1];
                    dst[2] = palette[pi + 2];
                    dst[3] = idx < trns.size() ? trns[idx] : 255;
                }
            }
        }
    }

    shrinkPreviewImage(out);
    out.status = "Preview loaded.";
    return true;
}

struct JpegErrorManager {
    jpeg_error_mgr pub;
    jmp_buf jump;
    char message[JMSG_LENGTH_MAX];
};

static void jpegErrorExit(j_common_ptr cinfo) {
    JpegErrorManager* err = reinterpret_cast<JpegErrorManager*>(cinfo->err);
    if (err) {
        (*cinfo->err->format_message)(cinfo, err->message);
        longjmp(err->jump, 1);
    }
}

static bool decodeJpegRgba(const std::vector<unsigned char>& jpg, PreviewImage& out) {
    if (jpg.size() < 4 || jpg[0] != 0xFF || jpg[1] != 0xD8) {
        out.status = "Not a JPEG image.";
        return false;
    }

    jpeg_decompress_struct cinfo{};
    JpegErrorManager jerr{};
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = jpegErrorExit;

    if (setjmp(jerr.jump)) {
        jpeg_destroy_decompress(&cinfo);
        out.status = jerr.message[0] ? std::string(jerr.message) : "Unable to decode JPEG.";
        return false;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, jpg.data(), static_cast<unsigned long>(jpg.size()));

    if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
        jpeg_destroy_decompress(&cinfo);
        out.status = "Invalid JPEG header.";
        return false;
    }

    cinfo.out_color_space = JCS_RGB;
    jpeg_start_decompress(&cinfo);

    const int width = static_cast<int>(cinfo.output_width);
    const int height = static_cast<int>(cinfo.output_height);
    const int comps = static_cast<int>(cinfo.output_components);
    if (width <= 0 || height <= 0 || width > 2048 || height > 2048 || static_cast<long long>(width) * static_cast<long long>(height) > 3000000LL || comps < 3) {
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        out.status = "Preview image is too large for Vita.";
        return false;
    }

    out.width = width;
    out.height = height;
    out.rgba.assign(static_cast<size_t>(width) * static_cast<size_t>(height) * 4, 255);

    std::vector<unsigned char> row(static_cast<size_t>(width) * static_cast<size_t>(comps));
    while (cinfo.output_scanline < cinfo.output_height) {
        JSAMPROW rows[1] = { row.data() };
        const unsigned int y = cinfo.output_scanline;
        jpeg_read_scanlines(&cinfo, rows, 1);
        unsigned char* dst = out.rgba.data() + static_cast<size_t>(y) * static_cast<size_t>(width) * 4;
        for (int x = 0; x < width; x++) {
            const unsigned char* src = row.data() + static_cast<size_t>(x) * static_cast<size_t>(comps);
            dst[x * 4 + 0] = src[0];
            dst[x * 4 + 1] = src[1];
            dst[x * 4 + 2] = src[2];
            dst[x * 4 + 3] = 255;
        }
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    shrinkPreviewImage(out);
    out.status = "Preview loaded.";
    return true;
}

void App::showStaticWallpaperMenu() {
    if (!ssh.isConnected()) {
        lastMessage = "Connect to a MiSTer first.";
        return;
    }

    auto loadWallpapers = [&]() -> std::vector<std::string> {
        SshResult r = ssh.runCommand("find /media/fat/wallpapers -maxdepth 1 -type f \\( -iname '*.png' -o -iname '*.jpg' -o -iname '*.jpeg' \\) 2>/dev/null | sort");
        std::vector<std::string> out;
        if (!r.success) return out;
        for (const std::string& line : splitLines(r.output)) {
            std::string path = trim(line);
            if (!path.empty()) out.push_back(path);
        }
        return out;
    };

    std::vector<std::string> wallpapers = loadWallpapers();
    int selectedWallpaper = 0;
    int scroll = 0;
    std::string statusText = wallpapers.empty() ? "No wallpapers found." : "Select a wallpaper.";
    std::string activePath = trim(runCommandMessage("cat /media/fat/Scripts/.config/static_wallpaper/selected_wallpaper.txt 2>/dev/null"));
    PreviewImage preview;
    std::string previewPath;
    int pendingPreviewIndex = -1;
    int previewDelayFrames = 0;

    auto schedulePreview = [&](int index) {
        if (index < 0 || index >= static_cast<int>(wallpapers.size())) return;
        const std::string& remotePath = wallpapers[index];
        if (remotePath.empty() || remotePath == previewPath || index == pendingPreviewIndex) return;
        pendingPreviewIndex = index;
        previewDelayFrames = 10;
        preview = PreviewImage{};
        preview.status = "Preview loading...";
    };

    auto loadPreview = [&](int index) {
        if (index < 0 || index >= static_cast<int>(wallpapers.size())) return;
        const std::string& remotePath = wallpapers[index];
        if (remotePath.empty() || remotePath == previewPath) return;
        previewPath = remotePath;
        pendingPreviewIndex = -1;
        preview = PreviewImage{};

        const int remoteLine = index + 1;
        const std::string encodedCommand =
            "LINE=" + std::to_string(remoteLine) + "; "
            "FOUND=$(find /media/fat/wallpapers -maxdepth 1 -type f \\( -iname '*.png' -o -iname '*.jpg' -o -iname '*.jpeg' \\) 2>/dev/null | sort | sed -n \"$LINE\"p); "
            "if [ -n \"$FOUND\" ] && [ -f \"$FOUND\" ]; then "
            "SIZE=$(wc -c < \"$FOUND\" 2>/dev/null || echo 0); "
            "if [ \"$SIZE\" -gt 4194304 ]; then echo __MC_VITA_PREVIEW_TOO_LARGE__; else base64 \"$FOUND\"; fi; "
            "else echo __MC_NX_PREVIEW_FILE_MISSING__; fi";

        SshResult encodedResult = ssh.runCommand(encodedCommand);
        if (!encodedResult.success) {
            preview.status = encodedResult.error.empty() ? "Unable to read preview image." : encodedResult.error;
            return;
        }

        if (encodedResult.output.find("__MC_NX_PREVIEW_FILE_MISSING__") != std::string::npos) {
            preview.status = "Preview file was not found on MiSTer.";
            return;
        }
        if (encodedResult.output.find("__MC_VITA_PREVIEW_TOO_LARGE__") != std::string::npos) {
            preview.status = "Preview is too large for Vita.";
            return;
        }
        if (encodedResult.output.find("__MC_VITA_PREVIEW_TOO_LARGE__") != std::string::npos) {
            preview.status = "Preview is too large for Vita.";
            return;
        }
        if (encodedResult.output.find("__MC_VITA_PREVIEW_TOO_LARGE__") != std::string::npos) {
            preview.status = "Preview is too large for Vita.";
            return;
        }

        std::vector<unsigned char> bytes = decodeBase64Text(encodedResult.output);
        if (bytes.empty()) {
            preview.status = "Unable to decode preview image.";
            return;
        }

        const bool isPng = bytes.size() >= 8 &&
            bytes[0] == 0x89 && bytes[1] == 0x50 && bytes[2] == 0x4e && bytes[3] == 0x47 &&
            bytes[4] == 0x0d && bytes[5] == 0x0a && bytes[6] == 0x1a && bytes[7] == 0x0a;
        const bool isJpg = bytes.size() >= 3 && bytes[0] == 0xff && bytes[1] == 0xd8 && bytes[2] == 0xff;

        if (isPng) decodePngRgba(bytes, preview);
        else if (isJpg) decodeJpegRgba(bytes, preview);
        else preview.status = "Unsupported preview format.";
    };

    if (!wallpapers.empty()) schedulePreview(selectedWallpaper);

    bool inputReleased = false;
    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, 92, UiRenderer::rgb(20, 16, 34));
        ui.fillRect(0, 90, UiRenderer::Width, 4, UiRenderer::rgb(143, 84, 255));
        ui.drawText(42, 30, "STATIC WALLPAPER", UiRenderer::rgb(248, 245, 255), 3);
        ui.drawStatusPill(UiRenderer::Width - 280, 28, ssh.isConnected() ? "CONNECTED" : "DISCONNECTED", ssh.isConnected());

        ui.drawCard(40, 124, 520, 500, "WALLPAPERS");
        ui.drawCard(590, 124, 650, 500, "PREVIEW");

        const int visible = 8;
        if (selectedWallpaper < scroll) scroll = selectedWallpaper;
        if (selectedWallpaper >= scroll + visible) scroll = selectedWallpaper - visible + 1;
        if (scroll < 0) scroll = 0;
        if (scroll > std::max(0, static_cast<int>(wallpapers.size()) - visible)) scroll = std::max(0, static_cast<int>(wallpapers.size()) - visible);

        int y = 186;
        for (int i = scroll; i < static_cast<int>(wallpapers.size()) && i < scroll + visible; i++) {
            std::string name = wallpapers[i];
            size_t slash = name.find_last_of('/');
            if (slash != std::string::npos) name = name.substr(slash + 1);
            const bool active = i == selectedWallpaper;
            const bool isApplied = wallpapers[i] == activePath;
            ui.drawButton(76, y, 448, 46, (isApplied ? "* " : "  ") + ellipsizeText(name, 32), active, false, false);
            y += 54;
        }

        if (!wallpapers.empty()) {
            std::string selectedPath = wallpapers[selectedWallpaper];
            std::string selectedName = selectedPath;
            size_t slash = selectedName.find_last_of('/');
            if (slash != std::string::npos) selectedName = selectedName.substr(slash + 1);
            ui.drawText(626, 198, "SELECTED", UiRenderer::rgb(174, 154, 218), 2);
            ui.drawText(626, 236, ellipsizeText(selectedName, 46), UiRenderer::rgb(248, 245, 255), 2);
            ui.drawText(626, 300, "PREVIEW", UiRenderer::rgb(174, 154, 218), 2);
            ui.fillRect(626, 338, 578, 210, UiRenderer::rgb(20, 16, 34));
            ui.drawRect(626, 338, 578, 210, UiRenderer::rgb(67, 57, 92), 2);
            if (!preview.rgba.empty() && preview.width > 0 && preview.height > 0) {
                ui.drawImageRgba(626, 338, 578, 210, preview.rgba.data(), preview.width, preview.height);
            } else {
                ui.drawTextCentered(626, 424, 578, ellipsizeText(preview.status.empty() ? "NO PREVIEW" : preview.status, 45), UiRenderer::rgb(174, 154, 218), 2);
            }
            ui.drawText(626, 570, ellipsizeText(selectedPath == activePath ? "CURRENT STATIC WALLPAPER" : statusText, 48), selectedPath == activePath ? UiRenderer::rgb(112, 232, 165) : UiRenderer::rgb(218, 208, 238), 2);
        } else {
            ui.drawText(76, 206, "NO WALLPAPERS FOUND", UiRenderer::rgb(232, 190, 120), 3);
            ui.drawText(626, 240, "INSTALL WALLPAPERS FIRST.", UiRenderer::rgb(218, 208, 238), 2);
        }

        ui.drawFooter("UP/DN SELECT    ×/□ APPLY    △ REMOVE    ○/SEL BACK");
        ui.endFrame();

        padUpdate(&pad);
        const u64 held = padGetButtons(&pad);
        const u64 buttons = padGetButtonsDown(&pad);
        if (!inputReleased) {
            if ((held & HidNpadButton_Minus) == 0) inputReleased = true;
            continue;
        }

        if (buttons & (HidNpadButton_Minus | HidNpadButton_B)) return;
        if (!wallpapers.empty() && (buttons & HidNpadButton_Up)) {
            selectedWallpaper = (selectedWallpaper + static_cast<int>(wallpapers.size()) - 1) % static_cast<int>(wallpapers.size());
            schedulePreview(selectedWallpaper);
        }
        if (!wallpapers.empty() && (buttons & HidNpadButton_Down)) {
            selectedWallpaper = (selectedWallpaper + 1) % static_cast<int>(wallpapers.size());
            schedulePreview(selectedWallpaper);
        }

        if (pendingPreviewIndex >= 0 && (held & (HidNpadButton_Up | HidNpadButton_Down)) == 0) {
            if (previewDelayFrames > 0) previewDelayFrames--;
            if (previewDelayFrames <= 0) loadPreview(pendingPreviewIndex);
        }

        if (!wallpapers.empty() && (buttons & (HidNpadButton_X | HidNpadButton_A))) {
            std::string selectedPath = wallpapers[selectedWallpaper];
            std::string selectedName = selectedPath;
            size_t selectedSlash = selectedName.find_last_of('/');
            if (selectedSlash != std::string::npos) selectedName = selectedName.substr(selectedSlash + 1);
            std::string ext = toLower(selectedName.substr(selectedName.find_last_of('.') == std::string::npos ? selectedName.size() : selectedName.find_last_of('.')));
            std::string target = (ext == ".png") ? StaticWallpaperTargetPng : StaticWallpaperTargetJpg;
            std::string other = (ext == ".png") ? StaticWallpaperTargetJpg : StaticWallpaperTargetPng;
            const int remoteLine = selectedWallpaper + 1;
            std::string command =
                "LINE=" + std::to_string(remoteLine) + "; "
                "SELECTED=$(find /media/fat/wallpapers -maxdepth 1 -type f \\( -iname '*.png' -o -iname '*.jpg' -o -iname '*.jpeg' \\) 2>/dev/null | sort | sed -n \"$LINE\"p); "
                "if [ -z \"$SELECTED\" ] || [ ! -f \"$SELECTED\" ]; then echo 'Selected wallpaper was not found.'; exit 1; fi; "
                "mkdir -p " + std::string(StaticWallpaperConfigDir) + " && "
                "rm -f " + shellQuote(other) + " && "
                "cp \"$SELECTED\" " + shellQuote(target) + " && "
                "rm -f " + shellQuote(other) + " && "
                "printf %s \"$SELECTED\" > " + shellQuote(StaticWallpaperConfigPath) + " && "
                "sync && " + std::string(MisterMenuReloadCommand);
            SshResult result = ssh.runCommand(command);
            if (result.success) {
                activePath = selectedPath;
                statusText = "Static wallpaper applied. MiSTer menu reloaded.";
            } else {
                statusText = result.error.empty() ? "Unable to apply static wallpaper." : result.error;
            }
        }
        if (buttons & HidNpadButton_Y) {
            if (confirm("REMOVE STATIC WALLPAPER", "REMOVE CURRENT STATIC WALLPAPER?")) {
                SshResult result = ssh.runCommand("rm -f /media/fat/menu.jpg /media/fat/menu.png; sync; " + std::string(MisterMenuReloadCommand));
                if (result.success) {
                    activePath.clear();
                    statusText = "Static wallpaper removed. MiSTer menu reloaded.";
                } else {
                    statusText = result.error.empty() ? "Unable to remove static wallpaper." : result.error;
                }
            }
        }
    }
}

void App::handleExtrasInput(u64 buttons) {
    if (buttons & HidNpadButton_Left) {
        selectedExtra = (selectedExtra + ExtraCount - 1) % ExtraCount;
        selected = 0;
        refreshCurrentExtraStatus(false);
        return;
    }
    if (buttons & HidNpadButton_Right) {
        selectedExtra = (selectedExtra + 1) % ExtraCount;
        selected = 0;
        refreshCurrentExtraStatus(false);
        return;
    }

    std::vector<std::string> actions = extraActions(static_cast<ExtraId>(selectedExtra));
    if (actions.empty()) return;

    if (buttons & HidNpadButton_Up) selected = (selected + static_cast<int>(actions.size()) - 1) % static_cast<int>(actions.size());
    if (buttons & HidNpadButton_Down) selected = (selected + 1) % static_cast<int>(actions.size());
    if (!(buttons & HidNpadButton_A)) return;

    if (!ssh.isConnected()) {
        lastMessage = "Connect to a MiSTer first.";
        return;
    }

    executeExtraAction(static_cast<ExtraId>(selectedExtra), selected);
}


void App::handleSettingsInput(u64 buttons) {
    if (!ssh.isConnected()) {
        if (buttons & HidNpadButton_A) lastMessage = "Connect to a MiSTer first.";
        return;
    }

    if (!settingsLoaded) {
        if (buttons & HidNpadButton_A) loadSettingsTab(true);
        return;
    }

    const int count = settingsOptionCount();

    if ((buttons & HidNpadButton_Left) || (buttons & HidNpadButton_Right)) {
        if (!settingsIniFiles.empty()) {
            auto it = std::find(settingsIniFiles.begin(), settingsIniFiles.end(), selectedSettingsIni);
            int index = it == settingsIniFiles.end() ? 0 : static_cast<int>(it - settingsIniFiles.begin());
            if (buttons & HidNpadButton_Left) index = (index + static_cast<int>(settingsIniFiles.size()) - 1) % static_cast<int>(settingsIniFiles.size());
            if (buttons & HidNpadButton_Right) index = (index + 1) % static_cast<int>(settingsIniFiles.size());
            selectedSettingsIni = settingsIniFiles[index];
            loadSelectedSettingsIni();
            settingsDirty = false;
            lastMessage = "Loaded " + selectedSettingsIni + ".";
        }
        return;
    }

    if (buttons & HidNpadButton_Up) selectedSettings = (selectedSettings + count - 1) % count;
    if (buttons & HidNpadButton_Down) selectedSettings = (selectedSettings + 1) % count;

    if (buttons & HidNpadButton_A) cycleSettingsValue(selectedSettings);
    if (buttons & HidNpadButton_X) saveSettingsIni();
    if (buttons & HidNpadButton_B) {
        loadSelectedSettingsIni();
        settingsDirty = false;
        lastMessage = "Settings changes discarded.";
    }
}

int App::settingsOptionCount() const {
    return 12;
}

std::string App::settingsLabel(int index) const {
    switch (index) {
        case 0: return "HDMI Mode";
        case 1: return "Resolution";
        case 2: return "HDMI Scaling Mode";
        case 3: return "HDMI Audio";
        case 4: return "HDR";
        case 5: return "HDMI Range";
        case 6: return "Analogue Output";
        case 7: return "MiSTer Logo";
        case 8: return "Font";
        case 9: return "AmigaVision Preset";
        case 10: return "Menu CRT Preset";
        case 11: return "Restore Defaults";
        default: return "";
    }
}

std::string App::settingsValue(int index) const {
    switch (index) {
        case 0: return settingsEasy.hdmiMode;
        case 1: return settingsEasy.resolution;
        case 2: return settingsEasy.scaling;
        case 3: return settingsEasy.hdmiAudio;
        case 4: return settingsEasy.hdr;
        case 5: return settingsEasy.hdmiLimited;
        case 6: return settingsEasy.analogue;
        case 7: return settingsEasy.logo;
        case 8: return safeText(settingsEasy.font, "Default");
        case 9: return settingsEasy.amigavisionPreset;
        case 10: return settingsEasy.menuCrtPreset;
        default: return "";
    }
}

std::vector<std::string> App::settingsOptionsForIndex(int index) const {
    switch (index) {
        case 0: return MisterIni::hdmiModeOptions();
        case 1: return MisterIni::resolutionOptions();
        case 2: return MisterIni::scalingOptions();
        case 3: return MisterIni::hdmiAudioOptions();
        case 4: return MisterIni::hdrOptions();
        case 5: return MisterIni::hdmiRangeOptions();
        case 6: return MisterIni::analogueOptions();
        case 7: return MisterIni::enabledDisabledOptions();
        case 8: return settingsFonts.empty() ? std::vector<std::string>{"Default"} : settingsFonts;
        case 9: return MisterIni::enabledDisabledOptions();
        case 10: return MisterIni::menuCrtPresetOptions();
        default: return {};
    }
}

static std::string nextOptionValue(const std::vector<std::string>& options, const std::string& current) {
    if (options.empty()) return current;
    auto it = std::find(options.begin(), options.end(), current);
    if (it == options.end()) return options.front();
    ++it;
    if (it == options.end()) return options.front();
    return *it;
}

void App::cycleSettingsValue(int index) {
    if (index == 11) {
        restoreSettingsDefaults();
        return;
    }

    std::vector<std::string> options = settingsOptionsForIndex(index);
    if (options.empty()) return;

    if (index == 8) {
        PadState pad;
        padInitializeDefault(&pad);

        while (appletMainLoop()) {
            padUpdate(&pad);
            u64 held = padGetButtons(&pad);
            if ((held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X)) == 0) break;
            ui.beginFrame();
            ui.drawCard(220, 150, 840, 410, "SELECT FONT");
            ui.drawTextCentered(260, 320, 760, "RELEASE BUTTONS", UiRenderer::rgb(174, 154, 218), 2);
            ui.drawFooter("UP/DN SELECT    × SELECT    □ SAVE & BACK    ○ CANCEL");
            ui.endFrame();
        }

        int selectedFont = 0;
        auto it = std::find(options.begin(), options.end(), settingsEasy.font);
        if (it != options.end()) selectedFont = static_cast<int>(it - options.begin());
        int pendingFont = selectedFont;
        int scroll = 0;
        const int visible = 6;

        while (appletMainLoop()) {
            if (pendingFont < scroll) scroll = pendingFont;
            if (pendingFont >= scroll + visible) scroll = pendingFont - visible + 1;
            if (scroll < 0) scroll = 0;
            if (scroll > std::max(0, static_cast<int>(options.size()) - visible)) scroll = std::max(0, static_cast<int>(options.size()) - visible);

            ui.beginFrame();
            ui.drawCard(220, 124, 840, 472, "SELECT FONT");
            ui.drawText(260, 176, "Selected", UiRenderer::rgb(174, 154, 218), 2);
            ui.drawText(410, 176, safeText(options[selectedFont], "Default"), UiRenderer::rgb(248, 245, 255), 2);

            int y = 226;
            for (int i = scroll; i < static_cast<int>(options.size()) && i < scroll + visible; i++) {
                const bool rowSelected = i == pendingFont;
                const bool rowActive = i == selectedFont;
                std::string label = options[i];
                if (label.size() > 40) label = label.substr(0, 37) + "...";
                label = rowActive ? ("[x] " + label) : ("[ ] " + label);
                ui.drawButton(260, y, 760, 46, label, rowSelected, false, false);
                if (rowActive) {
                    ui.drawText(880, y + 15, "SELECTED", UiRenderer::rgb(112, 232, 165), 2);
                }
                y += 56;
            }

            std::string counter = std::to_string(pendingFont + 1) + " / " + std::to_string(options.size());
            ui.drawTextCentered(260, 566, 760, counter, UiRenderer::rgb(174, 154, 218), 2);
            ui.drawFooter("UP/DN MOVE    × SELECT    □ SAVE & BACK    ○ CANCEL");
            ui.endFrame();

            padUpdate(&pad);
            const u64 buttons = padGetButtonsDown(&pad);
            if (buttons & HidNpadButton_Up) pendingFont = (pendingFont + static_cast<int>(options.size()) - 1) % static_cast<int>(options.size());
            if (buttons & HidNpadButton_Down) pendingFont = (pendingFont + 1) % static_cast<int>(options.size());
            if (buttons & HidNpadButton_A) selectedFont = pendingFont;
            if (buttons & HidNpadButton_X) {
                settingsEasy.font = options[selectedFont];
                settingsDirty = true;
                lastMessage = "Font selected.";
                return;
            }
            if (buttons & HidNpadButton_B) {
                lastMessage = "Font selection cancelled.";
                return;
            }
        }
        return;
    }

    switch (index) {
        case 0: settingsEasy.hdmiMode = nextOptionValue(options, settingsEasy.hdmiMode); break;
        case 1: settingsEasy.resolution = nextOptionValue(options, settingsEasy.resolution); break;
        case 2: settingsEasy.scaling = nextOptionValue(options, settingsEasy.scaling); break;
        case 3: settingsEasy.hdmiAudio = nextOptionValue(options, settingsEasy.hdmiAudio); break;
        case 4: settingsEasy.hdr = nextOptionValue(options, settingsEasy.hdr); break;
        case 5: settingsEasy.hdmiLimited = nextOptionValue(options, settingsEasy.hdmiLimited); break;
        case 6: settingsEasy.analogue = nextOptionValue(options, settingsEasy.analogue); break;
        case 7: settingsEasy.logo = nextOptionValue(options, settingsEasy.logo); break;
        case 8: settingsEasy.font = nextOptionValue(options, settingsEasy.font); break;
        case 9: settingsEasy.amigavisionPreset = nextOptionValue(options, settingsEasy.amigavisionPreset); break;
        case 10: settingsEasy.menuCrtPreset = nextOptionValue(options, settingsEasy.menuCrtPreset); break;
    }
    settingsDirty = true;
}
std::string App::extraTitle(ExtraId id) const {
    switch (id) {
        case ExtraId::ZaparooFrontend: return "ZAPAROO FRONTEND";
        case ExtraId::RetroAchievementCores: return "RETROACHIEVEMENT CORES";
        case ExtraId::Mms2GbCore: return "MMS2 GB CORE";
    }
    return "EXTRA";
}

static bool extraStatusInstalled(const std::vector<std::string>& lines) {
    return statusValue(lines, "INSTALLED") == "YES";
}

static bool extraStatusFilesMissing(const std::vector<std::string>& lines) {
    std::string installed = statusValue(lines, "INSTALLED");
    std::string status = statusValue(lines, "STATUS");
    return installed == "FILES MISSING" || status == "FILES MISSING";
}

std::vector<std::string> App::extraActions(ExtraId id) const {
    std::vector<std::string> statusLines = selectedExtra >= 0 && selectedExtra < static_cast<int>(cachedExtraStatus.size())
        ? cachedExtraStatus[selectedExtra]
        : std::vector<std::string>{};
    const bool installed = extraStatusInstalled(statusLines);
    const bool filesMissing = extraStatusFilesMissing(statusLines);
    const bool updateAvailable = extraUpdateAvailable[static_cast<int>(id)];

    std::vector<std::string> actions;
    if (extraUpdateCheckPending && extraUpdateCheckIndex == static_cast<int>(id)) {
        return actions;
    }
    if (!installed) {
        actions.push_back("INSTALL");
        if (id == ExtraId::RetroAchievementCores && filesMissing) {
            actions.push_back("CONFIGURE");
            actions.push_back("UNINSTALL");
        }
        return actions;
    }

    actions.push_back("CHECK FOR UPDATES");
    if (updateAvailable) actions.push_back("UPDATE");
    if (id == ExtraId::RetroAchievementCores) actions.push_back("CONFIGURE");
    actions.push_back("UNINSTALL");
    return actions;
}

static std::string raSourcesHeredoc();

std::vector<std::string> App::extraStatus(ExtraId id) {
    if (!ssh.isConnected()) {
        if (id == ExtraId::ZaparooFrontend || id == ExtraId::Mms2GbCore) return {"INSTALLED: NO", "VERSION: UNKNOWN"};
        return {"INSTALLED: NO"};
    }

    if (id == ExtraId::ZaparooFrontend) {
        const std::string command =
            "INSTALLED=NO; "
            "if [ -f /media/fat/zaparoo/MiSTer_Zaparoo ] && [ -f /media/fat/zaparoo/frontend ] && [ -f /media/fat/zaparoo/menu_zaparoo.rbf ] && grep -q 'main=zaparoo/MiSTer_Zaparoo' /media/fat/MiSTer.ini 2>/dev/null; then INSTALLED=YES; fi; "
            "VERSION=$(cat /media/fat/Scripts/.config/zaparoo_frontend/version.txt 2>/dev/null); "
            "[ -z \"$VERSION\" ] && VERSION=$(cat /media/fat/Scripts/.config/zaparoo_launcher/version.txt 2>/dev/null); "
            "[ -z \"$VERSION\" ] && VERSION=UNKNOWN; "
            "echo INSTALLED: $INSTALLED; echo VERSION: $VERSION";
        return splitLines(runCommandMessage(command));
    }

    if (id == ExtraId::Mms2GbCore) {
        const std::string command = R"SH(
INSTALLED=NO
CORE=$(ls /media/fat/MMS2/Gameboy_*.rbf 2>/dev/null | grep -o 'Gameboy_[0-9][0-9]*\.rbf' | sed 's/Gameboy_//;s/\.rbf//' | sort | tail -1)
if [ -n "$CORE" ] && [ -f "/media/fat/Load GB-GBC Cartridge.mgl" ] && [ -f /media/fat/config/MMS2_GB_Cart.CFG ]; then INSTALLED=YES; fi
VERSION=$CORE
[ -z "$VERSION" ] && VERSION=$(cat /media/fat/Scripts/.config/mms2_gb_core/version.txt 2>/dev/null)
[ -z "$VERSION" ] && VERSION=UNKNOWN
echo INSTALLED: $INSTALLED
echo VERSION: $VERSION
)SH";
        return splitLines(runCommandMessage(command));
    }

    std::string command;
    command += R"SH(
TMP=/tmp/mc_ra_status_$$
rm -rf "$TMP"
mkdir -p "$TMP"
cat > "$TMP/sources.txt" <<'MCRASOURCES'
)SH";
    command += raSourcesHeredoc();
    command += R"SH(MCRASOURCES
FOUND=0
MISSING=0
check_path() {
    if [ -e "$1" ]; then FOUND=1; else MISSING=1; fi
}
check_path /media/fat/MiSTer_RA
check_path /media/fat/achievement.wav
check_path /media/fat/retroachievements.cfg
check_path /media/fat/_RA_Cores
check_path /media/fat/_RA_Cores/Cores
if grep -q '^main=MiSTer_RA' /media/fat/MiSTer.ini 2>/dev/null; then FOUND=1; else MISSING=1; fi
while IFS='|' read -r key title repo kind; do
    [ -z "$key" ] && continue
    [ "$key" = "main" ] && continue
    check_path "/media/fat/_RA_Cores/Cores/$title.rbf"
    check_path "/media/fat/_RA_Cores/$title.mgl"
done < "$TMP/sources.txt"
if [ "$FOUND" = "1" ] && [ "$MISSING" = "0" ]; then
    INSTALLED=YES
elif [ "$FOUND" = "1" ] && [ "$MISSING" = "1" ]; then
    INSTALLED="FILES MISSING"
else
    INSTALLED=NO
fi
rm -rf "$TMP"
echo INSTALLED: $INSTALLED
)SH";
    return splitLines(runCommandMessage(command));
}

void App::refreshCurrentExtraStatus(bool checkLatest) {
    if (cachedExtraStatus.empty()) cachedExtraStatus.assign(ExtraCount, std::vector<std::string>{"STATUS: NOT CHECKED"});
    if (selectedExtra < 0 || selectedExtra >= ExtraCount) selectedExtra = 0;

    ExtraId id = static_cast<ExtraId>(selectedExtra);
    cachedExtraStatus[selectedExtra] = extraStatus(id);

    if (!checkLatest) return;

    if (!ssh.isConnected() || !extraStatusInstalled(cachedExtraStatus[selectedExtra])) {
        lastMessage = "Install this extra before checking for updates.";
        return;
    }

    extraUpdateAvailable[selectedExtra] = false;
    extraLatestVersion[selectedExtra].clear();
    cachedExtraStatus[selectedExtra].push_back("STATUS: CHECKING FOR UPDATES...");
    cachedExtraStatus[selectedExtra].push_back("UPDATES AVAILABLE: CHECKING");
    extraUpdateCheckPending = true;
    extraUpdateCheckIndex = selectedExtra;
    lastMessage = "Checking for updates...";
}

void App::performPendingExtraUpdateCheck() {
    if (!extraUpdateCheckPending) return;

    const int source = extraUpdateCheckIndex;
    extraUpdateCheckPending = false;
    extraUpdateCheckIndex = -1;

    if (source < 0 || source >= ExtraCount) return;
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }

    ExtraId id = static_cast<ExtraId>(source);
    std::vector<std::string> baseLines = extraStatus(id);
    if (!extraStatusInstalled(baseLines)) {
        cachedExtraStatus[source] = baseLines;
        extraUpdateAvailable[source] = false;
        lastMessage = "Install this extra before checking for updates.";
        return;
    }

    int updateCount = 0;
    bool checkFailed = false;

    if (id == ExtraId::ZaparooFrontend) {
        const std::string checkCommand = R"SH(
latest_from_repo() {
    repo="$1"
    latest=$(wget --no-check-certificate --server-response --spider "https://github.com/$repo/releases/latest" 2>&1 | sed -n 's/.*[Ll]ocation: .*\/releases\/tag\/\([^?\r]*\).*/\1/p' | tail -1)
    if [ -z "$latest" ]; then
        json=/tmp/mc_latest_release_$$.json
        wget --no-check-certificate --header='User-Agent: MiSTer-Companion-Vita' -O "$json" "https://api.github.com/repos/$repo/releases/latest" >/dev/null 2>&1
        latest=$(sed -n 's/.*"tag_name"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "$json" | head -1)
        rm -f "$json"
    fi
    printf '%s' "$latest"
}
installed=$(cat /media/fat/Scripts/.config/zaparoo_frontend/version.txt 2>/dev/null)
[ -z "$installed" ] && installed=$(cat /media/fat/Scripts/.config/zaparoo_launcher/version.txt 2>/dev/null)
latest=$(latest_from_repo ZaparooProject/zaparoo-frontend)
echo "MC_ZAP_INSTALLED:$installed"
echo "MC_ZAP_LATEST:$latest"
)SH";
        std::vector<std::string> checkLines = splitLines(runCommandMessage(checkCommand));
        std::string installed = normalizeExtraVersion(statusValue(checkLines, "MC_ZAP_INSTALLED"));
        std::string latest = normalizeExtraVersion(statusValue(checkLines, "MC_ZAP_LATEST"));

        if (latest.empty()) {
            checkFailed = true;
        } else if (!installed.empty() && installed != latest) {
            updateCount = 1;
        }
    } else if (id == ExtraId::Mms2GbCore) {
        const std::string checkCommand = R"SH(
TMP=/tmp/mc_mms2_gb_check_$$
rm -rf "$TMP"
mkdir -p "$TMP"
wget --no-check-certificate --header='User-Agent: MiSTer-Companion-Vita' -O "$TMP/releases.json" "https://api.github.com/repos/Heber-co-uk/Gameboy_MiSTer_Cart/contents/releases?ref=master" >/dev/null 2>&1
latest=$(grep -o 'Gameboy_[0-9][0-9]*\.rbf' "$TMP/releases.json" | sed 's/Gameboy_//;s/\.rbf//' | sort | tail -1)
installed=$(ls /media/fat/MMS2/Gameboy_*.rbf 2>/dev/null | grep -o 'Gameboy_[0-9][0-9]*\.rbf' | sed 's/Gameboy_//;s/\.rbf//' | sort | tail -1)
[ -z "$installed" ] && installed=$(cat /media/fat/Scripts/.config/mms2_gb_core/version.txt 2>/dev/null)
echo "MC_MMS2_INSTALLED:$installed"
echo "MC_MMS2_LATEST:$latest"
rm -rf "$TMP"
)SH";
        std::vector<std::string> checkLines = splitLines(runCommandMessage(checkCommand));
        std::string installed = normalizeExtraVersion(statusValue(checkLines, "MC_MMS2_INSTALLED"));
        std::string latest = normalizeExtraVersion(statusValue(checkLines, "MC_MMS2_LATEST"));

        if (latest.empty()) {
            checkFailed = true;
        } else if (!installed.empty() && installed != latest) {
            updateCount = 1;
        }
    } else {
        std::string checkCommand;
        checkCommand += R"SH(
TMP=/tmp/mc_ra_update_check_$$
rm -rf "$TMP"
mkdir -p "$TMP"
cat > "$TMP/sources.txt" <<'MCRASOURCES'
)SH";
        checkCommand += raSourcesHeredoc();
        checkCommand += R"SH(MCRASOURCES
latest_from_repo() {
    repo="$1"
    latest=$(wget --no-check-certificate --server-response --spider "https://github.com/$repo/releases/latest" 2>&1 | sed -n 's/.*[Ll]ocation: .*\/releases\/tag\/\([^?\r]*\).*/\1/p' | tail -1)
    if [ -z "$latest" ]; then
        json="$TMP/latest.json"
        wget --no-check-certificate --header='User-Agent: MiSTer-Companion-Vita' -O "$json" "https://api.github.com/repos/$repo/releases/latest" >/dev/null 2>&1
        latest=$(sed -n 's/.*"tag_name"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "$json" | head -1)
        rm -f "$json"
    fi
    printf '%s' "$latest"
}
echo MC_RA_INSTALLED_BEGIN
if [ -f /media/fat/Scripts/.config/ra_cores/versions.json ]; then
    cat /media/fat/Scripts/.config/ra_cores/versions.json
fi
echo
echo MC_RA_INSTALLED_END
echo MC_RA_LATEST_BEGIN
while IFS='|' read -r key title repo kind; do
    [ -z "$key" ] && continue
    latest=$(latest_from_repo "$repo")
    echo "MC_LATEST|$key|$title|$repo|$latest"
done < "$TMP/sources.txt"
echo MC_RA_LATEST_END
rm -rf "$TMP"
)SH";

        std::vector<std::string> checkLines = splitLines(runCommandMessage(checkCommand));
        std::string installedJson;
        bool inInstalled = false;
        int latestSeen = 0;
        int latestResolved = 0;
        int parsedInstalled = 0;

        for (const std::string& line : checkLines) {
            if (line == "MC_RA_INSTALLED_BEGIN") {
                inInstalled = true;
                continue;
            }
            if (line == "MC_RA_INSTALLED_END") {
                inInstalled = false;
                continue;
            }
            if (inInstalled) {
                installedJson += line;
                installedJson += '\n';
            }
        }

        for (const std::string& line : checkLines) {
            if (line.rfind("MC_LATEST|", 0) != 0) continue;
            std::vector<std::string> parts = splitByChar(line, '|');
            if (parts.size() < 5) continue;

            const std::string key = parts[1];
            const std::string title = parts[2];
            const std::string repo = parts[3];
            const std::string latest = normalizeExtraVersion(parts[4]);
            latestSeen++;
            if (latest.empty()) continue;
            latestResolved++;

            std::string installed = normalizeExtraVersion(installedRaVersionForSource(installedJson, key, title, repo));
            if (installed.empty()) continue;
            parsedInstalled++;

            if (installed != latest) updateCount++;
        }

        if (latestSeen == 0 || latestResolved == 0) {
            checkFailed = true;
        } else if (parsedInstalled == 0) {
            updateCount = 0;
        }
    }

    baseLines.push_back("UPDATES AVAILABLE: " + (checkFailed ? std::string("UNKNOWN") : std::to_string(updateCount)));
    baseLines.push_back(std::string("UPDATE: ") + (!checkFailed && updateCount > 0 ? "YES" : "NO"));
    cachedExtraStatus[source] = baseLines;
    extraLatestVersion[source].clear();
    extraUpdateAvailable[source] = !checkFailed && updateCount > 0;
    lastMessage = checkFailed
        ? "Update check failed."
        : (updateCount > 0
            ? ("Update check complete. " + std::to_string(updateCount) + " update(s) available.")
            : "Update check complete. No updates available.");
}

static std::string zaparooFrontendInstallShell() {
    return R"SH(
set -u
TMP=/tmp/mc_zaparoo_frontend
rm -rf "$TMP"
mkdir -p "$TMP" /media/fat/zaparoo /media/fat/Scripts/.config/zaparoo_frontend
echo Finding latest Zaparoo Frontend release...
wget --no-check-certificate --header='User-Agent: MiSTer-Companion-Vita' -O "$TMP/release.json" "https://api.github.com/repos/ZaparooProject/zaparoo-frontend/releases/latest" || exit 1
TAG=$(sed -n 's/.*"tag_name"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "$TMP/release.json" | head -1)
if [ -z "$TAG" ]; then echo Unable to determine latest Zaparoo Frontend version.; exit 1; fi
echo Latest version: $TAG
echo Finding release ZIP...
URL=$(sed -n 's/.*"browser_download_url"[[:space:]]*:[[:space:]]*"\([^"]*\.zip\)".*/\1/p' "$TMP/release.json" | head -1)
if [ -z "$URL" ]; then echo Unable to find ZIP asset.; exit 1; fi
echo Downloading package...
wget --no-check-certificate -O "$TMP/frontend.zip" "$URL" || exit 1
if [ ! -s "$TMP/frontend.zip" ]; then echo Downloaded ZIP is empty.; exit 1; fi
echo Extracting package...
unzip -o "$TMP/frontend.zip" -d "$TMP/extract" >/dev/null || exit 1
MAIN=$(find "$TMP/extract" -path '*/zaparoo/MiSTer_Zaparoo' -o -name MiSTer_Zaparoo | head -1)
FRONTEND=$(find "$TMP/extract" -path '*/zaparoo/frontend' -o -name frontend | head -1)
MENU=$(find "$TMP/extract" -path '*/zaparoo/menu_zaparoo.rbf' -o -name menu_zaparoo.rbf | head -1)
if [ -z "$MAIN" ] || [ -z "$FRONTEND" ] || [ -z "$MENU" ]; then echo Required Zaparoo Frontend files missing from ZIP.; exit 1; fi
echo Removing old frontend files...
rm -f /media/fat/zaparoo/MiSTer_Zaparoo /media/fat/zaparoo/frontend /media/fat/zaparoo/launcher /media/fat/zaparoo/menu_zaparoo.rbf
cp "$MAIN" /media/fat/zaparoo/MiSTer_Zaparoo || exit 1
cp "$FRONTEND" /media/fat/zaparoo/frontend || exit 1
cp "$MENU" /media/fat/zaparoo/menu_zaparoo.rbf || exit 1
chmod +x /media/fat/zaparoo/MiSTer_Zaparoo /media/fat/zaparoo/frontend
if [ ! -f /media/fat/MiSTer.ini ]; then
    wget --no-check-certificate -O /media/fat/MiSTer.ini https://raw.githubusercontent.com/Anime0t4ku/mister-companion/main/assets/MiSTer_example.ini || true
fi
if ! grep -q '^main=zaparoo/MiSTer_Zaparoo' /media/fat/MiSTer.ini 2>/dev/null; then
    if grep -q '^\[MiSTer\]' /media/fat/MiSTer.ini 2>/dev/null; then
        awk 'BEGIN{done=0} /^\[MiSTer\]/{print; print "main=zaparoo/MiSTer_Zaparoo"; done=1; next} {print} END{if(done==0){print "[MiSTer]"; print "main=zaparoo/MiSTer_Zaparoo"}}' /media/fat/MiSTer.ini > "$TMP/MiSTer.ini" && mv "$TMP/MiSTer.ini" /media/fat/MiSTer.ini
    else
        printf '[MiSTer]\nmain=zaparoo/MiSTer_Zaparoo\n\n' | cat - /media/fat/MiSTer.ini > "$TMP/MiSTer.ini" && mv "$TMP/MiSTer.ini" /media/fat/MiSTer.ini
    fi
fi
printf '%s\n' "$TAG" > /media/fat/Scripts/.config/zaparoo_frontend/version.txt
rm -rf "$TMP"
sync
echo Zaparoo Frontend installed.
echo A MiSTer menu reload is recommended.
)SH";
}

void App::installOrUpdateZaparooFrontend() {
    const bool success = showStreamingCommandWindow("ZAPAROO FRONTEND", zaparooFrontendInstallShell(), "Zaparoo Frontend installed.", "Zaparoo Frontend install failed.");
    if (success) {
        extraUpdateAvailable[static_cast<int>(ExtraId::ZaparooFrontend)] = false;
        sendSoftRebootCommand();
    }
    refreshCurrentExtraStatus(false);
}

void App::uninstallZaparooFrontend() {
    if (!confirm("UNINSTALL ZAPAROO FRONTEND", "ARE YOU SURE?")) return;
    const std::string command = R"SH(
echo Removing Zaparoo Frontend files...
rm -f /media/fat/zaparoo/MiSTer_Zaparoo /media/fat/zaparoo/frontend /media/fat/zaparoo/launcher /media/fat/zaparoo/menu_zaparoo.rbf
rm -f /media/fat/Scripts/.config/zaparoo_frontend/version.txt /media/fat/Scripts/.config/zaparoo_launcher/version.txt
if [ -f /media/fat/MiSTer.ini ]; then
    grep -v '^main=zaparoo/MiSTer_Zaparoo$' /media/fat/MiSTer.ini | grep -v '^alt_launcher=zaparoo/launcher$' > /tmp/mc_mister_ini_zap && mv /tmp/mc_mister_ini_zap /media/fat/MiSTer.ini
fi
sync
echo Zaparoo Frontend uninstalled.
)SH";
    const bool success = showStreamingCommandWindow("UNINSTALL ZAPAROO FRONTEND", command, "Zaparoo Frontend uninstalled.", "Zaparoo Frontend uninstall failed.");
    extraUpdateAvailable[static_cast<int>(ExtraId::ZaparooFrontend)] = false;
    if (success) {
        sendSoftRebootCommand();
        waitForReconnectAfterReboot("ZAPAROO FRONTEND", "Soft reboot command sent. Waiting for MiSTer...");
    }
    refreshCurrentExtraStatus(false);
}

static std::string mms2GbCoreInstallShell() {
    return R"SH(
set -u
TMP=/tmp/mc_mms2_gb_core
rm -rf "$TMP"
mkdir -p "$TMP" /media/fat/MMS2 /media/fat/config /media/fat/Scripts/.config/mms2_gb_core

echo Finding latest MMS2 GB core...
wget --no-check-certificate --header='User-Agent: MiSTer-Companion-Vita' -O "$TMP/releases.json" "https://api.github.com/repos/Heber-co-uk/Gameboy_MiSTer_Cart/contents/releases?ref=master" || exit 1
LATEST=$(grep -o 'Gameboy_[0-9][0-9]*\.rbf' "$TMP/releases.json" | sed 's/Gameboy_//;s/\.rbf//' | sort | tail -1)
if [ -z "$LATEST" ]; then echo Unable to determine latest MMS2 GB core.; exit 1; fi
FILE="Gameboy_${LATEST}.rbf"
URL=$(sed -n 's/.*"download_url"[[:space:]]*:[[:space:]]*"\([^"]*Gameboy_'"$LATEST"'\.rbf\)".*/\1/p' "$TMP/releases.json" | head -1)
if [ -z "$URL" ]; then
    URL="https://raw.githubusercontent.com/Heber-co-uk/Gameboy_MiSTer_Cart/master/releases/$FILE"
fi

echo Latest core: $FILE
echo Downloading MMS2 GB core...
wget --no-check-certificate -O "$TMP/$FILE" "$URL" || exit 1
if [ ! -s "$TMP/$FILE" ]; then echo Downloaded core is empty.; exit 1; fi

echo Installing MMS2 GB core...
cp "$TMP/$FILE" "/media/fat/MMS2/$FILE" || exit 1
find /media/fat/MMS2 -maxdepth 1 -type f -name 'Gameboy_*.rbf' ! -name "$FILE" -delete 2>/dev/null || true

echo Installing launcher shortcut...
wget --no-check-certificate -O "/media/fat/Load GB-GBC Cartridge.mgl" "https://raw.githubusercontent.com/Anime0t4ku/mister-companion/main/assets/Load%20GB-GBC%20Cartridge.mgl" || exit 1
if [ ! -s "/media/fat/Load GB-GBC Cartridge.mgl" ]; then echo Launcher download is empty.; exit 1; fi

echo Installing cartridge config...
wget --no-check-certificate -O /media/fat/config/MMS2_GB_Cart.CFG "https://raw.githubusercontent.com/Anime0t4ku/mister-companion/main/assets/MMS2_GB_Cart.CFG" || exit 1
if [ ! -s /media/fat/config/MMS2_GB_Cart.CFG ]; then echo Config download is empty.; exit 1; fi

printf '%s\n' "$LATEST" > /media/fat/Scripts/.config/mms2_gb_core/version.txt
rm -rf "$TMP"
sync
echo MMS2 GB Core installed.
echo A MiSTer menu reload is recommended.
)SH";
}

void App::installOrUpdateMms2GbCore() {
    const bool success = showStreamingCommandWindow("MMS2 GB CORE", mms2GbCoreInstallShell(), "MMS2 GB Core installed.", "MMS2 GB Core install failed.");
    if (success) {
        extraUpdateAvailable[static_cast<int>(ExtraId::Mms2GbCore)] = false;
        if (confirm("RETURN TO MENU", "RELOAD THE MISTER MENU NOW?")) {
            sendSoftRebootCommand();
        }
    }
    refreshCurrentExtraStatus(false);
}

void App::uninstallMms2GbCore() {
    if (!confirm("UNINSTALL MMS2 GB CORE", "REMOVE THE MMS2 GB CORE, SHORTCUT AND CONFIG?")) return;
    const std::string command = R"SH(
echo Removing MMS2 GB Core files...
rm -f /media/fat/MMS2/Gameboy_*.rbf
rm -f "/media/fat/Load GB-GBC Cartridge.mgl"
rm -f /media/fat/config/MMS2_GB_Cart.CFG
rm -f /media/fat/Scripts/.config/mms2_gb_core/version.txt
sync
echo MMS2 GB Core uninstalled.
)SH";
    const bool success = showStreamingCommandWindow("UNINSTALL MMS2 GB CORE", command, "MMS2 GB Core uninstalled.", "MMS2 GB Core uninstall failed.");
    extraUpdateAvailable[static_cast<int>(ExtraId::Mms2GbCore)] = false;
    if (success && confirm("RETURN TO MENU", "RELOAD THE MISTER MENU NOW?")) {
        sendSoftRebootCommand();
    }
    refreshCurrentExtraStatus(false);
}

static std::string raSourcesHeredoc() {
    return R"RA(main|Main_MiSTer|odelot/Main_MiSTer|main_zip
nes|NES|odelot/NES_MiSTer|rbf
snes|SNES|odelot/SNES_MiSTer|rbf
gameboy|Gameboy|odelot/Gameboy_MiSTer|rbf
gba|GBA|odelot/GBA_MiSTer|rbf
n64|N64|odelot/N64_MiSTer|rbf
psx|PSX|odelot/PSX_MiSTer|rbf
megadrive|MegaDrive|odelot/MegaDrive_MiSTer|rbf
megacd|MegaCD|odelot/MegaCD_MiSTer|rbf
sms|SMS|odelot/SMS_MiSTer|rbf
neogeo|NeoGeo|odelot/NeoGeo_MiSTer|rbf
turbografx16|TurboGrafx16|odelot/TurboGrafx16_MiSTer|rbf
atari7800|Atari7800|odelot/Atari7800_MiSTer|rbf
s32x|S32X|odelot/S32X_MiSTer|rbf
)RA";
}

static std::string raCoresInstallShell(bool updateOnly) {
    std::string command;
    command += "TMP=/tmp/mc_ra_cores\nrm -rf \"$TMP\"\nmkdir -p \"$TMP\" /media/fat/_RA_Cores/Cores /media/fat/Scripts/.config/ra_cores\n";
    command += "cat > \"$TMP/sources.txt\" <<'MCRASOURCES'\n" + raSourcesHeredoc() + "MCRASOURCES\n";
    command += updateOnly ? "echo Checking for RetroAchievement Cores updates...\n" : "echo Installing RetroAchievement Cores...\n";
    command += R"SH(
INSTALLED=NO
if [ -f /media/fat/MiSTer_RA ] && [ -f /media/fat/achievement.wav ] && [ -d /media/fat/_RA_Cores/Cores ]; then INSTALLED=YES; fi
printf '{\n  "sources": {\n' > "$TMP/versions.json"
FIRST=1
while IFS='|' read -r key title repo kind; do
    [ -z "$key" ] && continue
    release_json="$TMP/${key}_release.json"
    wget --no-check-certificate --header='User-Agent: MiSTer-Companion-Vita' -O "$release_json" "https://api.github.com/repos/$repo/releases/latest" || exit 1
    latest=$(sed -n 's/.*"tag_name"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "$release_json" | head -1)
    if [ -z "$latest" ]; then echo "Unable to determine latest $title release."; exit 1; fi
    installed=
    if [ -f /media/fat/Scripts/.config/ra_cores/versions.json ]; then
        flat_versions=$(tr -d '\n\r\t ' < /media/fat/Scripts/.config/ra_cores/versions.json)
        installed=$(printf '%s' "$flat_versions" | sed -n 's/.*"'"$key"'":{"version":"\([^"]*\)".*/\1/p')
        [ -z "$installed" ] && installed=$(printf '%s' "$flat_versions" | sed -n 's/.*"'"$key"'":{[^}]*"version":"\([^"]*\)".*/\1/p')
        [ -z "$installed" ] && installed=$(printf '%s' "$flat_versions" | sed -n 's/.*"'"$key"'":"\([^"]*\)".*/\1/p')
    fi
    if [ "$INSTALLED" = "YES" ] && [ "$installed" = "$latest" ]; then
        echo "$title is already up to date, skipping download."
        if [ "$FIRST" = 0 ]; then printf ',\n' >> "$TMP/versions.json"; fi
        FIRST=0
        if [ "$key" = "main" ]; then
            printf '    "%s": {"asset": "", "files": ["/media/fat/MiSTer_RA", "/media/fat/achievement.wav", "/media/fat/retroachievements.cfg"], "version": "%s"}' "$key" "$latest" >> "$TMP/versions.json"
        else
            printf '    "%s": {"asset": "", "files": ["/media/fat/_RA_Cores/Cores/%s.rbf", "/media/fat/_RA_Cores/%s.mgl"], "version": "%s"}' "$key" "$title" "$title" "$latest" >> "$TMP/versions.json"
        fi
        continue
    fi
    echo "Downloading $title $latest..."
    URL=$(sed -n 's/.*"browser_download_url"[[:space:]]*:[[:space:]]*"\([^"]*\.zip\)".*/\1/p' "$release_json" | head -1)
    if [ -z "$URL" ]; then URL=$(sed -n 's/.*"browser_download_url"[[:space:]]*:[[:space:]]*"\([^"]*\.rbf\)".*/\1/p' "$release_json" | head -1); fi
    if [ -z "$URL" ]; then echo "Unable to find asset for $title."; exit 1; fi
    OUT="$TMP/$key.asset"
    wget --no-check-certificate -O "$OUT" "$URL" || exit 1
    if [ "$kind" = "main_zip" ]; then
        rm -rf "$TMP/main_extract" && mkdir -p "$TMP/main_extract"
        unzip -o "$OUT" -d "$TMP/main_extract" >/dev/null || exit 1
        MAIN=$(find "$TMP/main_extract" -name MiSTer | head -1)
        CFG=$(find "$TMP/main_extract" -name retroachievements.cfg | head -1)
        WAV=$(find "$TMP/main_extract" -name achievement.wav | head -1)
        [ -z "$MAIN" ] && echo "Main_MiSTer binary missing." && exit 1
        cp "$MAIN" /media/fat/MiSTer_RA || exit 1
        chmod +x /media/fat/MiSTer_RA
        [ -n "$WAV" ] && cp "$WAV" /media/fat/achievement.wav
        if [ -n "$CFG" ] && [ ! -f /media/fat/retroachievements.cfg ]; then cp "$CFG" /media/fat/retroachievements.cfg; fi
    else
        if echo "$URL" | grep -qi '\.zip$'; then
            rm -rf "$TMP/core_extract" && mkdir -p "$TMP/core_extract"
            unzip -o "$OUT" -d "$TMP/core_extract" >/dev/null || exit 1
            RBF=$(find "$TMP/core_extract" -name '*.rbf' | head -1)
        else
            RBF="$OUT"
        fi
        [ -z "$RBF" ] && echo "RBF missing for $title." && exit 1
        cp "$RBF" "/media/fat/_RA_Cores/Cores/$title.rbf" || exit 1
        cat > "/media/fat/_RA_Cores/$title.mgl" <<EOFRA
<mistergamedescription>
    <rbf>_RA_Cores/Cores/$title.rbf</rbf>
</mistergamedescription>
EOFRA
    fi
    if [ "$FIRST" = 0 ]; then printf ',\n' >> "$TMP/versions.json"; fi
    FIRST=0
    asset_name=$(basename "$URL")
    if [ "$kind" = "main_zip" ]; then
        printf '    "%s": {"asset": "%s", "files": ["/media/fat/MiSTer_RA", "/media/fat/achievement.wav", "/media/fat/retroachievements.cfg"], "version": "%s"}' "$key" "$asset_name" "$latest" >> "$TMP/versions.json"
    else
        printf '    "%s": {"asset": "%s", "files": ["/media/fat/_RA_Cores/Cores/%s.rbf", "/media/fat/_RA_Cores/%s.mgl"], "version": "%s"}' "$key" "$asset_name" "$title" "$title" "$latest" >> "$TMP/versions.json"
    fi
done < "$TMP/sources.txt"
printf '\n  }\n}\n' >> "$TMP/versions.json"
cp "$TMP/versions.json" /media/fat/Scripts/.config/ra_cores/versions.json
if [ ! -f /media/fat/MiSTer.ini ]; then
    if [ -f /media/fat/MiSTer_Example.ini ]; then cp /media/fat/MiSTer_Example.ini /media/fat/MiSTer.ini; else wget --no-check-certificate -O /media/fat/MiSTer.ini https://raw.githubusercontent.com/Anime0t4ku/mister-companion/main/assets/MiSTer_example.ini || true; fi
fi
if ! grep -q '^\[RA_\*\]' /media/fat/MiSTer.ini 2>/dev/null; then
    printf '\n[RA_*]\nmain=MiSTer_RA\n' >> /media/fat/MiSTer.ini
fi
rm -f /media/fat/MiSTer_RA.ini
rm -rf "/media/fat/_RA Cores"
rm -rf "$TMP"
sync
echo RetroAchievement Cores install/update complete.
)SH";
    return command;
}

void App::installOrUpdateRaCores(bool updateOnly) {
    showStreamingCommandWindow(updateOnly ? "UPDATE RA CORES" : "INSTALL RA CORES", raCoresInstallShell(updateOnly), updateOnly ? "RetroAchievement Cores updated." : "RetroAchievement Cores installed.", "RetroAchievement Cores operation failed.");
    extraUpdateAvailable[static_cast<int>(ExtraId::RetroAchievementCores)] = false;
    refreshCurrentExtraStatus(false);
}

void App::uninstallRaCores() {
    if (!confirm("UNINSTALL RA CORES", "ARE YOU SURE?")) return;
    const std::string command = R"SH(
echo Removing RetroAchievement Cores files...
rm -f /media/fat/MiSTer_RA /media/fat/MiSTer_RA.ini /media/fat/achievement.wav /media/fat/Scripts/.config/ra_cores/versions.json
rm -rf /media/fat/_RA_Cores "/media/fat/_RA Cores"
if [ -f /media/fat/MiSTer.ini ]; then
    awk 'BEGIN{skip=0} /^\[RA_\*\]/{skip=1; next} /^\[/{skip=0} skip==0{print}' /media/fat/MiSTer.ini > /tmp/mc_mister_ini_ra && mv /tmp/mc_mister_ini_ra /media/fat/MiSTer.ini
fi
sync
echo RetroAchievement Cores uninstalled.
echo Kept /media/fat/retroachievements.cfg
)SH";
    showStreamingCommandWindow("UNINSTALL RA CORES", command, "RetroAchievement Cores uninstalled.", "RetroAchievement Cores uninstall failed.");
    extraUpdateAvailable[static_cast<int>(ExtraId::RetroAchievementCores)] = false;
    refreshCurrentExtraStatus(false);
}

static std::string raDefaultConfig() {
    return "username=odelot\npassword=\n\nshow_challenge_show_popup=1\nshow_challenge_hide_popup=0\nshow_progress_popups=1\nshow_progress_name=1\nleaderboards-enabled=1\ndebug=0\nhardcore=0\n";
}

void App::configureRaCores() {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }

    std::string current = remoteReadTextFile(RaConfigPath);
    if (current.empty()) current = raDefaultConfig();

    auto valueFor = [&](const std::string& key, const std::string& fallback) {
        for (const std::string& line : splitLines(current)) {
            if (line.rfind(key + "=", 0) == 0) return trim(line.substr(key.size() + 1));
        }
        return fallback;
    };

    std::vector<std::string> keys = {
        "username", "password", "show_challenge_show_popup", "show_challenge_hide_popup",
        "show_progress_popups", "show_progress_name", "leaderboards-enabled", "debug", "hardcore"
    };
    std::vector<std::string> labels = {
        "Username", "Password", "Challenge Popup", "Missed Challenge Popup",
        "Progress Popups", "Progress Name", "Leaderboards", "Debug", "Hardcore"
    };
    std::vector<std::string> values;
    for (const std::string& key : keys) values.push_back(valueFor(key, key == "username" ? "odelot" : (key == "password" ? "" : "1")));
    if (values[3].empty()) values[3] = "0";
    if (values[7].empty()) values[7] = "0";
    if (values[8].empty()) values[8] = "0";

    int cursor = 0;
    int scroll = 0;
    std::string screenMessage;
    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 held = padGetButtons(&pad);
        if ((held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X)) == 0) break;
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.drawCard(260, 240, 760, 180, "RA CORES CONFIG");
        ui.drawText(320, 330, "PREPARING CONFIGURATION SCREEN", UiRenderer::rgb(248, 245, 255), 2);
        ui.endFrame();
    }

    while (appletMainLoop()) {
        ui.beginFrame();
        drawHeader();
        ui.drawCard(40, 176, 1200, 424, "RETROACHIEVEMENT CORES CONFIG");
        int visible = 6;
        if (cursor < scroll) scroll = cursor;
        if (cursor >= scroll + visible) scroll = cursor - visible + 1;
        int y = 240;
        for (int i = scroll; i < static_cast<int>(keys.size()) && i < scroll + visible; i++) {
            std::string display = values[i].empty() ? "Not set" : values[i];
            if (keys[i] == "password" && !values[i].empty()) display = "********";
            if (i >= 2) display = values[i] == "1" ? "Yes" : "No";
            ui.drawButton(76, y, 1128, 48, labels[i] + "  " + display, cursor == i, false, false);
            y += 56;
        }
        ui.drawTextCentered(76, 572, 1128, std::to_string(cursor + 1) + " / " + std::to_string(keys.size()), UiRenderer::rgb(174, 154, 218), 2);
        ui.drawMessage(screenMessage.empty() ? "× Edit/Toggle    □ Save & Back    ○ Cancel" : screenMessage);
        ui.drawFooter("UP/DN SELECT    × EDIT/TOGGLE    □ SAVE    ○ CANCEL");
        ui.endFrame();

        padUpdate(&pad);
        u64 buttons = padGetButtonsDown(&pad);
        if (buttons & HidNpadButton_Up) cursor = (cursor + static_cast<int>(keys.size()) - 1) % static_cast<int>(keys.size());
        if (buttons & HidNpadButton_Down) cursor = (cursor + 1) % static_cast<int>(keys.size());
        if (buttons & HidNpadButton_B) {
            lastMessage = "RA Cores config unchanged.";
            return;
        }
        if (buttons & HidNpadButton_X) {
            std::string text = current;
            if (text.empty()) text = raDefaultConfig();
            std::set<std::string> seen;
            std::vector<std::string> out;
            for (const std::string& line : splitLines(text)) {
                if (line.find('=') == std::string::npos) { out.push_back(line); continue; }
                std::string k = trim(line.substr(0, line.find('=')));
                auto it = std::find(keys.begin(), keys.end(), k);
                if (it == keys.end()) { out.push_back(line); continue; }
                int idx = static_cast<int>(it - keys.begin());
                out.push_back(k + "=" + values[idx]);
                seen.insert(k);
            }
            for (int i = 0; i < static_cast<int>(keys.size()); i++) {
                if (!seen.count(keys[i])) out.push_back(keys[i] + "=" + values[i]);
            }
            std::string finalText;
            for (const std::string& line : out) finalText += line + "\n";
            std::string error;
            if (remoteWriteTextFile(RaConfigPath, finalText, error)) {
                lastMessage = "RA Cores config saved.";
                return;
            }
            screenMessage = error.empty() ? "Unable to save config." : error;
        }
        if (buttons & HidNpadButton_A) {
            if (cursor < 2) {
                editText(labels[cursor].c_str(), values[cursor], cursor == 1);
            } else {
                values[cursor] = values[cursor] == "1" ? "0" : "1";
            }
            screenMessage.clear();
            while (appletMainLoop()) {
                padUpdate(&pad);
                if ((padGetButtons(&pad) & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X)) == 0) break;
            }
        }
    }
}

void App::executeExtraAction(ExtraId id, int actionIndex) {
    std::vector<std::string> actions = extraActions(id);
    if (actionIndex < 0 || actionIndex >= static_cast<int>(actions.size())) return;
    const std::string action = actions[actionIndex];

    if (id == ExtraId::ZaparooFrontend) {
        if (action == "INSTALL" || action == "UPDATE") installOrUpdateZaparooFrontend();
        else if (action == "CHECK FOR UPDATES") refreshCurrentExtraStatus(true);
        else if (action == "UNINSTALL") uninstallZaparooFrontend();
        return;
    }

    if (id == ExtraId::Mms2GbCore) {
        if (action == "INSTALL" || action == "UPDATE") installOrUpdateMms2GbCore();
        else if (action == "CHECK FOR UPDATES") refreshCurrentExtraStatus(true);
        else if (action == "UNINSTALL") uninstallMms2GbCore();
        return;
    }

    if (action == "INSTALL") installOrUpdateRaCores(false);
    else if (action == "UPDATE") installOrUpdateRaCores(true);
    else if (action == "CHECK FOR UPDATES") refreshCurrentExtraStatus(true);
    else if (action == "CONFIGURE") configureRaCores();
    else if (action == "UNINSTALL") uninstallRaCores();
}



void App::loadSettingsTab(bool force) {
    if (!ssh.isConnected()) {
        settingsLoaded = false;
        lastMessage = "Connect to a MiSTer first.";
        return;
    }
    if (settingsLoaded && !force) return;
    if (!ensureRemoteMisterIni()) return;
    scanSettingsIniFiles();
    scanSettingsFonts();
    if (settingsIniFiles.empty()) {
        settingsLoaded = false;
        lastMessage = "No MiSTer.ini files found.";
        return;
    }
    if (std::find(settingsIniFiles.begin(), settingsIniFiles.end(), selectedSettingsIni) == settingsIniFiles.end()) {
        selectedSettingsIni = settingsIniFiles.front();
    }
    loadSelectedSettingsIni();
}

bool App::ensureRemoteMisterIni() {
    std::string command =
        "cd /media/fat 2>/dev/null || exit 1; "
        "if [ ! -f MiSTer.ini ]; then "
        "if [ -f MiSTer_Example.ini ]; then cp MiSTer_Example.ini MiSTer.ini; "
        "else wget --no-check-certificate -O MiSTer.ini " + shellQuote(DefaultMisterIniUrl) + "; fi; fi; "
        "test -s MiSTer.ini";
    SshResult result = ssh.runCommand(command);
    if (!result.success) {
        lastMessage = result.error.empty() ? "Unable to create MiSTer.ini." : result.error;
        return false;
    }
    return true;
}

void App::scanSettingsIniFiles() {
    settingsIniFiles.clear();
    SshResult result = ssh.runCommand(
        "cd /media/fat 2>/dev/null || exit 0; "
        "for f in MiSTer.ini MiSTer_*.ini; do [ -f \"$f\" ] && echo \"$f\"; done"
    );
    if (result.success) settingsIniFiles = MisterIni::sortIniFiles(splitLines(result.output));
}

void App::scanSettingsFonts() {
    settingsFonts.clear();
    settingsFonts.push_back("Default");
    SshResult result = ssh.runCommand(
        "cd /media/fat/font 2>/dev/null || exit 0; "
        "for f in *.pf; do [ -f \"$f\" ] && echo \"$f\"; done"
    );
    if (result.success) {
        for (const std::string& line : splitLines(result.output)) {
            std::string font = trim(line);
            if (!font.empty() && std::find(settingsFonts.begin(), settingsFonts.end(), font) == settingsFonts.end()) settingsFonts.push_back(font);
        }
    }
}

std::string App::remoteReadTextFile(const std::string& path) {
    SshResult result = ssh.runCommand("cat " + shellQuote(path));
    if (!result.success) return "";
    return result.output;
}

bool App::remoteWriteTextFile(const std::string& path, const std::string& text, std::string& error) {
    std::string normalized = text;
    if (normalized.empty() || normalized.back() != '\n') normalized += "\n";
    std::string marker = "MCNX_EOF_MISTER_INI";
    while (normalized.find(marker) != std::string::npos) marker += "_X";
    std::string command = "cat > " + shellQuote(path) + " <<'" + marker + "'\n" + normalized + marker + "\n";
    SshResult result = ssh.runCommand(command);
    if (!result.success) {
        error = result.error.empty() ? "Unable to write INI file." : result.error;
        return false;
    }
    return true;
}

void App::loadSelectedSettingsIni() {
    std::string filename = MisterIni::normalizeIniFilename(selectedSettingsIni);
    if (filename.empty()) filename = "MiSTer.ini";
    selectedSettingsIni = filename;
    settingsIniText = remoteReadTextFile("/media/fat/" + filename);
    if (settingsIniText.empty()) {
        settingsLoaded = false;
        lastMessage = "Unable to read " + filename + ".";
        return;
    }
    settingsEasy = MisterIni::easyValuesFromIniText(settingsIniText);
    if (std::find(settingsFonts.begin(), settingsFonts.end(), settingsEasy.font) == settingsFonts.end()) settingsFonts.push_back(settingsEasy.font);
    settingsLoaded = true;
    settingsDirty = false;
    lastMessage = filename + " loaded.";
}

void App::saveSettingsIni() {
    if (!settingsLoaded) return;
    std::string newText = MisterIni::updateIniText(settingsIniText, settingsEasy);
    std::string error;
    if (!remoteWriteTextFile("/media/fat/" + selectedSettingsIni, newText, error)) {
        lastMessage = error;
        return;
    }
    settingsIniText = newText;
    settingsDirty = false;
    lastMessage = selectedSettingsIni + " saved.";
}

void App::restoreSettingsDefaults() {
    std::string target = selectedSettingsIni.empty() ? "MiSTer.ini" : selectedSettingsIni;
    if (!confirm("Restore Defaults", "Replace the selected INI file with the default MiSTer configuration?")) return;
    std::string command =
        "cd /media/fat 2>/dev/null || exit 1; "
        "wget --no-check-certificate -O " + shellQuote(target) + " " + shellQuote(DefaultMisterIniUrl) + " && test -s " + shellQuote(target);
    showStreamingCommandWindow("Restore Defaults", command, "Defaults restored.", "Restore defaults failed.");
    scanSettingsIniFiles();
    scanSettingsFonts();
    selectedSettingsIni = target;
    loadSelectedSettingsIni();
}


void App::drawWallpapers() {
    ui.drawCard(40, 176, 580, 400, "WALLPAPER STATUS");
    ui.drawCard(660, 176, 580, 400, "WALLPAPER ACTIONS");

    const std::string sourceTitle = wallpaperSourceTitle(selectedWallpaperSource);

    if (gWallpaperStatusChecking && gWallpaperStatusSource == selectedWallpaperSource) {
        if (gWallpaperPollDelay > 0) {
            gWallpaperPollDelay--;
        } else {
            gWallpaperPollDelay = 30;
            SshResult doneResult = ssh.runCommand("if [ -f " + shellQuote(gWallpaperStatusToken + ".done") + " ]; then cat " + shellQuote(gWallpaperStatusToken + ".done") + "; else echo WAIT; fi");
            std::string state = doneResult.success ? trim(doneResult.output) : "WAIT";
            if (state == "OK" || state == "FAIL") {
                std::vector<int> totals;
                std::vector<int> installedCounts;
                std::vector<int> missingCounts;
                std::vector<std::string> statusLines;

                if (state == "OK") {
                    cachedWallpaperStatus = {"STATUS: CHECKING...", "STEP: PARSING DATABASE"};
                    SshResult dbResult = ssh.runCommand("cat " + shellQuote(gWallpaperStatusToken + ".b64"));
                    std::vector<unsigned char> zipData = dbResult.success ? decodeBase64Text(dbResult.output) : std::vector<unsigned char>();

                    auto checkPack = [&](const std::string& filterMode) {
                        std::vector<WallpaperDb::Entry> entries;
                        std::string error;
                        if (!wallpaperEntriesFromZipBytes(selectedWallpaperSource, filterMode, zipData, entries, error)) {
                            statusLines.push_back("STATUS: CHECK FAILED");
                            statusLines.push_back(ellipsizeText(error, 34));
                            totals.push_back(0);
                            installedCounts.push_back(0);
                            missingCounts.push_back(0);
                            return;
                        }
                        int total = static_cast<int>(entries.size());
                        int installedCount = countInstalledWallpaperEntries(entries, gWallpaperInstalledNames);
                        totals.push_back(total);
                        installedCounts.push_back(installedCount);
                        missingCounts.push_back(total - installedCount);
                    };

                    if (selectedWallpaperSource == 0) {
                        checkPack("169");
                        if (statusLines.empty()) checkPack("43");
                    } else {
                        checkPack("all");
                    }

                    int allTotal = 0;
                    int allInstalled = 0;
                    int allMissing = 0;
                    for (size_t i = 0; i < totals.size(); ++i) {
                        allTotal += totals[i];
                        allInstalled += installedCounts[i];
                        allMissing += missingCounts[i];
                    }

                    if (statusLines.empty()) {
                        if (allTotal <= 0) {
                            statusLines.push_back("STATUS: CHECK FAILED");
                            statusLines.push_back("WALLPAPERS INSTALLED: 0");
                        } else {
                            statusLines.push_back("STATUS: READY");
                            statusLines.push_back("WALLPAPERS INSTALLED: " + std::to_string(allInstalled) + " / " + std::to_string(allTotal));
                            if (allInstalled > 0 && allMissing > 0) statusLines.push_back("UPDATE AVAILABLE: YES");
                            else if (allInstalled > 0) statusLines.push_back("UPDATE AVAILABLE: NO");
                        }
                    }
                } else {
                    statusLines.push_back("STATUS: CHECK FAILED");
                    statusLines.push_back("Unable to download database.");
                    totals.push_back(0);
                    installedCounts.push_back(0);
                    missingCounts.push_back(0);
                }

                ssh.runCommand("rm -f " + shellQuote(gWallpaperStatusToken + ".b64") + " " + shellQuote(gWallpaperStatusToken + ".done"));

                statusLines.push_back("STATIC ACTIVE: " + gWallpaperStaticActive);
                statusLines.push_back("STATIC: " + gWallpaperStaticName);

                gWallpaperStatusChecking = false;
                gWallpaperPackTotal = totals;
                gWallpaperPackInstalled = installedCounts;
                gWallpaperPackMissing = missingCounts;
                gWallpaperInstalledTotal = 0;
                gWallpaperMissingTotal = 0;
                for (size_t i = 0; i < gWallpaperPackInstalled.size(); ++i) {
                    gWallpaperInstalledTotal += gWallpaperPackInstalled[i];
                    if (i < gWallpaperPackMissing.size()) gWallpaperMissingTotal += gWallpaperPackMissing[i];
                }
                cachedWallpaperStatus = statusLines;
            }
        }
    }

    ui.drawText(76, 236, "SOURCE", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 236, sourceTitle, UiRenderer::rgb(248, 245, 255), 3);
    ui.drawText(76, 290, "SUB TAB", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(220, 290, std::to_string(selectedWallpaperSource + 1) + " / " + std::to_string(WallpaperSourceCount), UiRenderer::rgb(248, 245, 255), 2);

    int y = 346;
    std::vector<std::string> statusLines = cachedWallpaperStatus.empty()
        ? std::vector<std::string>{ssh.isConnected() ? "STATUS: NOT CHECKED" : "STATUS: DISCONNECTED"}
        : cachedWallpaperStatus;
    for (const std::string& line : statusLines) {
        if (y > 530) break;
        ui.drawText(76, y, ellipsizeText(line, 42), UiRenderer::rgb(218, 208, 238), 2);
        y += 42;
    }

    const bool actionsEnabled = ssh.isConnected();
    std::vector<std::string> actions = wallpaperActions(selectedWallpaperSource);
    if (!actions.empty() && selectedWallpaperAction >= static_cast<int>(actions.size())) selectedWallpaperAction = static_cast<int>(actions.size()) - 1;
    int actionY = 232;
    for (int i = 0; i < static_cast<int>(actions.size()); i++) {
        if (actionY > 520) break;
        const bool danger = actions[i].find("REMOVE") != std::string::npos;
        ui.drawButton(696, actionY, 508, 54, actions[i], selectedWallpaperAction == i && actionsEnabled, danger, !actionsEnabled);
        actionY += 68;
    }

    ui.drawText(696, 538, "PRESS SELECT FOR STATIC WALLPAPER MENU", UiRenderer::rgb(174, 154, 218), 2);
}

void App::drawPassthrough() {
    ui.clear(UiRenderer::rgb(12, 10, 20));
    ui.fillRect(0, 0, UiRenderer::Width, UiRenderer::Height, UiRenderer::rgb(12, 10, 20));
    ui.fillRect(0, 0, UiRenderer::Width, 96, UiRenderer::rgb(20, 16, 34));
    ui.fillRect(0, 94, UiRenderer::Width, 4, UiRenderer::rgb(143, 84, 255));

    ui.drawText(42, 32, "REMOTE PASSTHROUGH ACTIVE", UiRenderer::rgb(248, 245, 255), 3);
    ui.drawStatusPill(UiRenderer::Width - 280, 28, remote.isConnected() ? "REMOTE READY" : "REMOTE LOST", remote.isConnected());

    ui.drawCard(220, 180, 840, 340, "VITA CONTROLS ARE FORWARDED TO MISTER");
    ui.drawText(280, 285, "USE THE VITA PHYSICAL BUTTONS TO CONTROL MISTER", UiRenderer::rgb(248, 245, 255), 2);
    ui.drawText(280, 345, "HOLD SELECT AND PRESS L", UiRenderer::rgb(174, 154, 218), 2);
    ui.drawText(280, 390, "SELECT + L EXITS PASSTHROUGH MODE", UiRenderer::rgb(248, 245, 255), 3);

    ui.drawFooter("PASSTHROUGH    SEL+L EXIT    RELEASE ALL ON EXIT");
}

void App::handleInput(u64 buttons) {
    if (buttons & HidNpadButton_L) {
        if (tab == Tab::Connection) tab = Tab::Extras;
        else if (tab == Tab::Device) tab = Tab::Connection;
        else if (tab == Tab::Remote) tab = Tab::Device;
        else if (tab == Tab::Scripts) tab = Tab::Remote;
        else if (tab == Tab::Settings) tab = Tab::Scripts;
        else if (tab == Tab::Wallpapers) tab = Tab::Settings;
        else tab = Tab::Wallpapers;
        selected = 0;
        if (tab == Tab::Scripts) refreshCurrentScriptStatus();
        if (tab == Tab::Settings) loadSettingsTab();
        if (tab == Tab::Wallpapers) refreshWallpaperStatus();
        if (tab == Tab::Extras) refreshCurrentExtraStatus(false);
        return;
    }
    if (buttons & HidNpadButton_R) {
        if (tab == Tab::Connection) tab = Tab::Device;
        else if (tab == Tab::Device) tab = Tab::Remote;
        else if (tab == Tab::Remote) tab = Tab::Scripts;
        else if (tab == Tab::Scripts) tab = Tab::Settings;
        else if (tab == Tab::Settings) tab = Tab::Wallpapers;
        else if (tab == Tab::Wallpapers) tab = Tab::Extras;
        else tab = Tab::Connection;
        selected = 0;
        if (tab == Tab::Remote && ssh.isConnected() && remoteInstalled == "Not checked") refreshRemoteStatus();
        if (tab == Tab::Scripts) refreshCurrentScriptStatus();
        if (tab == Tab::Settings) loadSettingsTab();
        if (tab == Tab::Wallpapers) refreshWallpaperStatus();
        if (tab == Tab::Extras) refreshCurrentExtraStatus(false);
        return;
    }

    if (tab == Tab::Connection) handleConnectionInput(buttons);
    else if (tab == Tab::Device) handleDeviceInput(buttons);
    else if (tab == Tab::Remote) handleRemoteInput(buttons);
    else if (tab == Tab::Scripts) handleScriptsInput(buttons);
    else if (tab == Tab::Settings) handleSettingsInput(buttons);
    else if (tab == Tab::Wallpapers) handleWallpapersInput(buttons);
    else handleExtrasInput(buttons);
}

void App::handleConnectionInput(u64 buttons) {
    if (ssh.isConnected()) {
        connectionProfileMode = false;
        selected = 3;
        if (buttons & HidNpadButton_A) connectOrDisconnect();
        return;
    }

    if (buttons & (HidNpadButton_Left | HidNpadButton_Right)) {
        connectionProfileMode = !connectionProfileMode;
        selected = 0;
        selectedProfile = 0;
        profileScroll = 0;
        lastMessage = connectionProfileMode ? "Profile mode." : "Manual connect mode.";
        return;
    }

    if (connectionProfileMode) {
        const int count = static_cast<int>(config.profiles.size());
        if (count <= 0) {
            if (buttons & HidNpadButton_B) connectionProfileMode = false;
            return;
        }

        if (buttons & HidNpadButton_Up) selectedProfile = (selectedProfile + count - 1) % count;
        if (buttons & HidNpadButton_Down) selectedProfile = (selectedProfile + 1) % count;

        if (buttons & HidNpadButton_A) {
            connectProfile(selectedProfile);
            return;
        }
        if (buttons & HidNpadButton_X) {
            editProfile(selectedProfile);
            return;
        }
        if (buttons & HidNpadButton_Minus) {
            deleteProfile(selectedProfile);
            return;
        }
        return;
    }

    if (buttons & HidNpadButton_Up) selected = (selected + 3) % 4;
    if (buttons & HidNpadButton_Down) selected = (selected + 1) % 4;

    if (buttons & HidNpadButton_Plus) {
        saveManualAsProfile();
        return;
    }
    if (buttons & HidNpadButton_Minus) {
        showMisterScanWindow();
        return;
    }
    if (!(buttons & HidNpadButton_A)) return;

    switch (selected) {
        case 0:
            if (ssh.isConnected()) { lastMessage = "Disconnect before editing host."; break; }
            {
                std::string before = config.host;
                editText("MiSTer IP or hostname", config.host);
                config.host = trim(config.host);
                if (config.host != before) {
                    clearActiveProfile();
                    if (loadProfileForHost(config.host)) lastMessage = "Profile loaded for this IP.";
                }
                saveConfig();
            }
            break;
        case 1:
            if (ssh.isConnected()) { lastMessage = "Disconnect before editing username."; break; }
            { std::string before = config.username; editText("SSH username", config.username); if (config.username != before) clearActiveProfile(); saveConfig(); }
            break;
        case 2:
            if (ssh.isConnected()) { lastMessage = "Disconnect before editing password."; break; }
            { std::string before = config.password; editText("SSH password", config.password, true); if (config.password != before) clearActiveProfile(); saveConfig(); }
            break;
        case 3: connectOrDisconnect(); break;
    }
}

void App::handleDeviceInput(u64 buttons) {
    if (buttons & HidNpadButton_Up) selected = (selected + 3) % 4;
    if (buttons & HidNpadButton_Down) selected = (selected + 1) % 4;
    if (!(buttons & HidNpadButton_A)) return;

    if (!ssh.isConnected()) {
        lastMessage = "Connect to a MiSTer first.";
        return;
    }

    switch (selected) {
        case 0: refreshDevice(); break;
        case 1: toggleSmb(); break;
        case 2: returnToMenu(); break;
        case 3: reboot(); break;
    }
}

void App::handleRemoteInput(u64 buttons) {
    const bool installed = remoteInstalled == "Yes";
    const bool running = remoteRunning == "Yes";
    const int actionCount = installed ? 4 : 2;

    if (buttons & HidNpadButton_Up) selected = (selected + actionCount - 1) % actionCount;
    if (buttons & HidNpadButton_Down) selected = (selected + 1) % actionCount;
    if (!(buttons & HidNpadButton_A)) return;

    if (!ssh.isConnected()) {
        lastMessage = "Connect to a MiSTer first.";
        return;
    }

    if (!installed) {
        switch (selected) {
            case 0: installRemoteDaemon(); break;
            case 1: refreshRemoteStatus(); break;
        }
        return;
    }

    if (running) {
        switch (selected) {
            case 0: startPassthrough(); break;
            case 1: stopRemoteDaemon(); break;
            case 2: toggleRemoteStartup(); break;
            case 3: refreshRemoteStatus(); break;
        }
        return;
    }

    switch (selected) {
        case 0: startRemoteDaemon(); break;
        case 1: toggleRemoteStartup(); break;
        case 2: uninstallRemoteDaemon(); break;
        case 3: refreshRemoteStatus(); break;
    }
}


void App::handleScriptsInput(u64 buttons) {
    if (buttons & HidNpadButton_Left) {
        selectedScript = (selectedScript + ScriptCount - 1) % ScriptCount;
        selected = 0;
        refreshCurrentScriptStatus();
        return;
    }
    if (buttons & HidNpadButton_Right) {
        selectedScript = (selectedScript + 1) % ScriptCount;
        selected = 0;
        refreshCurrentScriptStatus();
        return;
    }

    std::vector<std::string> actions = scriptActions(static_cast<ScriptId>(selectedScript));
    const int actionCount = static_cast<int>(actions.size());
    if (actionCount <= 0) return;

    if (buttons & HidNpadButton_Up) selected = (selected + actionCount - 1) % actionCount;
    if (buttons & HidNpadButton_Down) selected = (selected + 1) % actionCount;
    if (!(buttons & HidNpadButton_A)) return;

    if (!ssh.isConnected()) {
        lastMessage = "Connect to a MiSTer first.";
        return;
    }

    executeScriptAction(static_cast<ScriptId>(selectedScript), selected);
    refreshCurrentScriptStatus();
}

void App::handlePassthroughInput(u64 down, u64 up, u64 held) {
    if ((held & HidNpadButton_Minus) && (held & HidNpadButton_L)) {
        stopPassthrough();
        return;
    }

    sendPassthroughButton(HidNpadButton_Up, down, "dpad", "up", "down");
    sendPassthroughButton(HidNpadButton_Down, down, "dpad", "down", "down");
    sendPassthroughButton(HidNpadButton_Left, down, "dpad", "left", "down");
    sendPassthroughButton(HidNpadButton_Right, down, "dpad", "right", "down");

    sendPassthroughButton(HidNpadButton_Up, up, "dpad", "up", "up");
    sendPassthroughButton(HidNpadButton_Down, up, "dpad", "down", "up");
    sendPassthroughButton(HidNpadButton_Left, up, "dpad", "left", "up");
    sendPassthroughButton(HidNpadButton_Right, up, "dpad", "right", "up");

    sendPassthroughButton(HidNpadButton_A, down, "button", "b", "down");
    sendPassthroughButton(HidNpadButton_B, down, "button", "a", "down");
    sendPassthroughButton(HidNpadButton_X, down, "button", "y", "down");
    sendPassthroughButton(HidNpadButton_Y, down, "button", "x", "down");
    sendPassthroughButton(HidNpadButton_L, down, "button", "l", "down");
    sendPassthroughButton(HidNpadButton_R, down, "button", "r", "down");
    sendPassthroughButton(HidNpadButton_Minus, down, "button", "select", "down");
    sendPassthroughButton(HidNpadButton_Plus, down, "button", "start", "down");

    sendPassthroughButton(HidNpadButton_A, up, "button", "b", "up");
    sendPassthroughButton(HidNpadButton_B, up, "button", "a", "up");
    sendPassthroughButton(HidNpadButton_X, up, "button", "y", "up");
    sendPassthroughButton(HidNpadButton_Y, up, "button", "x", "up");
    sendPassthroughButton(HidNpadButton_L, up, "button", "l", "up");
    sendPassthroughButton(HidNpadButton_R, up, "button", "r", "up");
    sendPassthroughButton(HidNpadButton_Minus, up, "button", "select", "up");
    sendPassthroughButton(HidNpadButton_Plus, up, "button", "start", "up");
}

void App::sendPassthroughButton(u64 mask, u64 buttons, const std::string& control, const std::string& name, const std::string& action) {
    if (!passthroughActive) return;
    if (!(buttons & mask)) return;
    std::string message;
    if (!remote.sendController(control, name, action, message)) {
        passthroughActive = false;
        lastMessage = message;
    }
}

void App::editText(const char* title, std::string& value, bool password) {
    struct Key {
        std::string label;
        char value;
        int width;
    };

    auto makeRow = [](const std::string& chars) {
        std::vector<Key> row;
        for (char c : chars) row.push_back({std::string(1, c), c, 54});
        return row;
    };

    auto makeRows = [&](bool shifted) {
        std::vector<std::vector<Key>> rows;
        rows.push_back(makeRow("1234567890"));
        rows.push_back(makeRow(shifted ? "QWERTYUIOP" : "qwertyuiop"));
        rows.push_back(makeRow(shifted ? "ASDFGHJKL." : "asdfghjkl."));
        rows.push_back(makeRow(shifted ? "ZXCVBNM-_" : "zxcvbnm-_"));
        rows.push_back({
            {"@", '@', 54},
            {":" , ':', 54},
            {"/", '/', 54},
            {".", '.', 54},
            {"SPACE", ' ', 190}
        });
        return rows;
    };

    auto rowWidth = [](const std::vector<Key>& row) {
        int width = 0;
        for (const Key& key : row) width += key.width + 8;
        return row.empty() ? 0 : width - 8;
    };

    auto visibleText = [](const std::string& text, size_t maxChars) {
        if (text.empty()) return std::string(" ");
        if (text.size() <= maxChars) return text;
        if (maxChars <= 3) return text.substr(text.size() - maxChars);
        return "..." + text.substr(text.size() - (maxChars - 3));
    };

    std::string pending = value;
    bool shifted = false;
    int row = 0;
    int col = 0;
    bool inputReleased = false;

    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        std::vector<std::vector<Key>> rows = makeRows(shifted);
        if (row < 0) row = 0;
        if (row >= static_cast<int>(rows.size())) row = static_cast<int>(rows.size()) - 1;
        if (col < 0) col = 0;
        if (col >= static_cast<int>(rows[row].size())) col = static_cast<int>(rows[row].size()) - 1;

        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, UiRenderer::Height, UiRenderer::rgb(12, 10, 20));
        ui.drawCard(80, 56, 1120, 592, title ? title : "TEXT INPUT");

        ui.drawText(128, 124, password ? "PASSWORD FIELD" : "TEXT FIELD", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawText(128, 158, "CURRENT TEXT", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawRect(128, 188, 1024, 66, UiRenderer::rgb(74, 58, 112), 2);
        ui.drawText(150, 213, visibleText(pending, 72), UiRenderer::rgb(248, 245, 255), 2);
        ui.drawText(128, 274, shifted ? "CASE: UPPER" : "CASE: LOWER", UiRenderer::rgb(218, 208, 238), 2);

        int y = 306;
        for (int r = 0; r < static_cast<int>(rows.size()); ++r) {
            const auto& keys = rows[r];
            int x = (UiRenderer::Width - rowWidth(keys)) / 2;
            for (int c = 0; c < static_cast<int>(keys.size()); ++c) {
                const bool active = r == row && c == col;
                ui.drawButton(x, y, keys[c].width, 40, keys[c].label, active);
                x += keys[c].width + 8;
            }
            y += 50;
        }

        ui.drawFooter("DPAD MOVE    × TYPE    □ DEL    △ SAVE    ○ CANCEL    L/R CASE");
        ui.endFrame();

        padUpdate(&pad);
        const u64 held = padGetButtons(&pad);
        const u64 buttons = padGetButtonsDown(&pad);

        if (!inputReleased) {
            const u64 block = HidNpadButton_A | HidNpadButton_B | HidNpadButton_X | HidNpadButton_Y |
                              HidNpadButton_L | HidNpadButton_R |
                              HidNpadButton_Up | HidNpadButton_Down | HidNpadButton_Left | HidNpadButton_Right;
            if ((held & block) == 0) inputReleased = true;
            continue;
        }

        if (buttons & HidNpadButton_Up) row = (row + static_cast<int>(rows.size()) - 1) % static_cast<int>(rows.size());
        if (buttons & HidNpadButton_Down) row = (row + 1) % static_cast<int>(rows.size());
        if (buttons & HidNpadButton_Left) col = (col + static_cast<int>(rows[row].size()) - 1) % static_cast<int>(rows[row].size());
        if (buttons & HidNpadButton_Right) col = (col + 1) % static_cast<int>(rows[row].size());
        if (buttons & (HidNpadButton_L | HidNpadButton_R)) shifted = !shifted;

        if (col >= static_cast<int>(rows[row].size())) col = static_cast<int>(rows[row].size()) - 1;

        if ((buttons & HidNpadButton_A) && pending.size() < 255) pending.push_back(rows[row][col].value);
        if ((buttons & HidNpadButton_X) && !pending.empty()) pending.pop_back();
        if (buttons & HidNpadButton_Y) {
            value = pending;
            lastMessage = "Value updated.";
            return;
        }
        if (buttons & HidNpadButton_B) return;
    }
}

bool App::confirm(const char* title, const char* body) {
    int choice = 1;
    bool inputReleased = false;

    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, UiRenderer::Height, UiRenderer::rgb(12, 10, 20));
        ui.drawCard(300, 190, 680, 300, title);
        ui.drawText(350, 280, body, UiRenderer::rgb(248, 245, 255), 2);
        ui.drawButton(350, 370, 260, 58, "YES", choice == 0, true);
        ui.drawButton(670, 370, 260, 58, "NO", choice == 1);
        ui.drawFooter("LEFT/RIGHT SELECT    × CONFIRM    ○ CANCEL");
        ui.endFrame();

        padUpdate(&pad);
        const u64 held = padGetButtons(&pad);
        const u64 buttons = padGetButtonsDown(&pad);

        if (!inputReleased) {
            const u64 confirmButtons = HidNpadButton_A | HidNpadButton_B | HidNpadButton_Left | HidNpadButton_Right;
            if ((held & confirmButtons) == 0) inputReleased = true;
            continue;
        }

        if (buttons & HidNpadButton_Left) choice = 0;
        if (buttons & HidNpadButton_Right) choice = 1;
        if (buttons & HidNpadButton_A) return choice == 0;
        if (buttons & HidNpadButton_B) return false;
    }

    return false;
}

void App::saveConfig() {
    std::string error;
    if (ConfigStore::save(config, error)) lastMessage = "Profile saved to SD card.";
    else lastMessage = error;
}

bool App::hasCompleteManualConnection() const {
    return !trim(config.host).empty() && !trim(config.username).empty() && !trim(config.password).empty();
}

int App::profileIndexForHost(const std::string& host) const {
    const std::string wanted = toLower(trim(host));
    if (wanted.empty()) return -1;
    for (int i = 0; i < static_cast<int>(config.profiles.size()); ++i) {
        if (toLower(trim(config.profiles[i].host)) == wanted) return i;
    }
    return -1;
}

bool App::loadProfileForHost(const std::string& host) {
    int index = profileIndexForHost(host);
    if (index < 0) return false;
    const ConnectionProfile& profile = config.profiles[index];
    config.name = profile.name;
    activeProfileName = profile.name;
    config.host = profile.host;
    config.username = profile.username.empty() ? "root" : profile.username;
    config.password = profile.password.empty() ? "1" : profile.password;
    selectedProfile = index;
    return true;
}

void App::clearActiveProfile() {
    activeProfileName.clear();
    config.name.clear();
}

void App::saveManualAsProfile() {
    if (!hasCompleteManualConnection()) {
        lastMessage = "Enter host, username and password first.";
        return;
    }

    std::string profileName = activeProfileName.empty() ? "MiSTer" : activeProfileName;
    editText("Profile name", profileName);
    profileName = trim(profileName);
    if (profileName.empty()) {
        lastMessage = "Profile name is required.";
        return;
    }

    ConnectionProfile profile;
    profile.name = profileName;
    profile.host = trim(config.host);
    profile.username = trim(config.username).empty() ? "root" : trim(config.username);
    profile.password = trim(config.password).empty() ? "1" : trim(config.password);
    config.name = profile.name;
    activeProfileName = profile.name;
    config.profiles.push_back(profile);
    saveConfig();
    selectedProfile = static_cast<int>(config.profiles.size()) - 1;
    lastMessage = "Profile saved.";
}

void App::connectProfile(int index) {
    if (index < 0 || index >= static_cast<int>(config.profiles.size())) return;
    const ConnectionProfile& profile = config.profiles[index];
    config.name = profile.name;
    activeProfileName = profile.name;
    config.host = profile.host;
    config.username = profile.username.empty() ? "root" : profile.username;
    config.password = profile.password.empty() ? "1" : profile.password;
    saveConfig();
    connectOrDisconnect();
}

void App::editProfile(int index) {
    if (index < 0 || index >= static_cast<int>(config.profiles.size())) return;
    ConnectionProfile profile = config.profiles[index];
    const std::string originalName = profile.name;
    const std::string originalHost = profile.host;
    int cursor = 0;
    std::string screenMessage = "× Edit    □ Save & Back    ○ Cancel";

    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        ui.beginFrame();
        drawHeader();
        ui.drawCard(220, 126, 840, 474, "EDIT PROFILE");

        std::vector<std::string> labels = {"Name", "Host", "Username", "Password"};
        std::vector<std::string> values = {
            profile.name.empty() ? "Not set" : profile.name,
            profile.host.empty() ? "Not set" : profile.host,
            profile.username.empty() ? "root" : profile.username,
            profile.password.empty() ? "Not set" : "********"
        };

        for (int i = 0; i < 4; ++i) {
            ui.drawButton(260, 196 + i * 64, 760, 54, labels[i] + "  " + values[i], cursor == i, false, false);
        }

        ui.drawMessage(screenMessage);
        ui.drawFooter("UP/DN SELECT    × EDIT    □ SAVE    ○ CANCEL");
        ui.endFrame();

        padUpdate(&pad);
        const u64 buttons = padGetButtonsDown(&pad);
        if (buttons & HidNpadButton_Up) cursor = (cursor + 3) % 4;
        if (buttons & HidNpadButton_Down) cursor = (cursor + 1) % 4;
        if (buttons & HidNpadButton_B) {
            lastMessage = "Profile edit cancelled.";
            return;
        }
        if (buttons & HidNpadButton_A) {
            if (cursor == 0) editText("Profile name", profile.name);
            else if (cursor == 1) editText("MiSTer IP or hostname", profile.host);
            else if (cursor == 2) editText("SSH username", profile.username);
            else if (cursor == 3) editText("SSH password", profile.password, true);
            screenMessage = "× Edit    □ Save & Back    ○ Cancel";
            while (appletMainLoop()) {
                padUpdate(&pad);
                if ((padGetButtons(&pad) & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X | HidNpadButton_Up | HidNpadButton_Down)) == 0) break;
            }
        }
        if (buttons & HidNpadButton_X) {
            profile.name = trim(profile.name);
            profile.host = trim(profile.host);
            profile.username = trim(profile.username);
            profile.password = trim(profile.password);
            if (profile.name.empty()) { screenMessage = "Profile name is required."; continue; }
            if (profile.host.empty()) { screenMessage = "Host is required."; continue; }
            if (profile.username.empty()) profile.username = "root";
            if (profile.password.empty()) profile.password = "1";

            const bool editedActiveProfile = !activeProfileName.empty() && activeProfileName == originalName;
            const bool manualWasPointingAtProfile = toLower(trim(config.host)) == toLower(trim(originalHost));
            config.profiles[index] = profile;
            if (editedActiveProfile || manualWasPointingAtProfile) {
                config.name = profile.name;
                activeProfileName = profile.name;
                config.host = profile.host;
                config.username = profile.username;
                config.password = profile.password;
            }
            saveConfig();
            lastMessage = "Profile updated.";
            return;
        }
    }
}

void App::deleteProfile(int index) {
    if (index < 0 || index >= static_cast<int>(config.profiles.size())) return;
    std::string name = config.profiles[index].name;
    std::string body = "DELETE PROFILE " + name + "?";
    if (!confirm("DELETE PROFILE", body.c_str())) return;

    config.profiles.erase(config.profiles.begin() + index);
    if (activeProfileName == name) {
        activeProfileName.clear();
        config.name.clear();
    }
    if (selectedProfile >= static_cast<int>(config.profiles.size())) selectedProfile = static_cast<int>(config.profiles.size()) - 1;
    if (selectedProfile < 0) selectedProfile = 0;
    profileScroll = 0;
    saveConfig();
    lastMessage = "Profile deleted.";
}

void App::connectScannedHost(const std::string& host, bool saveAsProfile) {
    if (host.empty()) return;

    int existingProfile = profileIndexForHost(host);
    if (existingProfile >= 0) {
        loadProfileForHost(host);
        saveConfig();
        lastMessage = "Profile loaded for this IP.";
        connectOrDisconnect();
        return;
    }

    config.host = host;
    clearActiveProfile();
    if (trim(config.username).empty()) config.username = "root";
    if (trim(config.password).empty()) config.password = "1";

    if (saveAsProfile) {
        std::string profileName = "MiSTer " + host;
        editText("Profile name", profileName);
        profileName = trim(profileName);
        if (profileName.empty()) {
            lastMessage = "Profile name is required.";
            return;
        }
        ConnectionProfile profile;
        profile.name = profileName;
        profile.host = config.host;
        profile.username = config.username;
        profile.password = config.password;
        config.name = profile.name;
        activeProfileName = profile.name;
        config.profiles.push_back(profile);
        selectedProfile = static_cast<int>(config.profiles.size()) - 1;
    }

    saveConfig();
    connectOrDisconnect();
}

void App::showMisterScanWindow() {
    std::string subnet = subnetBaseFromIp(config.host);
    if (subnet.empty()) subnet = currentSubnetBase();
    if (subnet.empty()) subnet = "192.168.1.";

    std::vector<std::string> foundHosts;
    std::string message = "Scanning " + subnet + "1-254 for MiSTer...";

    PadState pad;
    padInitializeDefault(&pad);

    auto drawScan = [&](const std::string& line, int progress, bool completed, int selectedHost) {
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, UiRenderer::Height, UiRenderer::rgb(12, 10, 20));
        ui.drawCard(220, 105, 840, 500, "SCAN FOR MISTER");
        ui.drawText(260, 170, line, UiRenderer::rgb(248, 245, 255), 2);
        if (!completed) {
            ui.drawText(260, 220, "PROGRESS  " + std::to_string(progress) + " / 254", UiRenderer::rgb(174, 154, 218), 2);
            ui.drawFooter("SCANNING...    ○ CANCEL");
        } else {
            if (foundHosts.empty()) {
                ui.drawText(260, 250, "NO MISTER DEVICES FOUND", UiRenderer::rgb(248, 245, 255), 3);
                ui.drawText(260, 310, "CHECK THAT YOUR SWITCH AND MISTER ARE ON THE SAME NETWORK", UiRenderer::rgb(218, 208, 238), 2);
            } else {
                ui.drawText(260, 220, "FOUND MISTER DEVICES", UiRenderer::rgb(174, 154, 218), 2);
                const int visibleRows = 5;
                int scroll = selectedHost - visibleRows + 1;
                if (scroll < 0) scroll = 0;
                for (int row = 0; row < visibleRows; ++row) {
                    int index = scroll + row;
                    if (index >= static_cast<int>(foundHosts.size())) break;
                    ui.drawButton(260, 260 + row * 62, 760, 54, foundHosts[index], selectedHost == index);
                }
            }
            ui.drawFooter("UP/DN SELECT    × CONNECT    □ SAVE & CONNECT    ○ BACK");
        }
        ui.endFrame();
    };

    bool cancelled = false;
    for (int i = 1; i <= 254; ++i) {
        padUpdate(&pad);
        if (padGetButtonsDown(&pad) & HidNpadButton_B) {
            cancelled = true;
            break;
        }

        const std::string host = subnet + std::to_string(i);
        if (i == 1 || i % 8 == 0) drawScan(message, i, false, 0);
        if (tcpPortOpen(host, 22, 18)) {
            drawScan("Checking " + host + " for MiSTer identity...", i, false, 0);
            if (hostLooksLikeMister(host, config)) foundHosts.push_back(host);
        }
    }

    if (cancelled) {
        lastMessage = "Scan cancelled.";
        return;
    }

    int scanSelected = 0;
    bool inputReleased = false;
    while (appletMainLoop()) {
        drawScan(foundHosts.empty() ? "Scan complete." : "Scan complete. Select a MiSTer.", 254, true, scanSelected);
        padUpdate(&pad);
        const u64 held = padGetButtons(&pad);
        const u64 buttons = padGetButtonsDown(&pad);
        if (!inputReleased) {
            const u64 block = HidNpadButton_A | HidNpadButton_B | HidNpadButton_X | HidNpadButton_Up | HidNpadButton_Down;
            if ((held & block) == 0) inputReleased = true;
            continue;
        }
        if (buttons & HidNpadButton_B) {
            lastMessage = "Scan closed.";
            return;
        }
        if (!foundHosts.empty()) {
            if (buttons & HidNpadButton_Up) scanSelected = (scanSelected + static_cast<int>(foundHosts.size()) - 1) % static_cast<int>(foundHosts.size());
            if (buttons & HidNpadButton_Down) scanSelected = (scanSelected + 1) % static_cast<int>(foundHosts.size());
            if (buttons & HidNpadButton_A) {
                connectScannedHost(foundHosts[scanSelected], false);
                return;
            }
            if (buttons & HidNpadButton_X) {
                connectScannedHost(foundHosts[scanSelected], true);
                return;
            }
        }
    }
}

void App::connectOrDisconnect() {
    if (ssh.isConnected()) {
        if (passthroughActive) stopPassthrough();
        remote.disconnect();
        ssh.disconnect();
        status = "Disconnected";
        lastMessage = "Disconnected.";
        return;
    }

    status = "Connecting...";
    draw();

    std::string message;
    if (ssh.connect(config, message)) {
        status = "Connected";
        lastMessage = message;
        saveConfig();
        refreshDevice();
    } else {
        status = "Disconnected";
        lastMessage = message;
    }
}

std::string App::runCommandMessage(const std::string& command) {
    if (!ssh.isConnected()) return "No active MiSTer connection.";
    SshResult result = ssh.runCommand(command);
    if (result.success) return result.output.empty() ? "Command completed." : result.output;
    return result.error.empty() ? "Command failed." : result.error;
}

void App::refreshDevice() {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }
    refreshStorage();
    refreshSmb();
    refreshNowPlaying();
    lastMessage = "Device info refreshed.";
}

void App::refreshStorage() {
    SshResult sd = ssh.runCommand("df -h /media/fat | tail -1");
    sdStorage = sd.success ? formatDfLine(sd.output) : "Unable to read storage";

    SshResult usb = ssh.runCommand("df -h | grep /media/usb | head -1");
    usbStorage = usb.success && !usb.output.empty() ? formatDfLine(usb.output) : "No USB storage detected";
}

void App::refreshSmb() {
    SshResult result = ssh.runCommand("test -f /media/fat/linux/samba.sh && echo enabled || echo disabled");
    if (!result.success) {
        smbStatus = "Unknown";
        return;
    }
    smbStatus = trim(result.output) == "enabled" ? "Enabled" : "Disabled";
}

void App::refreshNowPlaying() {
    SshResult coreResult = ssh.runCommand("cat /tmp/CORENAME 2>/dev/null");
    SshResult activeGameResult = ssh.runCommand("cat /tmp/ACTIVEGAME 2>/dev/null");

    std::string activeGame = activeGameResult.success ? trim(activeGameResult.output) : "";
    if (activeGame.empty()) {
        SshResult fullPathResult = ssh.runCommand("cat /tmp/FULLPATH 2>/dev/null");
        activeGame = fullPathResult.success ? trim(fullPathResult.output) : "";
    }

    std::string core = coreResult.success ? normalizeCoreName(coreResult.output) : "";
    std::string upper = core;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    if (!core.empty() && upper != "UNKNOWN" && upper != "MENU" && !activeGame.empty()) {
        nowPlaying = core + " | " + prettifyGameName(activeGame);
    } else {
        nowPlaying.clear();
    }
}

void App::toggleSmb() {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }

    refreshSmb();
    const bool enable = smbStatus != "Enabled";
    const char* command = enable
        ? "if [ -f /media/fat/linux/_samba.sh ]; then mv /media/fat/linux/_samba.sh /media/fat/linux/samba.sh; fi"
        : "if [ -f /media/fat/linux/samba.sh ]; then mv /media/fat/linux/samba.sh /media/fat/linux/_samba.sh; fi";

    SshResult result = ssh.runCommand(command);
    if (result.success) {
        refreshSmb();
        lastMessage = enable ? "SMB enabled. A reboot is required." : "SMB disabled. A reboot is required.";
    } else {
        lastMessage = result.error.empty() ? "Unable to change SMB status." : result.error;
    }
}

void App::returnToMenu() {
    lastMessage = runCommandMessage("echo \"load_core /media/fat/menu.rbf\" > /dev/MiSTer_cmd");
    refreshNowPlaying();
}

void App::reboot() {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }
    if (!confirm("CONFIRM REBOOT", "ARE YOU SURE YOU WANT TO REBOOT THE MISTER?")) return;

    SshResult result = ssh.runCommand("sync; nohup /sbin/reboot >/dev/null 2>&1 &");
    if (result.success) {
        waitForReconnectAfterReboot("REBOOT MISTER", "Reboot command sent. Waiting for MiSTer...");
        return;
    }

    lastMessage = result.error.empty() ? "Reboot command may have failed." : result.error;
}

bool App::sendSoftRebootCommand() {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return false;
    }

    // Match the desktop app behavior: its Extras soft reboot path calls
    // return_to_menu_remote(), which sends a MiSTer menu reload command through
    // /dev/MiSTer_cmd instead of using /sbin/reboot or echo reboot.
    SshResult result = ssh.runCommand(std::string("sync; ") + MisterMenuReloadCommand);
    lastMessage = result.success ? "Soft reboot command sent." : "Soft reboot command may have failed.";
    return result.success;
}

void App::waitForReconnectAfterReboot(const std::string& title, const std::string& firstLine) {
    if (passthroughActive) passthroughActive = false;
    remote.disconnect();
    ssh.disconnect();

    status = "Rebooting...";
    sdStorage = "Rebooting...";
    usbStorage = "Rebooting...";
    smbStatus = "Rebooting...";
    nowPlaying.clear();

    auto drawReconnectWindow = [&](const std::string& line, int attempt, bool canCancel) {
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, 92, UiRenderer::rgb(20, 16, 34));
        ui.fillRect(0, 90, UiRenderer::Width, 4, UiRenderer::rgb(143, 84, 255));
        ui.drawText(42, 30, title, UiRenderer::rgb(248, 245, 255), 3);
        ui.drawCard(240, 180, 800, 320, "RECONNECTING");
        ui.drawText(300, 270, line, UiRenderer::rgb(248, 245, 255), 2);
        if (attempt > 0) {
            ui.drawText(300, 326, "Attempt: " + std::to_string(attempt), UiRenderer::rgb(174, 154, 218), 2);
        }
        ui.drawText(300, 382, "MiSTer Companion Vita will reconnect automatically.", UiRenderer::rgb(218, 208, 238), 2);
        ui.drawFooter(canCancel ? "○ STOP WAITING" : "PLEASE WAIT");
        ui.endFrame();
    };

    drawReconnectWindow(firstLine, 0, false);
    svcSleepThread(700000000LL);

    PadState pad;
    padInitializeDefault(&pad);
    bool inputReleased = false;

    auto checkCancel = [&]() -> bool {
        padUpdate(&pad);
        const u64 held = padGetButtons(&pad);
        const u64 down = padGetButtonsDown(&pad);
        if (!inputReleased) {
            if ((held & HidNpadButton_B) == 0) inputReleased = true;
            return false;
        }
        return (down & HidNpadButton_B) != 0;
    };

    auto cancelReconnect = [&]() {
        status = "Disconnected";
        sdStorage = "Not refreshed";
        usbStorage = "Not refreshed";
        smbStatus = "Not refreshed";
        lastMessage = "Reconnect cancelled.";
    };

    bool rebootStarted = false;
    for (int tick = 0; tick < 90 && appletMainLoop(); tick++) {
        drawReconnectWindow("Waiting for MiSTer to start rebooting...", 0, true);
        if (checkCancel()) {
            cancelReconnect();
            return;
        }

        std::string probeMessage;
        if (!ssh.connect(config, probeMessage)) {
            rebootStarted = true;
            break;
        }
        ssh.disconnect();
        svcSleepThread(100000000LL);
    }

    if (!rebootStarted) {
        for (int tick = 0; tick < 20 && appletMainLoop(); tick++) {
            drawReconnectWindow("Reboot is taking longer to start. Waiting...", 0, true);
            if (checkCancel()) {
                cancelReconnect();
                return;
            }
            svcSleepThread(100000000LL);
        }
    }

    for (int attempt = 1; attempt <= 45 && appletMainLoop(); attempt++) {
        drawReconnectWindow("Waiting for MiSTer to come back online...", attempt, true);

        bool cancelled = false;
        for (int tick = 0; tick < 20 && appletMainLoop(); tick++) {
            if (checkCancel()) {
                cancelled = true;
                break;
            }
            svcSleepThread(100000000LL);
        }

        if (cancelled) {
            cancelReconnect();
            return;
        }

        status = "Reconnecting...";
        drawReconnectWindow("Trying to reconnect over SSH...", attempt, true);

        std::string message;
        if (ssh.connect(config, message)) {
            status = "Connected";
            lastMessage = "MiSTer reboot complete. Reconnected.";
            refreshDevice();
            if (tab == Tab::Extras) refreshCurrentExtraStatus(false);
            return;
        }
        ssh.disconnect();
    }

    status = "Disconnected";
    sdStorage = "Not refreshed";
    usbStorage = "Not refreshed";
    smbStatus = "Not refreshed";
    lastMessage = "MiSTer did not reconnect automatically.";
}

void App::softRebootAndReconnect(const std::string& title) {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }
    sendSoftRebootCommand();
    waitForReconnectAfterReboot(title, "Soft reboot command sent. Waiting for MiSTer...");
}

void App::refreshRemoteStatus() {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }

    const std::string command =
        "if [ -x /media/fat/Scripts/companion_remote.sh ]; then "
        "/media/fat/Scripts/companion_remote.sh status --unattended; "
        "else "
        "echo SCRIPT_INSTALLED=0; "
        "echo DAEMON_INSTALLED=0; "
        "echo DAEMON_RUNNING=0; "
        "echo PORT_LISTENING=0; "
        "echo START_ON_BOOT=0; "
        "fi";

    SshResult result = ssh.runCommand(command);
    if (!result.success) {
        lastMessage = result.error.empty() ? "Unable to read remote status." : result.error;
        return;
    }

    remoteInstalled = contains(result.output, "DAEMON_INSTALLED=1") || contains(result.output, "SCRIPT_INSTALLED=1") ? "Yes" : "No";
    remoteRunning = contains(result.output, "DAEMON_RUNNING=1") || contains(result.output, "PORT_LISTENING=1") ? "Yes" : "No";
    remoteStartup = contains(result.output, "START_ON_BOOT=1") ? "Enabled" : "Disabled";
    lastMessage = "Remote status refreshed.";
}

void App::installRemoteDaemon() {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }

    const std::string command =
        "mkdir -p /media/fat/Scripts && "
        "if ! command -v wget >/dev/null 2>&1; then echo wget not found.; exit 1; fi; "
        "wget --no-check-certificate -O " + std::string(RemoteScriptPath) + " " + RemoteScriptUrl + " && "
        "test -s " + std::string(RemoteScriptPath) + " && "
        "chmod +x " + std::string(RemoteScriptPath) + " && " +
        remoteManageCommand("install");

    lastMessage = runCommandMessage(command);
    refreshRemoteStatus();
}

void App::startRemoteDaemon() {
    lastMessage = runCommandMessage(remoteManageCommand("start"));
    refreshRemoteStatus();
}

void App::stopRemoteDaemon() {
    std::string message;
    if (remote.isConnected()) remote.releaseAll(message);
    remote.disconnect();
    lastMessage = runCommandMessage(remoteManageCommand("stop"));
    refreshRemoteStatus();
}

void App::toggleRemoteStartup() {
    const bool enabled = remoteStartup == "Enabled";
    lastMessage = runCommandMessage(remoteManageCommand(enabled ? "disable-boot" : "enable-boot"));
    refreshRemoteStatus();
}

void App::uninstallRemoteDaemon() {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }
    if (!confirm("UNINSTALL REMOTE", "REMOVE THE COMPANION REMOTE DAEMON?")) return;

    std::string message;
    if (remote.isConnected()) remote.releaseAll(message);
    remote.disconnect();
    lastMessage = runCommandMessage(remoteManageCommand("uninstall"));
    refreshRemoteStatus();
}


void App::startPassthrough() {
    std::string message;
    if (!remote.isConnected()) {
        if (!remote.connect(config.host, message)) {
                lastMessage = message;
            return;
        }
    }

    passthroughActive = true;
    lastMessage.clear();
}

void App::stopPassthrough() {
    std::string message;
    if (remote.isConnected()) remote.releaseAll(message);
    remote.disconnect();
    passthroughActive = false;
    lastMessage = "Passthrough stopped. All inputs released.";
}

std::string App::scriptTitle(ScriptId id) const {
    switch (id) {
        case ScriptId::UpdateAll: return "UPDATE ALL";
        case ScriptId::Zaparoo: return "ZAPAROO";
        case ScriptId::MigrateSd: return "MIGRATE SD";
        case ScriptId::CifsMount: return "CIFS MOUNT";
        case ScriptId::AutoTime: return "AUTO TIME";
        case ScriptId::CdGameOrganizer: return "CD GAME ORGANIZER";
        case ScriptId::DavBrowser: return "DAV BROWSER";
        case ScriptId::FtpSaveSync: return "FTP SAVE SYNC";
        case ScriptId::StaticWallpaper: return "STATIC WALLPAPER";
        case ScriptId::Syncthing: return "SYNCTHING";
        case ScriptId::RaViewer: return "RA VIEWER";
    }
    return "SCRIPT";
}

std::vector<std::string> App::scriptActions(ScriptId id) {
    const int idx = static_cast<int>(id);
    const std::vector<std::string> lines = idx >= 0 && idx < static_cast<int>(cachedScriptStatus.size())
        ? cachedScriptStatus[idx]
        : std::vector<std::string>{};

    const bool installed = statusYes(lines, "INSTALLED");
    const bool config = statusYes(lines, "CONFIG");
    const bool startup = statusYes(lines, "START ON BOOT");

    switch (id) {
        case ScriptId::UpdateAll:
            if (installed) return {"RUN", "CONFIGURE", "UNINSTALL"};
            return {"INSTALL"};
        case ScriptId::Zaparoo:
            if (installed) return {startup ? "DISABLE START ON BOOT" : "ENABLE START ON BOOT", "UNINSTALL"};
            return {"INSTALL"};
        case ScriptId::MigrateSd:
            return installed ? std::vector<std::string>{"UNINSTALL"} : std::vector<std::string>{"INSTALL"};
        case ScriptId::CifsMount: {
            if (!installed) return {"INSTALL"};
            std::vector<std::string> actions = {"CONFIGURE", "MOUNT", "UNMOUNT"};
            if (config) actions.push_back("REMOVE CONFIG");
            actions.push_back("UNINSTALL");
            return actions;
        }
        case ScriptId::AutoTime:
            return installed ? std::vector<std::string>{"UNINSTALL"} : std::vector<std::string>{"INSTALL"};
        case ScriptId::CdGameOrganizer:
            return installed ? std::vector<std::string>{"UNINSTALL"} : std::vector<std::string>{"INSTALL"};
        case ScriptId::DavBrowser: {
            if (!installed) return {"INSTALL"};
            std::vector<std::string> actions = {"CONFIGURE"};
            if (config) actions.push_back("REMOVE CONFIG");
            actions.push_back("UNINSTALL");
            return actions;
        }
        case ScriptId::FtpSaveSync: {
            if (!installed) return {"INSTALL"};
            std::vector<std::string> actions = {"CONFIGURE", startup ? "DISABLE START ON BOOT" : "ENABLE START ON BOOT"};
            if (config) actions.push_back("REMOVE CONFIG");
            actions.push_back("UNINSTALL");
            return actions;
        }
        case ScriptId::StaticWallpaper:
            return installed ? std::vector<std::string>{"UNINSTALL"} : std::vector<std::string>{"INSTALL"};
        case ScriptId::Syncthing:
            if (installed) return {startup ? "DISABLE START ON BOOT" : "ENABLE START ON BOOT", "UNINSTALL"};
            return {"INSTALL"};
        case ScriptId::RaViewer: {
            if (!installed) return {"INSTALL"};
            std::vector<std::string> actions = {"EDIT CONFIG"};
            actions.push_back("UNINSTALL");
            return actions;
        }
    }
    return {};
}

void App::refreshCurrentScriptStatus() {
    if (selectedScript < 0 || selectedScript >= ScriptCount) return;
    if (!ssh.isConnected()) {
        cachedScriptStatus[selectedScript] = {"STATUS: DISCONNECTED"};
        return;
    }
    cachedScriptStatus[selectedScript] = scriptStatus(static_cast<ScriptId>(selectedScript));
}

std::vector<std::string> App::scriptStatus(ScriptId id) {
    if (!ssh.isConnected()) return {"STATUS: DISCONNECTED"};

    auto yesNo = [&](const std::string& cmd) -> std::string {
        SshResult r = ssh.runCommand(cmd);
        return r.success && contains(r.output, "YES") ? "YES" : "NO";
    };

    switch (id) {
        case ScriptId::UpdateAll:
            return {"INSTALLED: " + yesNo(execStatusCommand(UpdateAllPath))};
        case ScriptId::Zaparoo:
            return {"INSTALLED: " + yesNo(execStatusCommand(ZaparooPath)),
                    "START ON BOOT: " + yesNo("grep -F 'mrext/zaparoo' " + std::string(UserStartupPath) + " >/dev/null 2>&1 && echo YES || echo NO")};
        case ScriptId::MigrateSd:
            return {"INSTALLED: " + yesNo(execStatusCommand(MigrateSdPath))};
        case ScriptId::CifsMount:
            return {"INSTALLED: " + yesNo(execStatusCommand(CifsMountPath)),
                    "CONFIG: " + yesNo(fileStatusCommand(CifsConfigPath))};
        case ScriptId::AutoTime:
            return {"INSTALLED: " + yesNo(execStatusCommand(AutoTimePath))};
        case ScriptId::CdGameOrganizer:
            return {"INSTALLED: " + yesNo(execStatusCommand(CdGameOrganizerPath))};
        case ScriptId::DavBrowser:
            return {"INSTALLED: " + yesNo(execStatusCommand(DavBrowserPath)),
                    "CONFIG: " + yesNo(fileStatusCommand(DavBrowserConfigPath))};
        case ScriptId::FtpSaveSync:
            return {"INSTALLED: " + yesNo(execStatusCommand(FtpSaveSyncPath)),
                    "CONFIG: " + yesNo(fileStatusCommand(FtpSaveSyncConfigPath)),
                    "START ON BOOT: " + yesNo("grep -F 'ftp_save_sync_daemon.sh' " + std::string(UserStartupPath) + " >/dev/null 2>&1 && echo YES || echo NO")};
        case ScriptId::StaticWallpaper:
            return {"INSTALLED: " + yesNo(execStatusCommand(StaticWallpaperPath)),
                    "ACTIVE: " + yesNo("test -f /media/fat/menu.jpg -o -f /media/fat/menu.png && echo YES || echo NO")};
        case ScriptId::Syncthing:
            return {"INSTALLED: " + yesNo(execStatusCommand(SyncthingPath)),
                    "START ON BOOT: " + yesNo("grep -F 'syncthing_service.sh' " + std::string(UserStartupPath) + " >/dev/null 2>&1 && echo YES || echo NO")};
        case ScriptId::RaViewer:
            return {"INSTALLED: " + yesNo(execStatusCommand(RaViewerPath)),
                    "CONFIG: " + yesNo(fileStatusCommand(RaViewerConfigPath))};
    }
    return {"STATUS: UNKNOWN"};
}

void App::ensureScriptsDirs() {
    runCommandMessage("mkdir -p /media/fat/Scripts /media/fat/linux /media/fat/Scripts/.config/update_all /media/fat/Scripts/.config/dav_browser /media/fat/Scripts/.config/ftp_save_sync /media/fat/Scripts/.config/ra_viewer /media/fat/Scripts/.config/syncthing");
}

bool App::askField(const char* title, std::string& value, bool password) {
    std::string before = value;
    editText(title, value, password);
    return value != before || !value.empty();
}

bool App::askYesNo(const char* title, const char* body, bool defaultYes) {
    (void)defaultYes;
    return confirm(title, body);
}

void App::scriptInstall(const std::string& title, const std::string& path, const std::string& url) {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }
    ensureScriptsDirs();
    showStreamingCommandWindow("INSTALL " + title, downloadCommand(url, path), title + " installed.", title + " install failed.");
}

void App::scriptUninstall(const std::string& title, const std::string& command) {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }
    if (!confirm(("UNINSTALL " + title).c_str(), "ARE YOU SURE?")) return;
    lastMessage = runCommandMessage(command);
    if (lastMessage.empty()) lastMessage = title + " uninstalled.";
}

void App::showOutputWindow(const std::string& title, const std::string& output) {
    std::vector<std::string> lines;
    std::stringstream stream(output.empty() ? std::string("No output.") : output);
    std::string line;
    while (std::getline(stream, line)) {
        line = trim(line);
        if (line.empty()) {
            lines.push_back("");
            continue;
        }
        const size_t maxChars = 74;
        while (line.size() > maxChars) {
            lines.push_back(line.substr(0, maxChars));
            line = line.substr(maxChars);
        }
        lines.push_back(line);
    }
    if (lines.empty()) lines.push_back("No output.");

    int scroll = 0;
    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);
        if (down & (HidNpadButton_B | HidNpadButton_A | HidNpadButton_Plus)) break;
        if (down & HidNpadButton_Up) scroll = std::max(0, scroll - 1);
        if (down & HidNpadButton_Down) scroll = std::min(std::max(0, static_cast<int>(lines.size()) - 11), scroll + 1);

        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, 92, UiRenderer::rgb(20, 16, 34));
        ui.fillRect(0, 90, UiRenderer::Width, 4, UiRenderer::rgb(143, 84, 255));
        ui.drawText(42, 30, title, UiRenderer::rgb(248, 245, 255), 3);
        ui.drawCard(40, 124, 1200, 500, "OUTPUT");

        int y = 188;
        for (int i = scroll; i < static_cast<int>(lines.size()) && y <= 570; i++) {
            ui.drawText(76, y, lines[i], UiRenderer::rgb(218, 208, 238), 2);
            y += 34;
        }

        ui.drawFooter("UP/DN SCROLL    ×/○ CLOSE    START CLOSE");
        ui.endFrame();
    }
}

bool App::showStreamingCommandWindow(const std::string& title, const std::string& command, const std::string& successMessage, const std::string& failureMessage, bool* rebootDetected) {
    std::vector<std::string> lines;
    lines.push_back("Starting...");
    int scroll = 0;
    bool completed = false;
    bool success = false;
    std::string finalError;
    std::string completionTitle = "RUNNING";
    bool detectedReboot = false;

    auto appendText = [&](const std::string& text) {
        std::stringstream stream(text);
        std::string line;
        bool any = false;
        while (std::getline(stream, line)) {
            any = true;
            line = trim(line);
            const size_t maxChars = 74;
            if (line.empty()) {
                lines.push_back("");
            } else {
                while (line.size() > maxChars) {
                    lines.push_back(line.substr(0, maxChars));
                    line = line.substr(maxChars);
                }
                lines.push_back(line);
            }
        }
        if (!any && !text.empty()) lines.push_back(text);
        const int maxScroll = std::max(0, static_cast<int>(lines.size()) - 11);
        scroll = maxScroll;
    };

    auto drawStreamingWindow = [&]() {
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, 92, UiRenderer::rgb(20, 16, 34));
        ui.fillRect(0, 90, UiRenderer::Width, 4, UiRenderer::rgb(143, 84, 255));
        ui.drawText(42, 30, title, UiRenderer::rgb(248, 245, 255), 3);
        ui.drawCard(40, 124, 1200, 500, completed ? completionTitle : "RUNNING");

        int y = 188;
        for (int i = scroll; i < static_cast<int>(lines.size()) && y <= 570; i++) {
            ui.drawText(76, y, lines[i], UiRenderer::rgb(218, 208, 238), 2);
            y += 34;
        }

        ui.drawFooter(completed ? "UP/DN SCROLL    ○ CLOSE    × CLOSE" : "UP/DOWN SCROLL    PLEASE WAIT");
        ui.endFrame();
    };

    drawStreamingWindow();

    SshResult result = ssh.runCommandStreaming(command + " 2>&1", [&](const std::string& chunk) {
        appendText(chunk);
        if (textMentionsReboot(chunk)) detectedReboot = true;

        PadState pad;
        padInitializeDefault(&pad);
        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);
        if (down & HidNpadButton_Up) scroll = std::max(0, scroll - 1);
        if (down & HidNpadButton_Down) scroll = std::min(std::max(0, static_cast<int>(lines.size()) - 11), scroll + 1);

        drawStreamingWindow();
    });

    if (textMentionsReboot(result.output) || textMentionsReboot(result.error)) detectedReboot = true;
    if (rebootDetected) *rebootDetected = detectedReboot;

    success = result.success;
    completionTitle = success ? "FINISHED" : "FAILED";
    if (!result.error.empty()) finalError = result.error;
    appendText("");
    appendText(success ? "Finished successfully." : "Finished with errors.");
    appendText(success ? successMessage : failureMessage);
    if (!finalError.empty()) appendText(finalError);
    appendText("");
    appendText("Press ○ to close.");
    completed = true;
    drawStreamingWindow();

    PadState pad;
    padInitializeDefault(&pad);
    while (appletMainLoop()) {
        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);
        if (down & (HidNpadButton_B | HidNpadButton_A | HidNpadButton_Plus)) break;
        if (down & HidNpadButton_Up) scroll = std::max(0, scroll - 1);
        if (down & HidNpadButton_Down) scroll = std::min(std::max(0, static_cast<int>(lines.size()) - 11), scroll + 1);
        drawStreamingWindow();
    }

    lastMessage = success ? successMessage : failureMessage;
    return success;
}

void App::runScriptCommand(const std::string& title, const std::string& command, bool confirmFirst) {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }
    if (confirmFirst && !confirm(title.c_str(), "ARE YOU SURE?")) return;
    lastMessage = runCommandMessage(command);
    if (lastMessage.empty()) lastMessage = title + " complete.";
}

void App::configureUpdateAll() {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }

    ensureScriptsDirs();

    const std::string mainPath = "/media/fat/downloader.ini";
    const std::string arcadePath = "/media/fat/downloader_arcade_roms_db.ini";
    const std::string biosPath = "/media/fat/downloader_bios_db.ini";
    const std::string manualsPath = "/media/fat/downloader_ajgowans_manualsdb.ini";
    const std::string jsonPath = "/media/fat/Scripts/.config/update_all/update_all.json";
    const std::string arcadeOrganizerPath = "/media/fat/Scripts/update_arcade-organizer.ini";

    struct ConfigChoice {
        std::string label;
        std::vector<std::string> labels;
        int selected = 0;
    };

    struct ConfigToggle {
        std::string label;
        bool enabled = false;
    };

    struct ConfigAction {
        std::string label;
    };

    struct ConfigItem {
        enum class Type { Category, Toggle, Choice, Action } type = Type::Toggle;
        int index = 0;
        std::string label;
        bool indent = false;
        std::string dependsOn;
    };

    SshResult mainResult = ssh.runCommand("cat /media/fat/downloader.ini 2>/dev/null || true");
    SshResult arcadeResult = ssh.runCommand("cat /media/fat/downloader_arcade_roms_db.ini 2>/dev/null || true");
    SshResult biosResult = ssh.runCommand("cat /media/fat/downloader_bios_db.ini 2>/dev/null || true");
    SshResult manualsResult = ssh.runCommand("cat /media/fat/downloader_ajgowans_manualsdb.ini 2>/dev/null || true");
    SshResult jsonResult = ssh.runCommand("cat /media/fat/Scripts/.config/update_all/update_all.json 2>/dev/null || true");
    SshResult arcadeOrganizerResult = ssh.runCommand("cat /media/fat/Scripts/update_arcade-organizer.ini 2>/dev/null || true");

    std::string mainText = mainResult.output;
    std::string arcadeText = arcadeResult.output;
    std::string biosText = biosResult.output;
    std::string manualsText = manualsResult.output;
    std::string jsonText = jsonResult.output;
    std::string arcadeOrganizerText = arcadeOrganizerResult.output;
    std::string combinedText = mainText + "\n" + arcadeText + "\n" + biosText;

    std::vector<ConfigToggle> toggles;
    std::vector<ConfigChoice> choices;
    std::vector<ConfigAction> actions;
    std::vector<ConfigItem> items;

    auto addCategory = [&](const std::string& label) {
        ConfigItem item;
        item.type = ConfigItem::Type::Category;
        item.label = label;
        items.push_back(item);
    };

    auto addToggle = [&](const std::string& label, bool enabled, bool indent = false, const std::string& dependsOn = "") {
        ConfigToggle toggle;
        toggle.label = label;
        toggle.enabled = enabled;
        toggles.push_back(toggle);
        ConfigItem item;
        item.type = ConfigItem::Type::Toggle;
        item.index = static_cast<int>(toggles.size()) - 1;
        item.label = label;
        item.indent = indent;
        item.dependsOn = dependsOn;
        items.push_back(item);
    };

    auto addChoice = [&](const std::string& label, const std::vector<std::string>& labels, int selectedIndex, bool indent = false, const std::string& dependsOn = "") {
        ConfigChoice choice;
        choice.label = label;
        choice.labels = labels;
        choice.selected = std::max(0, std::min(selectedIndex, static_cast<int>(labels.size()) - 1));
        choices.push_back(choice);
        ConfigItem item;
        item.type = ConfigItem::Type::Choice;
        item.index = static_cast<int>(choices.size()) - 1;
        item.label = label;
        item.indent = indent;
        item.dependsOn = dependsOn;
        items.push_back(item);
    };

    auto addAction = [&](const std::string& label, bool indent = false, const std::string& dependsOn = "") {
        ConfigAction action;
        action.label = label;
        actions.push_back(action);
        ConfigItem item;
        item.type = ConfigItem::Type::Action;
        item.index = static_cast<int>(actions.size()) - 1;
        item.label = label;
        item.indent = indent;
        item.dependsOn = dependsOn;
        items.push_back(item);
    };

    int mainSource = 0;
    if (contains(combinedText, "aitorgomez.net")) mainSource = 2;
    else if (contains(combinedText, "MiSTer-DB9")) mainSource = 1;

    std::string frontierFilter = extractSectionValue(combinedText, "MiSTerOrganize/MiSTer_Frontier", "filter");
    int frontierSource = 0;
    if (frontierFilter == "pico-8") frontierSource = 1;
    else if (frontierFilter == "openbor-4086") frontierSource = 2;
    else if (frontierFilter == "openbor-7533") frontierSource = 3;
    else if (frontierFilter == "openbor-4086 openbor-7533") frontierSource = 4;
    else if (frontierFilter == "pico-8 openbor-4086") frontierSource = 5;
    else if (frontierFilter == "pico-8 openbor-7533") frontierSource = 6;

    std::string rannyFilter = extractSectionValue(combinedText, "Ranny-Snice/Ranny-Snice-Wallpapers", "filter");
    int rannySource = 2;
    if (rannyFilter == "ar16-9") rannySource = 0;
    else if (rannyFilter == "ar4-3") rannySource = 1;

    const std::vector<std::pair<std::string, std::string>> manualsSources = {
        {"3do", "3DO"},
        {"arcadia2001", "Arcadia 2001"},
        {"atari2600", "Atari 2600"},
        {"atari5200", "Atari 5200"},
        {"atari7800", "Atari 7800"},
        {"atarilynx", "Atari Lynx"},
        {"atarixegs", "Atari XEGS"},
        {"avision", "Adventure Vision"},
        {"ballyastrocade", "Bally Astrocade"},
        {"bbcbridge", "BBC Bridge"},
        {"cdi", "CD-i"},
        {"channelf", "Channel F"},
        {"colecovision", "ColecoVision"},
        {"creativision", "CreatiVision"},
        {"fds", "Famicom Disk System"},
        {"gameandwatch", "Game & Watch"},
        {"gameboy", "Game Boy"},
        {"gamegear", "Game Gear"},
        {"gba", "Game Boy Advance"},
        {"gbc", "Game Boy Color"},
        {"intellivision", "Intellivision"},
        {"jaguar", "Jaguar"},
        {"jaguarcd", "Jaguar CD"},
        {"lcdhandhelds", "LCD Handhelds"},
        {"megadrive", "Mega Drive"},
        {"n64", "Nintendo 64"},
        {"neogeoaes", "Neo Geo AES"},
        {"neogeocd", "Neo Geo CD"},
        {"nes", "NES"},
        {"ngp", "Neo Geo Pocket"},
        {"ngpc", "Neo Geo Pocket Color"},
        {"odyssey2", "Odyssey 2"},
        {"pokemonmini", "Pokémon Mini"},
        {"psx", "PlayStation"},
        {"pyuutajr", "Pyuuta Jr."},
        {"sega32x", "Sega 32X"},
        {"segacd", "Sega CD"},
        {"segasaturn", "Sega Saturn"},
        {"segasg1000", "SG-1000"},
        {"sms", "Master System"},
        {"snes", "SNES"},
        {"supervision", "Supervision"},
        {"turbografx16", "TurboGrafx-16"},
        {"turbografxcd", "TurboGrafx-CD"},
        {"vc4000", "VC 4000"},
        {"vectrex", "Vectrex"},
        {"wonderswanc", "WonderSwan Color"},
    };

    std::vector<std::string> manualsSelected;
    for (const auto& source : manualsSources) {
        if (sectionEnabledInText(manualsText, "ajgowans/manualsdb-" + source.first)) {
            manualsSelected.push_back(source.first);
        }
    }

    addCategory("Main Cores");
    addToggle("Enable Main Cores", sectionEnabledInText(combinedText, "distribution_mister"));
    addChoice("Source", {"MiSTer-devel", "DB9 / SNAC8", "AitorGomez"}, mainSource, true, "Enable Main Cores");

    addCategory("JTCores");
    addToggle("Enable JTCores", sectionEnabledInText(combinedText, "jtcores"));
    addToggle("Enable Beta Cores", jsonBoolValue(jsonText, "download_beta_cores"), true, "Enable JTCores");

    addCategory("Other Cores");
    addToggle("Coin-Op Collection", sectionEnabledInText(combinedText, "Coin-OpCollection/Distribution-MiSTerFPGA"));
    addToggle("Arcade Offset Folder", sectionEnabledInText(combinedText, "arcade_offset_folder"));
    addToggle("LLAPI Forks Folder", sectionEnabledInText(combinedText, "llapi_folder"));
    addToggle("Unofficial Distribution", sectionEnabledInText(combinedText, "theypsilon_unofficial_distribution"));
    addToggle("Y/C Builds", sectionEnabledInText(combinedText, "MikeS11/YC_Builds-MiSTer"));
    addToggle("agg23 MiSTer Cores", sectionEnabledInText(combinedText, "agg23_db"));
    addToggle("Alt Cores", sectionEnabledInText(combinedText, "ajgowans/alt-cores"));
    addToggle("Dual RAM Console Cores", sectionEnabledInText(combinedText, "TheJesusFish/Dual-Ram-Console-Cores"));
    addToggle("MiSTer Frontier", sectionEnabledInText(combinedText, "MiSTerOrganize/MiSTer_Frontier"));
    addChoice("Filter", {"All Frontier", "PICO-8", "OpenBOR 4086", "OpenBOR 7533", "OpenBOR 4086 + 7533", "PICO-8 + OpenBOR 4086", "PICO-8 + OpenBOR 7533"}, frontierSource, true, "MiSTer Frontier");

    addCategory("Tools & Scripts");
    addToggle("Arcade Organizer", iniBoolValue(arcadeOrganizerText, "ARCADE_ORGANIZER") || jsonBoolValue(jsonText, "introduced_arcade_names_txt"));
    addToggle("MiSTer Extensions", sectionEnabledInText(combinedText, "mrext/all"));
    addToggle("MiSTer SAM", sectionEnabledInText(combinedText, "MiSTer_SAM_files"));
    addToggle("tty2oled Add-on", sectionEnabledInText(combinedText, "tty2oled_files"));
    addToggle("i2c2oled Add-on", sectionEnabledInText(combinedText, "i2c2oled_files"));
    addToggle("RetroSpy Utility", sectionEnabledInText(combinedText, "retrospy/retrospy-MiSTer"));
    addToggle("Anime0t4ku Scripts", sectionEnabledInText(combinedText, "anime0t4ku_mister_scripts"));

    addCategory("Extra Content");
    addToggle("BIOS Database", sectionEnabledInText(combinedText, "bios_db"));
    addToggle("Arcade ROMs Database", sectionEnabledInText(combinedText, "arcade_roms_db"));
    addToggle("Uberyoji Boot ROMs", sectionEnabledInText(combinedText, "uberyoji_mister_boot_roms_mgl"));
    addToggle("Dinierto GBA Borders", sectionEnabledInText(combinedText, "Dinierto/MiSTer-GBA-Borders"));
    addToggle("Anime0t4ku Wallpapers", sectionEnabledInText(combinedText, "anime0t4ku_wallpapers"));
    addToggle("PCN Challenge Wallpapers", sectionEnabledInText(combinedText, "pcn_challenge_wallpapers"));
    addToggle("Ranny Snice Wallpapers", sectionEnabledInText(combinedText, "Ranny-Snice/Ranny-Snice-Wallpapers"));
    addChoice("Source", {"16:9 Wallpapers", "4:3 Wallpapers", "All Wallpapers"}, rannySource, true, "Ranny Snice Wallpapers");
    addToggle("Game Manuals (EN) DB's", !trim(manualsText).empty());
    addAction("Configure", true, "Game Manuals (EN) DB's");

    addCategory("Community Sources");
    addToggle("Insert-Coin", sectionEnabledInText(combinedText, "funkycochise/Insert-Coin"));
    addToggle("PCN Premium Member Wallpapers", sectionEnabledInText(combinedText, "pcn_premium_wallpapers"));

    int configSelected = 0;
    int configScroll = 0;
    bool shouldSave = false;
    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 held = padGetButtons(&pad);
        if ((held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X)) == 0) break;
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.drawCard(260, 240, 760, 180, "UPDATE ALL CONFIG");
        ui.drawText(320, 330, "PREPARING CONFIGURATION SCREEN", UiRenderer::rgb(248, 245, 255), 2);
        ui.endFrame();
    }

    auto toggleEnabledByLabel = [&](const std::string& label) -> bool {
        for (const ConfigToggle& toggle : toggles) {
            if (toggle.label == label) return toggle.enabled;
        }
        return false;
    };

    auto itemEnabled = [&](const ConfigItem& item) -> bool {
        if (item.type == ConfigItem::Type::Category) return false;
        if (!item.dependsOn.empty()) return toggleEnabledByLabel(item.dependsOn);
        return true;
    };

    auto itemSelectable = [&](int index) -> bool {
        return index >= 0 && index < static_cast<int>(items.size()) && items[index].type != ConfigItem::Type::Category;
    };

    auto moveConfigSelection = [&](int delta) {
        if (items.empty()) return;
        int guard = 0;
        do {
            configSelected = (configSelected + delta + static_cast<int>(items.size())) % static_cast<int>(items.size());
            guard++;
        } while (!itemSelectable(configSelected) && guard < static_cast<int>(items.size()));
    };

    if (!itemSelectable(configSelected)) moveConfigSelection(1);

    auto drawManualsMenu = [&]() {
        std::vector<std::string> working = manualsSelected;
        int manualSelected = 0;
        int manualScroll = 0;
        bool saveManuals = false;
        PadState manualsPad;
        padInitializeDefault(&manualsPad);

        while (appletMainLoop()) {
            padUpdate(&manualsPad);
            u64 held = padGetButtons(&manualsPad);
            if (!(held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X | HidNpadButton_Y | HidNpadButton_Plus | HidNpadButton_Up | HidNpadButton_Down))) break;
        }

        auto manualIsSelected = [&](const std::string& id) -> bool {
            return std::find(working.begin(), working.end(), id) != working.end();
        };


        auto drawManuals = [&]() {
            ui.beginFrame();
            ui.clear(UiRenderer::rgb(12, 10, 20));
            ui.fillRect(0, 0, UiRenderer::Width, 96, UiRenderer::rgb(20, 16, 34));
            ui.fillRect(0, 94, UiRenderer::Width, 4, UiRenderer::rgb(143, 84, 255));
            ui.drawText(42, 32, "GAME MANUALS DB'S", UiRenderer::rgb(248, 245, 255), 3);
            ui.drawCard(40, 124, 1200, 500, "MANUAL DATABASES");

            const int visible = 10;
            if (manualSelected < manualScroll) manualScroll = manualSelected;
            if (manualSelected >= manualScroll + visible) manualScroll = manualSelected - visible + 1;
            manualScroll = std::max(0, std::min(manualScroll, std::max(0, static_cast<int>(manualsSources.size()) - visible)));

            int y = 188;
            for (int row = 0; row < visible; row++) {
                int index = manualScroll + row;
                if (index >= static_cast<int>(manualsSources.size())) break;
                bool active = index == manualSelected;
                if (active) {
                    ui.fillRect(70, y - 10, 1140, 38, UiRenderer::rgb(124, 70, 220));
                    ui.drawRect(70, y - 10, 1140, 38, UiRenderer::rgb(184, 140, 255), 2);
                }
                const auto& source = manualsSources[index];
                ui.drawText(90, y, std::string(manualIsSelected(source.first) ? "[X] " : "[ ] ") + source.second, UiRenderer::rgb(248, 245, 255), 2);
                y += 42;
            }

            ui.drawText(76, 638, std::to_string(manualSelected + 1) + " / " + std::to_string(manualsSources.size()), UiRenderer::rgb(174, 154, 218), 2);
            ui.drawFooter("× TOGGLE    □ SAVE & BACK    ○ CANCEL");
            ui.endFrame();
        };

        while (appletMainLoop()) {
            drawManuals();
            padUpdate(&manualsPad);
            u64 down = padGetButtonsDown(&manualsPad);
            if (down & HidNpadButton_Up) manualSelected = (manualSelected + static_cast<int>(manualsSources.size()) - 1) % static_cast<int>(manualsSources.size());
            if (down & HidNpadButton_Down) manualSelected = (manualSelected + 1) % static_cast<int>(manualsSources.size());
            if (down & HidNpadButton_A) {
                const std::string& id = manualsSources[manualSelected].first;
                auto found = std::find(working.begin(), working.end(), id);
                if (found == working.end()) working.push_back(id);
                else working.erase(found);
            }
            if (down & HidNpadButton_X) {
                saveManuals = true;
                break;
            }
            if (down & HidNpadButton_B) {
                saveManuals = false;
                break;
            }
        }

        if (saveManuals) manualsSelected = working;

        while (appletMainLoop()) {
            padUpdate(&manualsPad);
            u64 held = padGetButtons(&manualsPad);
            if (!(held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X | HidNpadButton_Y | HidNpadButton_Plus))) break;
        }
    };

    auto drawConfig = [&]() {
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, 96, UiRenderer::rgb(20, 16, 34));
        ui.fillRect(0, 94, UiRenderer::Width, 4, UiRenderer::rgb(143, 84, 255));
        ui.drawText(42, 32, "UPDATE ALL CONFIG", UiRenderer::rgb(248, 245, 255), 3);
        ui.drawCard(40, 124, 1200, 500, "UPDATE_ALL SOURCES");

        const int visible = 10;
        if (configSelected < configScroll) configScroll = configSelected;
        if (configSelected >= configScroll + visible) configScroll = configSelected - visible + 1;
        configScroll = std::max(0, std::min(configScroll, std::max(0, static_cast<int>(items.size()) - visible)));

        int y = 188;
        for (int row = 0; row < visible; row++) {
            int index = configScroll + row;
            if (index >= static_cast<int>(items.size())) break;
            const ConfigItem& item = items[index];
            bool active = index == configSelected;
            bool enabled = itemEnabled(item);

            if (active) {
                ui.fillRect(70, y - 10, 1140, 38, UiRenderer::rgb(124, 70, 220));
                ui.drawRect(70, y - 10, 1140, 38, UiRenderer::rgb(184, 140, 255), 2);
            }

            if (item.type == ConfigItem::Type::Category) {
                ui.drawText(90, y, item.label, UiRenderer::rgb(184, 140, 255), 2);
                ui.fillRect(90, y + 25, 1080, 2, UiRenderer::rgb(70, 52, 105));
            } else if (item.type == ConfigItem::Type::Toggle) {
                const ConfigToggle& toggle = toggles[item.index];
                const u32 textColor = enabled ? UiRenderer::rgb(248, 245, 255) : UiRenderer::rgb(120, 112, 140);
                const u32 valueColor = enabled
                    ? (toggle.enabled ? UiRenderer::rgb(112, 232, 165) : UiRenderer::rgb(218, 208, 238))
                    : UiRenderer::rgb(120, 112, 140);
                const int x = item.indent ? 130 : 90;
                ui.drawText(x, y, std::string(toggle.enabled ? "[X] " : "[ ] ") + toggle.label, textColor, 2);
                ui.drawText(1010, y, toggle.enabled ? "ON" : "OFF", valueColor, 2);
            } else if (item.type == ConfigItem::Type::Choice) {
                const ConfigChoice& choice = choices[item.index];
                const u32 textColor = enabled ? UiRenderer::rgb(248, 245, 255) : UiRenderer::rgb(120, 112, 140);
                const u32 valueColor = enabled ? UiRenderer::rgb(218, 208, 238) : UiRenderer::rgb(120, 112, 140);
                const int x = item.indent ? 130 : 90;
                ui.drawText(x, y, item.label + ":", textColor, 2);
                ui.drawText(720, y, choice.labels[choice.selected], valueColor, 2);
            } else if (item.type == ConfigItem::Type::Action) {
                const u32 textColor = enabled ? UiRenderer::rgb(248, 245, 255) : UiRenderer::rgb(120, 112, 140);
                const int x = item.indent ? 130 : 90;
                ui.drawText(x, y, "> " + actions[item.index].label, textColor, 2);
                if (enabled) ui.drawText(1010, y, "OPEN", UiRenderer::rgb(218, 208, 238), 2);
            }
            y += 42;
        }

        ui.drawText(76, 638, std::to_string(configSelected + 1) + " / " + std::to_string(items.size()), UiRenderer::rgb(174, 154, 218), 2);
        ui.drawFooter("× TOGGLE/CYCLE/OPEN    □ SAVE & BACK    ○ CANCEL");
        ui.endFrame();
    };

    while (appletMainLoop()) {
        drawConfig();
        padUpdate(&pad);
        u64 down = padGetButtonsDown(&pad);
        if (down & HidNpadButton_Up) moveConfigSelection(-1);
        if (down & HidNpadButton_Down) moveConfigSelection(1);
        if (down & HidNpadButton_A) {
            ConfigItem& item = items[configSelected];
            if (!itemEnabled(item)) {
                continue;
            }
            if (item.type == ConfigItem::Type::Toggle) {
                toggles[item.index].enabled = !toggles[item.index].enabled;
            } else if (item.type == ConfigItem::Type::Choice) {
                choices[item.index].selected = (choices[item.index].selected + 1) % static_cast<int>(choices[item.index].labels.size());
            } else if (item.type == ConfigItem::Type::Action && actions[item.index].label == "Configure") {
                drawManualsMenu();
            }
        }
        if (down & HidNpadButton_X) {
            shouldSave = true;
            break;
        }
        if (down & HidNpadButton_B) {
            shouldSave = false;
            break;
        }
    }

    if (!shouldSave) {
        lastMessage = "Update All config cancelled.";
        return;
    }

    std::vector<std::string> mainLines = splitLines(mainText);
    std::vector<std::string> arcadeLines = splitLines(arcadeText);
    std::vector<std::string> biosLines = splitLines(biosText);

    const std::vector<std::string> mainKnownSections = {
        "distribution_mister", "jtcores", "Coin-OpCollection/Distribution-MiSTerFPGA", "arcade_offset_folder", "llapi_folder",
        "theypsilon_unofficial_distribution", "MikeS11/YC_Builds-MiSTer", "agg23_db", "ajgowans/alt-cores", "TheJesusFish/Dual-Ram-Console-Cores",
        "MiSTerOrganize/MiSTer_Frontier", "mrext/all", "MiSTer_SAM_files", "tty2oled_files", "i2c2oled_files", "retrospy/retrospy-MiSTer",
        "anime0t4ku_mister_scripts", "bios_db", "arcade_roms_db", "uberyoji_mister_boot_roms_mgl", "Dinierto/MiSTer-GBA-Borders",
        "funkycochise/Insert-Coin", "anime0t4ku_wallpapers", "pcn_challenge_wallpapers", "pcn_premium_wallpapers", "Ranny-Snice/Ranny-Snice-Wallpapers"
    };
    for (const std::string& section : mainKnownSections) mainLines = removeSectionFromLines(mainLines, section);
    arcadeLines = removeSectionFromLines(arcadeLines, "arcade_roms_db");
    biosLines = removeSectionFromLines(biosLines, "bios_db");

    auto toggle = [&](int index) -> bool { return toggles[index].enabled; };
    auto choice = [&](int index) -> int { return choices[index].selected; };

    int t = 0;
    int c = 0;
    bool mainCores = toggle(t++);
    int mainChoice = choice(c++);
    bool jtcores = toggle(t++);
    bool jtBeta = toggle(t++);
    bool coinop = toggle(t++);
    bool arcadeOffset = toggle(t++);
    bool llapi = toggle(t++);
    bool unofficial = toggle(t++);
    bool yc = toggle(t++);
    bool agg23 = toggle(t++);
    bool altcores = toggle(t++);
    bool dualram = toggle(t++);
    bool frontier = toggle(t++);
    int frontierChoice = choice(c++);
    bool arcadeOrg = toggle(t++);
    bool mrext = toggle(t++);
    bool sam = toggle(t++);
    bool tty2oled = toggle(t++);
    bool i2c2oled = toggle(t++);
    bool retrospy = toggle(t++);
    bool animeScripts = toggle(t++);
    bool bios = toggle(t++);
    bool arcadeRoms = toggle(t++);
    bool bootroms = toggle(t++);
    bool gbaBorders = toggle(t++);
    bool animeWallpapers = toggle(t++);
    bool pcnChallenge = toggle(t++);
    bool rannyWallpapers = toggle(t++);
    int rannyChoice = choice(c++);
    bool manualsDb = toggle(t++);
    bool insertCoin = toggle(t++);
    bool pcnPremium = toggle(t++);

    if (mainCores) {
        std::string url = "https://raw.githubusercontent.com/MiSTer-devel/Distribution_MiSTer/main/db.json.zip";
        if (mainChoice == 1) url = "https://raw.githubusercontent.com/MiSTer-DB9/Distribution_MiSTer/main/dbencc.json.zip";
        else if (mainChoice == 2) url = "https://www.aitorgomez.net/static/mistermain/db.json.zip";
        appendSection(mainLines, {"[distribution_mister]", "db_url = " + url});
    }
    if (jtcores) appendSection(mainLines, {"[jtcores]", "db_url = https://raw.githubusercontent.com/jotego/jtcores_mister/main/jtbindb.json.zip", "filter = [MiSTer]"});
    if (coinop) appendSection(mainLines, {"[Coin-OpCollection/Distribution-MiSTerFPGA]", "db_url = https://raw.githubusercontent.com/Coin-OpCollection/Distribution-MiSTerFPGA/db/db.json.zip"});
    if (arcadeOffset) appendSection(mainLines, {"[arcade_offset_folder]", "db_url = https://raw.githubusercontent.com/Toryalai1/Arcade_Offset/db/arcadeoffsetdb.json.zip"});
    if (llapi) appendSection(mainLines, {"[llapi_folder]", "db_url = https://raw.githubusercontent.com/MiSTer-LLAPI/LLAPI_folder_MiSTer/main/llapidb.json.zip"});
    if (unofficial) appendSection(mainLines, {"[theypsilon_unofficial_distribution]", "db_url = https://raw.githubusercontent.com/theypsilon/Distribution_Unofficial_MiSTer/main/unofficialdb.json.zip"});
    if (yc) appendSection(mainLines, {"[MikeS11/YC_Builds-MiSTer]", "db_url = https://raw.githubusercontent.com/MikeS11/YC_Builds-MiSTer/db/db.json.zip"});
    if (agg23) appendSection(mainLines, {"[agg23_db]", "db_url = https://raw.githubusercontent.com/agg23/mister-repository/db/manifest.json"});
    if (altcores) appendSection(mainLines, {"[ajgowans/alt-cores]", "db_url = https://raw.githubusercontent.com/ajgowans/alt-cores/db/db.json.zip"});
    if (dualram) appendSection(mainLines, {"[TheJesusFish/Dual-Ram-Console-Cores]", "db_url = https://raw.githubusercontent.com/TheJesusFish/Dual-Ram-Console-Cores/db/db.json.zip"});
    if (frontier) {
        std::vector<std::string> lines = {"[MiSTerOrganize/MiSTer_Frontier]", "db_url = https://raw.githubusercontent.com/MiSTerOrganize/MiSTer_Frontier/db/db.json.zip"};
        const std::vector<std::string> filters = {"", "pico-8", "openbor-4086", "openbor-7533", "openbor-4086 openbor-7533", "pico-8 openbor-4086", "pico-8 openbor-7533"};
        if (!filters[frontierChoice].empty()) lines.push_back("filter = " + filters[frontierChoice]);
        appendSection(mainLines, lines);
    }
    if (mrext) appendSection(mainLines, {"[mrext/all]", "db_url = https://raw.githubusercontent.com/wizzomafizzo/mrext/main/releases/all.json"});
    if (sam) appendSection(mainLines, {"[MiSTer_SAM_files]", "db_url = https://raw.githubusercontent.com/mrchrisster/MiSTer_SAM/db/db.json.zip"});
    if (tty2oled) appendSection(mainLines, {"[tty2oled_files]", "db_url = https://raw.githubusercontent.com/venice1200/MiSTer_tty2oled/main/tty2oleddb.json"});
    if (i2c2oled) appendSection(mainLines, {"[i2c2oled_files]", "db_url = https://raw.githubusercontent.com/venice1200/MiSTer_i2c2oled/main/i2c2oleddb.json"});
    if (retrospy) appendSection(mainLines, {"[retrospy/retrospy-MiSTer]", "db_url = https://raw.githubusercontent.com/retrospy/retrospy-MiSTer/db/db.json.zip"});
    if (animeScripts) appendSection(mainLines, {"[anime0t4ku_mister_scripts]", "db_url = https://raw.githubusercontent.com/Anime0t4ku/0t4ku-mister-scripts/db/db/scripts.json.zip"});
    if (bios) appendSection(biosLines, {"[bios_db]", "db_url = https://raw.githubusercontent.com/ajgowans/BiosDB_MiSTer/db/bios_db.json.zip"});
    if (arcadeRoms) appendSection(arcadeLines, {"[arcade_roms_db]", "db_url = https://raw.githubusercontent.com/zakk4223/ArcadeROMsDB_MiSTer/db/arcade_roms_db.json.zip"});
    if (bootroms) appendSection(mainLines, {"[uberyoji_mister_boot_roms_mgl]", "db_url = https://raw.githubusercontent.com/uberyoji/mister-boot-roms/main/db/uberyoji_mister_boot_roms_mgl.json"});
    if (gbaBorders) appendSection(mainLines, {"[Dinierto/MiSTer-GBA-Borders]", "db_url = https://raw.githubusercontent.com/Dinierto/MiSTer-GBA-Borders/db/db.json.zip"});
    if (insertCoin) appendSection(mainLines, {"[funkycochise/Insert-Coin]", "db_url = https://raw.githubusercontent.com/funkycochise/Insert-Coin/db/db.json.zip"});
    if (animeWallpapers) appendSection(mainLines, {"[anime0t4ku_wallpapers]", "db_url = https://raw.githubusercontent.com/Anime0t4ku/MiSTerWallpapers/db/db/0t4kuwallpapers.json.zip"});
    if (pcnChallenge) appendSection(mainLines, {"[pcn_challenge_wallpapers]", "db_url = https://raw.githubusercontent.com/Anime0t4ku/MiSTerWallpapers/db/db/pcnchallenge.json.zip"});
    if (pcnPremium) appendSection(mainLines, {"[pcn_premium_wallpapers]", "db_url = https://raw.githubusercontent.com/Anime0t4ku/MiSTerWallpapers/db/db/pcnpremium.json.zip"});
    if (rannyWallpapers) {
        std::string filter = rannyChoice == 0 ? "ar16-9" : (rannyChoice == 1 ? "ar4-3" : "all");
        appendSection(mainLines, {"[Ranny-Snice/Ranny-Snice-Wallpapers]", "db_url = https://raw.githubusercontent.com/Ranny-Snice/Ranny-Snice-Wallpapers/db/db.json.zip", "filter = " + filter});
    }

    std::string manualsDbText;
    if (manualsDb && !manualsSelected.empty()) {
        for (size_t i = 0; i < manualsSelected.size(); i++) {
            if (i) manualsDbText += "\n";
            manualsDbText += "[ajgowans/manualsdb-" + manualsSelected[i] + "]\n";
            manualsDbText += "db_url = https://raw.githubusercontent.com/ajgowans/manualsdb-" + manualsSelected[i] + "/db/db.json.zip\n";
        }
    }

    std::string newJson = trim(jsonText).empty()
        ? "{\n    \"migration_version\": 6,\n    \"theme\": \"Blue Installer\",\n    \"mirror\": \"\",\n    \"countdown_time\": 15,\n    \"log_viewer\": true,\n    \"use_settings_screen_theme_in_log_viewer\": true,\n    \"autoreboot\": true,\n    \"download_beta_cores\": false,\n    \"names_region\": \"JP\",\n    \"names_char_code\": \"CHAR18\",\n    \"names_sort_code\": \"Common\",\n    \"introduced_arcade_names_txt\": true,\n    \"pocket_firmware_update\": false,\n    \"pocket_backup\": false,\n    \"timeline_after_logs\": true,\n    \"overscan\": \"medium\",\n    \"monochrome_ui\": false\n}\n"
        : jsonText;
    newJson = setJsonBool(newJson, "download_beta_cores", jtBeta);
    newJson = setJsonBool(newJson, "introduced_arcade_names_txt", arcadeOrg);

    mainLines = normalizeIniLines(mainLines);
    arcadeLines = normalizeIniLines(arcadeLines);
    biosLines = normalizeIniLines(biosLines);

    std::string command;
    command += writeTextCommand(mainPath, joinLines(mainLines));
    command += "\n";
    command += writeTextCommand(arcadePath, joinLines(arcadeLines));
    command += "\n";
    command += writeTextCommand(biosPath, joinLines(biosLines));
    command += "\n";
    command += writeTextCommand(jsonPath, newJson);
    command += "\n";
    if (arcadeOrg) {
        command += writeTextCommand(arcadeOrganizerPath, "ARCADE_ORGANIZER=true\nSKIPALTS=false\n");
    } else {
        command += "rm -f " + shellQuote(arcadeOrganizerPath) + "\n";
    }
    if (manualsDb && !manualsDbText.empty()) {
        command += writeTextCommand(manualsPath, manualsDbText);
    } else {
        command += "rm -f " + shellQuote(manualsPath) + "\n";
    }

    ui.beginFrame();
    ui.clear(UiRenderer::rgb(12, 10, 20));
    ui.drawCard(260, 240, 760, 180, "SAVING CONFIG");
    ui.drawText(310, 330, "PLEASE WAIT WHILE CONFIG IS SAVED", UiRenderer::rgb(248, 245, 255), 2);
    ui.endFrame();

    SshResult saveResult = ssh.runCommand(command);
    if (saveResult.success) {
        lastMessage = "Update All config saved.";
    } else {
        lastMessage = saveResult.error.empty() ? "Update All config save failed." : saveResult.error;
    }
}

void App::configureCifsMount() {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }

    auto cifsValue = [](const std::string& text, const std::string& key) -> std::string {
        for (const std::string& rawLine : splitLines(text)) {
            std::string line = trim(rawLine);
            if (line.empty() || line.front() == '#' || line.front() == ';') continue;
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            if (trim(line.substr(0, eq)) != key) continue;
            std::string value = trim(line.substr(eq + 1));
            if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }
            return value;
        }
        return "";
    };

    SshResult existingResult = ssh.runCommand(std::string("cat ") + CifsConfigPath + " 2>/dev/null || true");
    const std::string existing = existingResult.success ? existingResult.output : "";

    std::string server = cifsValue(existing, "SERVER");
    std::string share = cifsValue(existing, "SHARE");
    std::string username = cifsValue(existing, "USERNAME");
    std::string password = cifsValue(existing, "PASSWORD");
    std::string mountValue = toLower(cifsValue(existing, "MOUNT_AT_BOOT"));
    bool mountAtBoot = mountValue.empty() ? true : mountValue != "false";

    std::string screenMessage = "";
    int field = 0;
    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 held = padGetButtons(&pad);
        if ((held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X)) == 0) break;
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.drawCard(260, 240, 760, 180, "CIFS CONFIG");
        ui.drawText(320, 330, "PREPARING CONFIGURATION SCREEN", UiRenderer::rgb(248, 245, 255), 2);
        ui.endFrame();
    }

    auto maskedPassword = [&]() -> std::string {
        if (password.empty()) return "Not set";
        return std::string(password.size() > 16 ? 16 : password.size(), '*');
    };

    while (appletMainLoop()) {
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, UiRenderer::Height, UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, 96, UiRenderer::rgb(20, 16, 34));
        ui.fillRect(0, 94, UiRenderer::Width, 4, UiRenderer::rgb(143, 84, 255));
        ui.drawText(42, 32, "CIFS MOUNT CONFIG", UiRenderer::rgb(248, 245, 255), 3);
        ui.drawStatusPill(UiRenderer::Width - 250, 28, "CONFIG", true);

        ui.drawCard(80, 138, 1120, 470, "NETWORK SHARE");
        ui.drawText(118, 204, "SERVER IP", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 186, 700, 46, safeText(server), field == 0, false, false);

        ui.drawText(118, 268, "SHARE NAME", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 250, 700, 46, safeText(share), field == 1, false, false);

        ui.drawText(118, 332, "USERNAME", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 314, 700, 46, safeText(username, "Optional"), field == 2, false, false);

        ui.drawText(118, 396, "PASSWORD", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 378, 700, 46, maskedPassword(), field == 3, false, false);

        ui.drawText(118, 460, "MOUNT AT BOOT", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 442, 700, 46, mountAtBoot ? "YES" : "NO", field == 4, false, false);

        ui.drawButton(420, 520, 700, 54, "TEST CONNECTION", field == 5, false, false);

        if (!screenMessage.empty()) {
            ui.drawMessage(screenMessage);
        }
        ui.drawFooter("× EDIT/TOGGLE/TEST    □ SAVE & BACK    ○ CANCEL");
        ui.endFrame();

        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);
        if (down & HidNpadButton_Up) field = (field + 5) % 6;
        if (down & HidNpadButton_Down) field = (field + 1) % 6;

        if (down & HidNpadButton_B) {
            lastMessage = "CIFS config unchanged.";
            return;
        }

        if (down & HidNpadButton_X) {
            if (trim(server).empty()) {
                screenMessage = "Server IP is required.";
                continue;
            }
            if (trim(share).empty()) {
                screenMessage = "Share name is required.";
                continue;
            }

            std::string text =
                std::string("SERVER=\"") + server + "\"\n" +
                "SHARE=\"" + share + "\"\n" +
                "USERNAME=\"" + username + "\"\n" +
                "PASSWORD=\"" + password + "\"\n" +
                "LOCAL_DIR=\"cifs/games\"\n" +
                "WAIT_FOR_SERVER=\"true\"\n" +
                "MOUNT_AT_BOOT=\"" + std::string(mountAtBoot ? "true" : "false") + "\"\n" +
                "SINGLE_CIFS_CONNECTION=\"true\"\n";

            ui.beginFrame();
            ui.clear(UiRenderer::rgb(12, 10, 20));
            ui.drawCard(260, 240, 760, 180, "SAVING CIFS CONFIG");
            ui.drawText(310, 330, "PLEASE WAIT WHILE CONFIG IS SAVED", UiRenderer::rgb(248, 245, 255), 2);
            ui.endFrame();

            SshResult result = ssh.runCommand(std::string("mkdir -p /media/fat/Scripts && printf %s ") + shellQuote(text) + " > " + CifsConfigPath);
            if (result.success) {
                lastMessage = "CIFS config saved.";
            } else {
                lastMessage = result.error.empty() ? "CIFS config save failed." : result.error;
            }
            return;
        }

        if (down & HidNpadButton_A) {
            switch (field) {
                case 0:
                    editText("Server IP", server);
                    screenMessage.clear();
                    break;
                case 1:
                    editText("Share Name", share);
                    screenMessage.clear();
                    break;
                case 2:
                    editText("Username", username);
                    screenMessage.clear();
                    break;
                case 3:
                    editText("Password", password, true);
                    screenMessage.clear();
                    break;
                case 4:
                    mountAtBoot = !mountAtBoot;
                    screenMessage = mountAtBoot ? "Mount at boot enabled." : "Mount at boot disabled.";
                    break;
                case 5: {
                    if (trim(server).empty() || trim(share).empty()) {
                        screenMessage = "Server IP and Share Name are required.";
                        break;
                    }
                    ui.beginFrame();
                    ui.clear(UiRenderer::rgb(12, 10, 20));
                    ui.drawCard(260, 240, 760, 180, "TESTING CIFS CONNECTION");
                    ui.drawText(310, 330, "PLEASE WAIT WHILE CONNECTION IS TESTED", UiRenderer::rgb(248, 245, 255), 2);
                    ui.endFrame();

                    std::string unc = "//" + server + "/" + share;
                    std::string cmd =
                        "mkdir -p /tmp/cifs_test && "
                        "mount -t cifs " + shellQuote(unc) + " /tmp/cifs_test "
                        "-o username=" + shellQuote(username) + ",password=" + shellQuote(password) + " && "
                        "umount /tmp/cifs_test && echo SUCCESS";
                    SshResult test = ssh.runCommand(cmd);
                    screenMessage = test.success && contains(test.output, "SUCCESS")
                        ? "Connection successful."
                        : "Unable to connect to the network share.";
                    break;
                }
            }

            while (appletMainLoop()) {
                padUpdate(&pad);
                u64 held = padGetButtons(&pad);
                if ((held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X)) == 0) break;
            }
        }
    }

    lastMessage = "CIFS config unchanged.";
}

void App::configureDavBrowser() {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }

    SshResult existingResult = ssh.runCommand(std::string("cat ") + DavBrowserConfigPath + " 2>/dev/null || true");
    const std::string existing = existingResult.success ? existingResult.output : "";

    std::string serverUrl = simpleConfigValue(existing, "SERVER_URL");
    std::string username = simpleConfigValue(existing, "USERNAME");
    std::string password = simpleConfigValue(existing, "PASSWORD");
    std::string remotePath = simpleConfigValue(existing, "REMOTE_PATH");
    bool skipTls = simpleConfigValue(existing, "SKIP_TLS_VERIFY").empty()
        ? true
        : iniBoolValue(existing, "SKIP_TLS_VERIFY", true);

    std::string screenMessage;
    int field = 0;
    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 held = padGetButtons(&pad);
        if ((held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X)) == 0) break;
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.drawCard(260, 240, 760, 180, "DAV BROWSER CONFIG");
        ui.drawText(320, 330, "PREPARING CONFIGURATION SCREEN", UiRenderer::rgb(248, 245, 255), 2);
        ui.endFrame();
    }

    auto maskedPassword = [&]() -> std::string {
        if (password.empty()) return "Not set";
        return std::string(password.size() > 16 ? 16 : password.size(), '*');
    };

    while (appletMainLoop()) {
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, UiRenderer::Height, UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, 96, UiRenderer::rgb(20, 16, 34));
        ui.fillRect(0, 94, UiRenderer::Width, 4, UiRenderer::rgb(143, 84, 255));
        ui.drawText(42, 32, "DAV BROWSER CONFIG", UiRenderer::rgb(248, 245, 255), 3);
        ui.drawStatusPill(UiRenderer::Width - 250, 28, "CONFIG", true);

        ui.drawCard(80, 138, 1120, 470, "WEBDAV CONNECTION");
        ui.drawText(118, 204, "SERVER URL", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 186, 700, 46, safeText(serverUrl), field == 0, false, false);

        ui.drawText(118, 268, "USERNAME", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 250, 700, 46, safeText(username), field == 1, false, false);

        ui.drawText(118, 332, "PASSWORD", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 314, 700, 46, maskedPassword(), field == 2, false, false);

        ui.drawText(118, 396, "REMOTE PATH", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 378, 700, 46, safeText(remotePath, "Server root"), field == 3, false, false);

        ui.drawText(118, 460, "SKIP TLS VERIFY", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 442, 700, 46, boolText(skipTls), field == 4, false, false);

        if (!screenMessage.empty()) ui.drawMessage(screenMessage);
        ui.drawFooter("× EDIT/TOGGLE    □ SAVE & BACK    ○ CANCEL");
        ui.endFrame();

        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);
        if (down & HidNpadButton_Up) field = (field + 4) % 5;
        if (down & HidNpadButton_Down) field = (field + 1) % 5;

        if (down & HidNpadButton_B) {
            lastMessage = "DAV Browser config unchanged.";
            return;
        }

        if (down & HidNpadButton_X) {
            if (trim(serverUrl).empty()) {
                screenMessage = "Server URL is required.";
                continue;
            }
            if (trim(username).empty()) {
                screenMessage = "Username is required.";
                continue;
            }
            if (password.empty()) {
                screenMessage = "Password is required.";
                continue;
            }

            std::string text =
                "SERVER_URL=" + serverUrl + "\n" +
                "USERNAME=" + username + "\n" +
                "PASSWORD=" + password + "\n" +
                "REMOTE_PATH=" + remotePath + "\n" +
                "SKIP_TLS_VERIFY=" + std::string(skipTls ? "true" : "false") + "\n";

            ui.beginFrame();
            ui.clear(UiRenderer::rgb(12, 10, 20));
            ui.drawCard(260, 240, 760, 180, "SAVING DAV CONFIG");
            ui.drawText(310, 330, "PLEASE WAIT WHILE CONFIG IS SAVED", UiRenderer::rgb(248, 245, 255), 2);
            ui.endFrame();

            SshResult result = ssh.runCommand(writeTextCommand(DavBrowserConfigPath, text));
            lastMessage = result.success ? "DAV Browser config saved." : (result.error.empty() ? "DAV Browser config save failed." : result.error);
            return;
        }

        if (down & HidNpadButton_A) {
            switch (field) {
                case 0: editText("DAV Server URL", serverUrl); screenMessage.clear(); break;
                case 1: editText("DAV Username", username); screenMessage.clear(); break;
                case 2: editText("DAV Password", password, true); screenMessage.clear(); break;
                case 3: editText("DAV Remote Path", remotePath); screenMessage.clear(); break;
                case 4: skipTls = !skipTls; screenMessage = skipTls ? "TLS verification will be skipped." : "TLS verification enabled."; break;
            }

            while (appletMainLoop()) {
                padUpdate(&pad);
                u64 held = padGetButtons(&pad);
                if ((held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X)) == 0) break;
            }
        }
    }

    lastMessage = "DAV Browser config unchanged.";
}

void App::configureFtpSaveSync() {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }

    SshResult existingResult = ssh.runCommand(std::string("cat ") + FtpSaveSyncConfigPath + " 2>/dev/null || true");
    const std::string existing = existingResult.success ? existingResult.output : "";

    std::string protocol = simpleConfigValue(existing, "PROTOCOL");
    if (protocol.empty()) protocol = "sftp";
    protocol = toLower(protocol) == "ftp" ? "ftp" : "sftp";
    std::string host = simpleConfigValue(existing, "HOST");
    std::string port = simpleConfigValue(existing, "PORT");
    if (port.empty()) port = protocol == "sftp" ? "22" : "21";
    std::string username = simpleConfigValue(existing, "USERNAME");
    std::string password = simpleConfigValue(existing, "PASSWORD");
    std::string remoteBase = simpleConfigValue(existing, "REMOTE_BASE");
    if (remoteBase.empty()) remoteBase = "/";
    std::string deviceName = simpleConfigValue(existing, "DEVICE_NAME");
    bool syncSavestates = iniBoolValue(existing, "SYNC_SAVESTATES", false);

    std::string screenMessage;
    int field = 0;
    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 held = padGetButtons(&pad);
        if ((held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X)) == 0) break;
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.drawCard(260, 240, 760, 180, "FTP SAVE SYNC CONFIG");
        ui.drawText(320, 330, "PREPARING CONFIGURATION SCREEN", UiRenderer::rgb(248, 245, 255), 2);
        ui.endFrame();
    }

    auto maskedPassword = [&]() -> std::string {
        if (password.empty()) return "Not set";
        return std::string(password.size() > 16 ? 16 : password.size(), '*');
    };

    while (appletMainLoop()) {
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, UiRenderer::Height, UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, 96, UiRenderer::rgb(20, 16, 34));
        ui.fillRect(0, 94, UiRenderer::Width, 4, UiRenderer::rgb(143, 84, 255));
        ui.drawText(42, 32, "FTP SAVE SYNC CONFIG", UiRenderer::rgb(248, 245, 255), 3);
        ui.drawStatusPill(UiRenderer::Width - 250, 28, "CONFIG", true);

        ui.drawCard(80, 118, 1120, 510, "REMOTE SYNC CONNECTION");
        const int labelX = 118;
        const int buttonX = 420;
        const int startY = 172;
        const int stepY = 54;

        ui.drawText(labelX, startY + 12, "PROTOCOL", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(buttonX, startY, 700, 42, protocol == "sftp" ? "SFTP (recommended)" : "FTP", field == 0, false, false);
        ui.drawText(labelX, startY + stepY + 12, "HOST", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(buttonX, startY + stepY, 700, 42, safeText(host), field == 1, false, false);
        ui.drawText(labelX, startY + stepY * 2 + 12, "PORT", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(buttonX, startY + stepY * 2, 700, 42, safeText(port), field == 2, false, false);
        ui.drawText(labelX, startY + stepY * 3 + 12, "USERNAME", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(buttonX, startY + stepY * 3, 700, 42, safeText(username), field == 3, false, false);
        ui.drawText(labelX, startY + stepY * 4 + 12, "PASSWORD", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(buttonX, startY + stepY * 4, 700, 42, maskedPassword(), field == 4, false, false);
        ui.drawText(labelX, startY + stepY * 5 + 12, "REMOTE BASE", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(buttonX, startY + stepY * 5, 700, 42, safeText(remoteBase, "/"), field == 5, false, false);
        ui.drawText(labelX, startY + stepY * 6 + 12, "DEVICE NAME", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(buttonX, startY + stepY * 6, 700, 42, safeText(deviceName), field == 6, false, false);
        ui.drawText(labelX, startY + stepY * 7 + 12, "SYNC SAVESTATES", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(buttonX, startY + stepY * 7, 700, 42, boolText(syncSavestates), field == 7, false, false);

        if (syncSavestates) {
            ui.drawText(buttonX, 610, "Warning: savestates may cause issues with some cores or games.", UiRenderer::rgb(255, 200, 104), 1);
        }
        if (!screenMessage.empty()) ui.drawMessage(screenMessage);
        ui.drawFooter("× EDIT/TOGGLE    □ SAVE & BACK    ○ CANCEL");
        ui.endFrame();

        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);
        if (down & HidNpadButton_Up) field = (field + 7) % 8;
        if (down & HidNpadButton_Down) field = (field + 1) % 8;

        if (down & HidNpadButton_B) {
            lastMessage = "FTP Save Sync config unchanged.";
            return;
        }

        if (down & HidNpadButton_X) {
            if (trim(host).empty()) { screenMessage = "Host is required."; continue; }
            if (trim(port).empty()) { screenMessage = "Port is required."; continue; }
            bool portOk = true;
            for (char c : port) {
                if (!std::isdigit(static_cast<unsigned char>(c))) { portOk = false; break; }
            }
            if (!portOk) { screenMessage = "Port must be a number."; continue; }
            if (trim(username).empty()) { screenMessage = "Username is required."; continue; }
            if (password.empty()) { screenMessage = "Password is required."; continue; }
            if (trim(deviceName).empty()) { screenMessage = "Device Name is required."; continue; }
            if (trim(remoteBase).empty()) remoteBase = "/";
            if (!remoteBase.empty() && remoteBase.front() != '/') remoteBase = "/" + remoteBase;

            std::string text =
                "PROTOCOL=" + protocol + "\n" +
                "HOST=" + host + "\n" +
                "PORT=" + port + "\n" +
                "USERNAME=" + username + "\n" +
                "PASSWORD=" + password + "\n" +
                "REMOTE_BASE=" + remoteBase + "\n" +
                "DEVICE_NAME=" + deviceName + "\n\n" +
                "SYNC_SAVES=true\n" +
                "SYNC_SAVESTATES=" + std::string(syncSavestates ? "true" : "false") + "\n" +
                "SYNC_INTERVAL=15\n\n" +
                "SKIP_HOST_KEY_CHECK=true\n" +
                "SKIP_TLS_VERIFY=false\n" +
                "PAUSE_WHILE_CORE_RUNNING=true\n\n" +
                "MIN_AGE_SECONDS=5\n";

            ui.beginFrame();
            ui.clear(UiRenderer::rgb(12, 10, 20));
            ui.drawCard(260, 240, 760, 180, "SAVING FTP CONFIG");
            ui.drawText(310, 330, "PLEASE WAIT WHILE CONFIG IS SAVED", UiRenderer::rgb(248, 245, 255), 2);
            ui.endFrame();

            SshResult result = ssh.runCommand(writeTextCommand(FtpSaveSyncConfigPath, text));
            lastMessage = result.success ? "FTP Save Sync config saved." : (result.error.empty() ? "FTP Save Sync config save failed." : result.error);
            return;
        }

        if (down & HidNpadButton_A) {
            switch (field) {
                case 0: {
                    bool wasSftp = protocol == "sftp";
                    protocol = wasSftp ? "ftp" : "sftp";
                    if (protocol == "sftp" && (port.empty() || port == "21")) port = "22";
                    if (protocol == "ftp" && (port.empty() || port == "22")) port = "21";
                    screenMessage = protocol == "sftp" ? "Protocol set to SFTP." : "Protocol set to FTP.";
                    break;
                }
                case 1: editText("Host", host); screenMessage.clear(); break;
                case 2: editText("Port", port); screenMessage.clear(); break;
                case 3: editText("Username", username); screenMessage.clear(); break;
                case 4: editText("Password", password, true); screenMessage.clear(); break;
                case 5: editText("Remote Base", remoteBase); screenMessage.clear(); break;
                case 6: editText("Device Name", deviceName); screenMessage.clear(); break;
                case 7: syncSavestates = !syncSavestates; screenMessage = syncSavestates ? "Savestate sync enabled." : "Savestate sync disabled."; break;
            }

            while (appletMainLoop()) {
                padUpdate(&pad);
                u64 held = padGetButtons(&pad);
                if ((held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X)) == 0) break;
            }
        }
    }

    lastMessage = "FTP Save Sync config unchanged.";
}

void App::configureRaViewer() {
    if (!ssh.isConnected()) {
        lastMessage = "No active MiSTer connection.";
        return;
    }

    SshResult existingResult = ssh.runCommand(std::string("cat ") + RaViewerConfigPath + " 2>/dev/null || true");
    const std::string existing = existingResult.success ? existingResult.output : "";

    std::string username = simpleConfigValue(existing, "username");
    std::string apiKey = simpleConfigValue(existing, "api_key");
    std::string screenMessage;
    int field = 0;
    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 held = padGetButtons(&pad);
        if ((held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X)) == 0) break;
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.drawCard(260, 240, 760, 180, "RA VIEWER CONFIG");
        ui.drawText(320, 330, "PREPARING CONFIGURATION SCREEN", UiRenderer::rgb(248, 245, 255), 2);
        ui.endFrame();
    }

    auto maskedApiKey = [&]() -> std::string {
        if (apiKey.empty()) return "Not set";
        return std::string(apiKey.size() > 20 ? 20 : apiKey.size(), '*');
    };

    while (appletMainLoop()) {
        ui.beginFrame();
        ui.clear(UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, UiRenderer::Height, UiRenderer::rgb(12, 10, 20));
        ui.fillRect(0, 0, UiRenderer::Width, 96, UiRenderer::rgb(20, 16, 34));
        ui.fillRect(0, 94, UiRenderer::Width, 4, UiRenderer::rgb(143, 84, 255));
        ui.drawText(42, 32, "RA VIEWER CONFIG", UiRenderer::rgb(248, 245, 255), 3);
        ui.drawStatusPill(UiRenderer::Width - 250, 28, "CONFIG", true);

        ui.drawCard(80, 170, 1120, 340, "RETROACHIEVEMENTS LOGIN");
        ui.drawText(118, 250, "USERNAME", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 232, 700, 46, safeText(username), field == 0, false, false);
        ui.drawText(118, 314, "API KEY", UiRenderer::rgb(174, 154, 218), 2);
        ui.drawButton(420, 296, 700, 46, maskedApiKey(), field == 1, false, false);

        if (!screenMessage.empty()) ui.drawMessage(screenMessage);
        ui.drawFooter("× EDIT    □ SAVE & BACK    ○ CANCEL");
        ui.endFrame();

        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);
        if (down & HidNpadButton_Up) field = (field + 1) % 2;
        if (down & HidNpadButton_Down) field = (field + 1) % 2;

        if (down & HidNpadButton_B) {
            lastMessage = "RA Viewer config unchanged.";
            return;
        }

        if (down & HidNpadButton_X) {
            if (trim(username).empty()) { screenMessage = "Username is required."; continue; }
            if (trim(apiKey).empty()) { screenMessage = "API Key is required."; continue; }

            std::string text =
                std::string("[retroachievements]\n") +
                "username = " + username + "\n" +
                "api_key = " + apiKey + "\n";

            ui.beginFrame();
            ui.clear(UiRenderer::rgb(12, 10, 20));
            ui.drawCard(260, 240, 760, 180, "SAVING RA VIEWER CONFIG");
            ui.drawText(300, 330, "PLEASE WAIT WHILE CONFIG IS SAVED", UiRenderer::rgb(248, 245, 255), 2);
            ui.endFrame();

            SshResult result = ssh.runCommand(writeTextCommand(RaViewerConfigPath, text));
            lastMessage = result.success ? "RA Viewer config saved." : (result.error.empty() ? "RA Viewer config save failed." : result.error);
            return;
        }

        if (down & HidNpadButton_A) {
            if (field == 0) {
                editText("RA Username", username);
            } else {
                editText("RA API Key", apiKey, true);
            }
            screenMessage.clear();

            while (appletMainLoop()) {
                padUpdate(&pad);
                u64 held = padGetButtons(&pad);
                if ((held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X)) == 0) break;
            }
        }
    }

    lastMessage = "RA Viewer config unchanged.";
}

void App::executeScriptAction(ScriptId id, int actionIndex) {
    std::vector<std::string> actions = scriptActions(id);
    if (actionIndex < 0 || actionIndex >= static_cast<int>(actions.size())) return;
    const std::string action = actions[actionIndex];

    switch (id) {
        case ScriptId::UpdateAll:
            if (action == "INSTALL") {
                const std::string command =
                    "mkdir -p /media/fat/Scripts /media/fat/Scripts/.config/update_all && "
                    "URL=https://raw.githubusercontent.com/theypsilon/Update_All_MiSTer/master/update_all.sh; "
                    "TARGET=/media/fat/Scripts/update_all.sh; "
                    "TMP=/tmp/mc_update_all.sh; "
                    "WGET_LOG=/tmp/mc_update_all_wget.log; "
                    "rm -f \"$TMP\" \"$WGET_LOG\"; "
                    "echo Downloading update_all launcher...; "
                    "if ! command -v wget >/dev/null 2>&1; then echo wget not found.; exit 1; fi; "
                    "if ! wget --no-check-certificate -O \"$TMP\" \"$URL\" >\"$WGET_LOG\" 2>&1; then "
                    "echo Unable to download update_all launcher.; "
                    "cat \"$WGET_LOG\"; "
                    "exit 1; "
                    "fi; "
                    "if [ ! -s \"$TMP\" ]; then "
                    "echo Downloaded update_all launcher is empty.; "
                    "cat \"$WGET_LOG\"; "
                    "exit 1; "
                    "fi; "
                    "mv \"$TMP\" \"$TARGET\" && "
                    "chmod +x \"$TARGET\" && "
                    "rm -f \"$WGET_LOG\" && "
                    "echo Installation complete.";

                showStreamingCommandWindow("INSTALL UPDATE ALL", command, "Update All installed.", "Update All install failed.");
                refreshCurrentScriptStatus();
            } else if (action == "CONFIGURE") {
                configureUpdateAll();
                refreshCurrentScriptStatus();
            } else if (action == "RUN") {
                if (!confirm("RUN UPDATE ALL", "ARE YOU SURE?")) return;
                bool rebootDetected = false;
                const bool updateAllSuccess = showStreamingCommandWindow("UPDATE ALL", "/media/fat/Scripts/update_all.sh", "Update All complete.", "Update All failed.", &rebootDetected);

                bool connectionLostAfterRun = false;
                if (rebootDetected || !updateAllSuccess) {
                    SshResult probe = ssh.runCommand("printf MC_ALIVE");
                    connectionLostAfterRun = !probe.success || trim(probe.output) != "MC_ALIVE";
                }

                if (rebootDetected || connectionLostAfterRun) {
                    waitForReconnectAfterReboot("UPDATE ALL", "Reboot detected. Waiting for MiSTer...");
                }
                if (ssh.isConnected()) refreshCurrentScriptStatus();
            } else if (action == "UNINSTALL") {
                scriptUninstall("UPDATE ALL", "rm -f /media/fat/Scripts/update_all.sh");
                refreshCurrentScriptStatus();
            }
            break;
        case ScriptId::Zaparoo:
            if (action == "INSTALL") {
                const std::string command =
                    "mkdir -p /media/fat/Scripts /tmp/mc_zap && "
                    "rm -rf /tmp/mc_zap/* && "
                    "if ! command -v wget >/dev/null 2>&1; then echo wget not found.; exit 1; fi; "
                    "echo Finding latest Zaparoo release...; "
                    "JSON=/tmp/mc_zap/release.json; "
                    "wget --no-check-certificate --header='User-Agent: MiSTer-Companion-Vita' -O \"$JSON\" https://api.github.com/repos/ZaparooProject/zaparoo-core/releases/latest || exit 1; "
                    "URL=$(sed -n 's/.*\\\"browser_download_url\\\": *\\\"\\([^\\\"]*mister_arm[^\\\"]*\\.zip\\)\\\".*/\1/p' \"$JSON\" | head -1); "
                    "if [ -z \"$URL\" ]; then echo Unable to find MiSTer Zaparoo release.; exit 1; fi; "
                    "echo Downloading Zaparoo package...; "
                    "wget --no-check-certificate -O /tmp/mc_zap/zaparoo.zip \"$URL\" || exit 1; "
                    "if [ ! -s /tmp/mc_zap/zaparoo.zip ]; then echo Zaparoo download is empty.; exit 1; fi; "
                    "echo Extracting Zaparoo package...; "
                    "unzip -o /tmp/mc_zap/zaparoo.zip -d /tmp/mc_zap >/dev/null || exit 1; "
                    "SCRIPT=$(find /tmp/mc_zap -name zaparoo.sh | head -1); "
                    "if [ -z \"$SCRIPT\" ]; then echo Could not find zaparoo.sh inside release ZIP.; exit 1; fi; "
                    "cp \"$SCRIPT\" /media/fat/Scripts/zaparoo.sh && "
                    "chmod +x /media/fat/Scripts/zaparoo.sh && "
                    "echo Installation complete.";
                showStreamingCommandWindow("INSTALL ZAPAROO", command, "Zaparoo installed.", "Zaparoo install failed.");
                refreshCurrentScriptStatus();
            }
            else if (action == "ENABLE START ON BOOT") runScriptCommand("ENABLE ZAPAROO", "mkdir -p /media/fat/linux; test -f /media/fat/linux/user-startup.sh || printf '#!/bin/sh\\n' > /media/fat/linux/user-startup.sh; grep -F 'mrext/zaparoo' /media/fat/linux/user-startup.sh >/dev/null 2>&1 || printf '\\n# mrext/zaparoo\\n[[ -e /media/fat/Scripts/zaparoo.sh ]] && /media/fat/Scripts/zaparoo.sh -service $1\\n' >> /media/fat/linux/user-startup.sh; chmod +x /media/fat/linux/user-startup.sh");
            else if (action == "DISABLE START ON BOOT") runScriptCommand("DISABLE ZAPAROO", "sed -i '/mrext\\/zaparoo/d;/zaparoo.sh -service/d' /media/fat/linux/user-startup.sh 2>/dev/null || true");
            else if (action == "UNINSTALL") scriptUninstall("ZAPAROO", "rm -f /media/fat/Scripts/zaparoo.sh");
            break;
        case ScriptId::MigrateSd:
            if (action == "INSTALL") scriptInstall("MIGRATE SD", MigrateSdPath, UrlMigrateSd);
            else if (action == "UNINSTALL") scriptUninstall("MIGRATE SD", "rm -f /media/fat/Scripts/migrate_sd.sh");
            break;
        case ScriptId::CifsMount:
            if (action == "INSTALL") showStreamingCommandWindow("INSTALL CIFS", downloadCommand(UrlCifsMount, CifsMountPath) + " && " + downloadCommand(UrlCifsUmount, CifsUmountPath), "CIFS installed.", "CIFS install failed.");
            else if (action == "CONFIGURE") configureCifsMount();
            else if (action == "MOUNT") runScriptCommand("MOUNT CIFS", CifsMountPath);
            else if (action == "UNMOUNT") runScriptCommand("UNMOUNT CIFS", CifsUmountPath);
            else if (action == "REMOVE CONFIG") runScriptCommand("REMOVE CIFS CONFIG", "rm -f /media/fat/Scripts/cifs_mount.ini", true);
            else if (action == "UNINSTALL") scriptUninstall("CIFS", "rm -f /media/fat/Scripts/cifs_mount.sh /media/fat/Scripts/cifs_umount.sh");
            break;
        case ScriptId::AutoTime:
            if (action == "INSTALL") scriptInstall("AUTO TIME", AutoTimePath, UrlAutoTime);
            else if (action == "UNINSTALL") scriptUninstall("AUTO TIME", "rm -f /media/fat/Scripts/auto_time.sh");
            break;
        case ScriptId::CdGameOrganizer:
            if (action == "INSTALL") scriptInstall("CD GAME ORGANIZER", CdGameOrganizerPath, UrlCdGameOrganizer);
            else if (action == "UNINSTALL") scriptUninstall("CD GAME ORGANIZER", "rm -f /media/fat/Scripts/cd_game_organizer.sh");
            break;
        case ScriptId::DavBrowser:
            if (action == "INSTALL") scriptInstall("DAV BROWSER", DavBrowserPath, UrlDavBrowser);
            else if (action == "CONFIGURE") configureDavBrowser();
            else if (action == "REMOVE CONFIG") runScriptCommand("REMOVE DAV CONFIG", "rm -f /media/fat/Scripts/.config/dav_browser/dav_browser.ini", true);
            else if (action == "UNINSTALL") scriptUninstall("DAV BROWSER", "rm -f /media/fat/Scripts/dav_browser.sh; rm -rf /media/fat/Scripts/.config/dav_browser");
            break;
        case ScriptId::FtpSaveSync:
            if (action == "INSTALL") scriptInstall("FTP SAVE SYNC", FtpSaveSyncPath, UrlFtpSaveSync);
            else if (action == "CONFIGURE") configureFtpSaveSync();
            else if (action == "ENABLE START ON BOOT") runScriptCommand("ENABLE FTP SAVE SYNC", "mkdir -p /media/fat/linux; test -f /media/fat/linux/user-startup.sh || printf '#!/bin/sh\\n' > /media/fat/linux/user-startup.sh; grep -F 'ftp_save_sync_daemon.sh' /media/fat/linux/user-startup.sh >/dev/null 2>&1 || printf '\\n# ftp_save_sync START\\n/media/fat/Scripts/.config/ftp_save_sync/ftp_save_sync_daemon.sh >/dev/null 2>&1\\n# ftp_save_sync END\\n' >> /media/fat/linux/user-startup.sh; chmod +x /media/fat/linux/user-startup.sh");
            else if (action == "DISABLE START ON BOOT") runScriptCommand("DISABLE FTP SAVE SYNC", "sed -i '/ftp_save_sync START/,/ftp_save_sync END/d' /media/fat/linux/user-startup.sh 2>/dev/null || true");
            else if (action == "REMOVE CONFIG") runScriptCommand("REMOVE FTP CONFIG", "rm -f /media/fat/Scripts/.config/ftp_save_sync/ftp_save_sync.ini", true);
            else if (action == "UNINSTALL") scriptUninstall("FTP SAVE SYNC", "sed -i '/ftp_save_sync START/,/ftp_save_sync END/d' /media/fat/linux/user-startup.sh 2>/dev/null || true; rm -f /media/fat/Scripts/ftp_save_sync.sh; rm -rf /media/fat/Scripts/.config/ftp_save_sync");
            break;
        case ScriptId::StaticWallpaper:
            if (action == "INSTALL") scriptInstall("STATIC WALLPAPER", StaticWallpaperPath, UrlStaticWallpaper);
            else if (action == "UNINSTALL") scriptUninstall("STATIC WALLPAPER", "rm -f /media/fat/Scripts/static_wallpaper.sh; rm -rf /media/fat/Scripts/.config/static_wallpaper; rm -f /media/fat/menu.jpg /media/fat/menu.png");
            break;
        case ScriptId::Syncthing:
            if (action == "INSTALL") scriptInstall("SYNCTHING", SyncthingPath, UrlSyncthingScript);
            else if (action == "ENABLE START ON BOOT") runScriptCommand("ENABLE SYNCTHING", "mkdir -p /media/fat/linux; test -f /media/fat/linux/user-startup.sh || printf '#!/bin/sh\\n' > /media/fat/linux/user-startup.sh; grep -F 'syncthing_service.sh' /media/fat/linux/user-startup.sh >/dev/null 2>&1 || printf '\\n# Start Syncthing\\n/media/fat/Scripts/.config/syncthing/syncthing_service.sh start &\\n' >> /media/fat/linux/user-startup.sh; chmod +x /media/fat/linux/user-startup.sh");
            else if (action == "DISABLE START ON BOOT") runScriptCommand("DISABLE SYNCTHING", "sed -i '/Start Syncthing/d;/syncthing_service.sh/d' /media/fat/linux/user-startup.sh 2>/dev/null || true");
            else if (action == "UNINSTALL") scriptUninstall("SYNCTHING", "rm -f /media/fat/Scripts/syncthing.sh; rm -rf /media/fat/Scripts/.config/syncthing");
            break;
        case ScriptId::RaViewer:
            if (action == "INSTALL") scriptInstall("RA VIEWER", RaViewerPath, UrlRaViewer);
            else if (action == "EDIT CONFIG") configureRaViewer();
            else if (action == "UNINSTALL") scriptUninstall("RA VIEWER", "rm -f /media/fat/Scripts/ra_viewer.sh; rm -rf /media/fat/Scripts/.config/ra_viewer");
            break;
    }
}



std::string App::formatDfLine(const std::string& line) {
    std::vector<std::string> parts = splitWords(line);
    if (parts.size() < 5) return trim(line.empty() ? "Unknown" : line);
    return parts[3] + " free of " + parts[1] + " (" + parts[4] + " used)";
}

std::string App::normalizeCoreName(std::string core) {
    core = trim(core);
    if (core.empty()) return "Unknown";
    std::replace(core.begin(), core.end(), '_', ' ');

    std::string compact;
    for (char c : core) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) compact.push_back(::toupper(c));
    }

    if (compact == "TGFX16") return "TurboGrafx-16";
    if (compact == "PCECD") return "PC Engine CD";
    if (compact == "SMS") return "Master System";
    if (compact == "GENESIS") return "Genesis";
    if (compact == "MEGADRIVE") return "Mega Drive";
    if (compact == "NES") return "NES";
    if (compact == "SNES") return "SNES";
    if (compact == "GBA") return "Game Boy Advance";
    if (compact == "GBC") return "Game Boy Color";
    if (compact == "GB") return "Game Boy";
    if (compact == "PSX") return "PlayStation";
    if (compact == "AO486") return "ao486";

    return core;
}

std::string App::prettifyGameName(const std::string& path) {
    std::string value = trim(path);
    while (!value.empty() && (value.back() == '/' || value.back() == '\\')) value.pop_back();

    size_t slash = value.find_last_of("/\\");
    std::string name = slash == std::string::npos ? value : value.substr(slash + 1);
    size_t dot = name.find_last_of('.');
    if (dot != std::string::npos) name = name.substr(0, dot);

    const std::vector<std::string> tokens = {"(Disc 1)", "(Disc 2)", "(Disc 3)", "(USA)", "(Europe)", "(Japan)", "[!]", "_"};
    for (const std::string& token : tokens) {
        size_t pos;
        while ((pos = name.find(token)) != std::string::npos) {
            name.replace(pos, token.size(), token == "_" ? " " : "");
        }
    }

    return trim(name.empty() ? "Unknown" : name);
}
