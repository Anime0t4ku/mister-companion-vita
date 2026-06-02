#pragma once

#include <cstdint>
#include <psp2/ctrl.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>

using u8 = uint8_t;
using u32 = uint32_t;
using u64 = uint64_t;
using Result = int;

#define R_FAILED(rc) ((rc) < 0)
#define R_SUCCEEDED(rc) ((rc) >= 0)
#define RGBA8(r, g, b, a) ((u32)((((u32)(a)) << 24) | (((u32)(b)) << 16) | (((u32)(g)) << 8) | ((u32)(r))))

static constexpr u64 HidNpadButton_A      = 1ULL << 0;
static constexpr u64 HidNpadButton_B      = 1ULL << 1;
static constexpr u64 HidNpadButton_X      = 1ULL << 2;
static constexpr u64 HidNpadButton_Y      = 1ULL << 3;
static constexpr u64 HidNpadButton_L      = 1ULL << 4;
static constexpr u64 HidNpadButton_R      = 1ULL << 5;
static constexpr u64 HidNpadButton_ZL     = 1ULL << 6;
static constexpr u64 HidNpadButton_ZR     = 1ULL << 7;
static constexpr u64 HidNpadButton_Plus   = 1ULL << 8;
static constexpr u64 HidNpadButton_Minus  = 1ULL << 9;
static constexpr u64 HidNpadButton_Up     = 1ULL << 10;
static constexpr u64 HidNpadButton_Down   = 1ULL << 11;
static constexpr u64 HidNpadButton_Left   = 1ULL << 12;
static constexpr u64 HidNpadButton_Right  = 1ULL << 13;
static constexpr u64 HidNpadButton_StickL = 1ULL << 14;
static constexpr u64 HidNpadButton_StickR = 1ULL << 15;

static constexpr int HidNpadStyleSet_NpadStandard = 0;

struct PadState {
    u64 current = 0;
    u64 previous = 0;
};

static inline u64 vitaButtonsToNxMask(uint32_t buttons) {
    u64 out = 0;
    if (buttons & SCE_CTRL_CROSS) out |= HidNpadButton_A;
    if (buttons & SCE_CTRL_CIRCLE) out |= HidNpadButton_B;
    if (buttons & SCE_CTRL_SQUARE) out |= HidNpadButton_X;
    if (buttons & SCE_CTRL_TRIANGLE) out |= HidNpadButton_Y;
    if (buttons & SCE_CTRL_LTRIGGER) out |= HidNpadButton_L;
    if (buttons & SCE_CTRL_RTRIGGER) out |= HidNpadButton_R;
    if (buttons & SCE_CTRL_START) out |= HidNpadButton_Plus;
    if (buttons & SCE_CTRL_SELECT) out |= HidNpadButton_Minus;
    if (buttons & SCE_CTRL_UP) out |= HidNpadButton_Up;
    if (buttons & SCE_CTRL_DOWN) out |= HidNpadButton_Down;
    if (buttons & SCE_CTRL_LEFT) out |= HidNpadButton_Left;
    if (buttons & SCE_CTRL_RIGHT) out |= HidNpadButton_Right;
    return out;
}

static inline void padConfigureInput(int, int) {}

static inline void padInitializeDefault(PadState* pad) {
    if (!pad) return;
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_DIGITAL);
    pad->current = 0;
    pad->previous = 0;
}

static inline void padUpdate(PadState* pad) {
    if (!pad) return;
    SceCtrlData data{};
    sceCtrlPeekBufferPositive(0, &data, 1);
    pad->previous = pad->current;
    pad->current = vitaButtonsToNxMask(data.buttons);
}

static inline u64 padGetButtons(const PadState* pad) {
    return pad ? pad->current : 0;
}

static inline u64 padGetButtonsDown(const PadState* pad) {
    return pad ? (pad->current & ~pad->previous) : 0;
}

static inline u64 padGetButtonsUp(const PadState* pad) {
    return pad ? (~pad->current & pad->previous) : 0;
}

static inline bool appletMainLoop() {
    return true;
}

static inline void svcSleepThread(int64_t nanoseconds) {
    sceKernelDelayThread(static_cast<unsigned int>(nanoseconds / 1000));
}
