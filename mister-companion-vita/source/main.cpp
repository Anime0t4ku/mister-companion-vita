#include <psp2/kernel/processmgr.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/sysmodule.h>
#include <psp2/apputil.h>
#include <psp2/common_dialog.h>
#include <libssh2.h>
#include <cstring>

#include "app.hpp"

static unsigned char netMemory[4 * 1024 * 1024];

static void vitaNetworkInit() {
    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);

    SceNetInitParam netInitParam{};
    netInitParam.memory = netMemory;
    netInitParam.size = sizeof(netMemory);
    netInitParam.flags = 0;

    sceNetInit(&netInitParam);
    sceNetCtlInit();
}

static void vitaNetworkExit() {
    sceNetCtlTerm();
    sceNetTerm();
    sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);
}

int main(int argc, char* argv[]) {
    SceAppUtilInitParam appUtilInit{};
    SceAppUtilBootParam appUtilBoot{};
    sceAppUtilInit(&appUtilInit, &appUtilBoot);

    SceCommonDialogConfigParam commonDialogConfig{};
    sceCommonDialogSetConfigParam(&commonDialogConfig);

    vitaNetworkInit();
    libssh2_init(0);

    App app;
    app.run();

    libssh2_exit();
    vitaNetworkExit();
    sceAppUtilShutdown();

    sceKernelExitProcess(0);
    return 0;
}