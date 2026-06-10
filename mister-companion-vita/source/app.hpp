#pragma once

#include <switch.h>

#include "config.hpp"
#include "mister_ini.hpp"
#include "remote_client.hpp"
#include "ssh_client.hpp"
#include "ui.hpp"

#include <string>
#include <vector>

class App {
public:
    void run();

private:
    enum class Tab {
        Connection,
        Device,
        Remote,
        Scripts,
        Settings,
        Wallpapers,
        Extras,
    };

    enum class ScriptId {
        UpdateAll,
        Zaparoo,
        MigrateSd,
        CifsMount,
        AutoTime,
        CdGameOrganizer,
        DavBrowser,
        FtpSaveSync,
        StaticWallpaper,
        Syncthing,
        RaViewer,
    };

    enum class ExtraId {
        ZaparooFrontend,
        RetroAchievementCores,
        Mms2GbCore,
    };

    AppConfig config;
    SshClient ssh;
    RemoteClient remote;
    UiRenderer ui;
    Tab tab = Tab::Connection;
    int selected = 0;
    bool connectionProfileMode = false;
    std::string activeProfileName;
    int selectedProfile = 0;
    int profileScroll = 0;
    std::string status = "Disconnected";
    std::string lastMessage;
    std::string sdStorage = "Not refreshed";
    std::string usbStorage = "Not refreshed";
    std::string smbStatus = "Not refreshed";
    std::string nowPlaying;
    std::string remoteInstalled = "Not checked";
    std::string remoteRunning = "Not checked";
    std::string remoteStartup = "Not checked";
    std::string remoteInstalledVersion = "Unknown";
    std::string remoteLatestVersion = "Unknown";
    bool remoteUpdateAvailable = false;
    bool passthroughActive = false;
    int selectedScript = 0;
    std::vector<std::vector<std::string>> cachedScriptStatus;
    int selectedWallpaperSource = 0;
    int selectedWallpaperAction = 0;
    std::vector<std::string> cachedWallpaperStatus;
    std::vector<int> wallpaperPackTotal;
    std::vector<int> wallpaperPackInstalled;
    std::vector<int> wallpaperPackMissing;
    int wallpaperInstalledTotal = 0;
    int wallpaperMissingTotal = 0;
    int selectedExtra = 0;
    std::vector<std::vector<std::string>> cachedExtraStatus;
    bool extraUpdateAvailable[3] = {false, false, false};
    std::string extraLatestVersion[3];
    bool extraUpdateCheckPending = false;
    int extraUpdateCheckIndex = -1;
    std::vector<std::string> settingsIniFiles;
    std::vector<std::string> settingsFonts;
    std::string selectedSettingsIni = "MiSTer.ini";
    std::string settingsIniText;
    MisterIniEasyValues settingsEasy;
    bool settingsLoaded = false;
    bool settingsDirty = false;
    int selectedSettings = 0;
    int settingsScroll = 0;

    void draw();
    void drawHeader();
    void drawConnection();
    void drawDevice();
    void drawRemote();
    void drawScripts();
    void drawSettings();
    void drawWallpapers();
    void drawExtras();
    void drawPassthrough();

    void handleInput(u64 buttons);
    void handleConnectionInput(u64 buttons);
    void handleDeviceInput(u64 buttons);
    void handleRemoteInput(u64 buttons);
    void handleScriptsInput(u64 buttons);
    void handleSettingsInput(u64 buttons);
    void handleWallpapersInput(u64 buttons);
    void handleExtrasInput(u64 buttons);
    void handlePassthroughInput(u64 down, u64 up, u64 held);

    void editText(const char* title, std::string& value, bool password = false);
    bool confirm(const char* title, const char* body);

    void saveConfig();
    void connectOrDisconnect();
    bool hasCompleteManualConnection() const;
    void saveManualAsProfile();
    void connectProfile(int index);
    int profileIndexForHost(const std::string& host) const;
    bool loadProfileForHost(const std::string& host);
    void clearActiveProfile();
    void editProfile(int index);
    void deleteProfile(int index);
    void showMisterScanWindow();
    void connectScannedHost(const std::string& host, bool saveAsProfile);

    void refreshDevice();
    void refreshStorage();
    void refreshSmb();
    void refreshNowPlaying();

    void toggleSmb();
    void reboot();
    bool sendSoftRebootCommand();
    void waitForReconnectAfterReboot(const std::string& title, const std::string& firstLine);
    void softRebootAndReconnect(const std::string& title);
    void returnToMenu();

    void refreshRemoteStatus();
    void installRemoteDaemon();
    void startRemoteDaemon();
    void stopRemoteDaemon();
    void toggleRemoteStartup();
    void uninstallRemoteDaemon();
    void startPassthrough();
    void stopPassthrough();
    void sendPassthroughButton(u64 mask, u64 buttons, const std::string& control, const std::string& name, const std::string& action);

    std::string scriptTitle(ScriptId id) const;
    std::vector<std::string> scriptActions(ScriptId id);
    std::vector<std::string> scriptStatus(ScriptId id);
    void refreshCurrentScriptStatus();
    void executeScriptAction(ScriptId id, int actionIndex);
    void configureUpdateAll();
    void configureCifsMount();
    void configureDavBrowser();
    void configureFtpSaveSync();
    void configureRaViewer();
    void scriptInstall(const std::string& title, const std::string& path, const std::string& url);
    void scriptUninstall(const std::string& title, const std::string& command);
    void runScriptCommand(const std::string& title, const std::string& command, bool confirmFirst = false);
    void showOutputWindow(const std::string& title, const std::string& output);
    bool showStreamingCommandWindow(const std::string& title, const std::string& command, const std::string& successMessage, const std::string& failureMessage, bool* rebootDetected = nullptr);
    bool askField(const char* title, std::string& value, bool password = false);
    bool askYesNo(const char* title, const char* body, bool defaultYes = false);
    void ensureScriptsDirs();

    std::string wallpaperSourceTitle(int index) const;
    std::vector<std::string> wallpaperActions(int index) const;
    void refreshWallpaperStatus();
    void executeWallpaperAction(int actionIndex);
    void installWallpaperPack(const std::string& title, const std::string& dbUrl, const std::string& rawBase, const std::string& filterMode);
    void removeWallpaperPack(const std::string& title, const std::string& dbUrl, const std::string& rawBase, const std::string& filterMode);
    void showStaticWallpaperMenu();

    std::string extraTitle(ExtraId id) const;
    std::vector<std::string> extraActions(ExtraId id) const;
    std::vector<std::string> extraStatus(ExtraId id);
    void refreshCurrentExtraStatus(bool checkLatest = false);
    void performPendingExtraUpdateCheck();
    void executeExtraAction(ExtraId id, int actionIndex);
    void installOrUpdateZaparooFrontend();
    void enableZaparooFrontend();
    void uninstallZaparooFrontend();
    void installOrUpdateRaCores(bool updateOnly = false);
    void uninstallRaCores();
    void configureRaCores();
    void installOrUpdateMms2GbCore();
    void uninstallMms2GbCore();

    void loadSettingsTab(bool force = false);
    bool ensureRemoteMisterIni();
    void scanSettingsIniFiles();
    void scanSettingsFonts();
    void loadSelectedSettingsIni();
    void saveSettingsIni();
    void restoreSettingsDefaults();
    void cycleSettingsValue(int index);
    std::string settingsLabel(int index) const;
    std::string settingsValue(int index) const;
    int settingsOptionCount() const;
    std::vector<std::string> settingsOptionsForIndex(int index) const;
    std::string remoteReadTextFile(const std::string& path);
    bool remoteWriteTextFile(const std::string& path, const std::string& text, std::string& error);

    std::string runCommandMessage(const std::string& command);
    std::string formatDfLine(const std::string& line);
    std::string prettifyGameName(const std::string& path);
    std::string normalizeCoreName(std::string core);
};
