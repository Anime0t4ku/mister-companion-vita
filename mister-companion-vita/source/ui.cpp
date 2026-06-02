#include "ui.hpp"

#include <vita2d.h>

#include <algorithm>
#include <cctype>
#include <cmath>

namespace {
constexpr u32 Transparent = 0;
}

u32 UiRenderer::rgb(u8 r, u8 g, u8 b) {
    return RGBA8(r, g, b, 255);
}

int UiRenderer::sx(int value) const {
    return static_cast<int>(std::round(value * Scale + OffsetX));
}

int UiRenderer::sy(int value) const {
    return static_cast<int>(std::round(value * Scale + OffsetY));
}

int UiRenderer::sw(int value) const {
    return std::max(1, static_cast<int>(std::round(value * Scale)));
}

bool UiRenderer::initialize() {
    vita2d_init();
    vita2d_set_clear_color(rgb(14, 12, 22));
    ready = true;
    return true;
}

void UiRenderer::shutdown() {
    if (!ready) return;
    vita2d_fini();
    ready = false;
}

void UiRenderer::beginFrame() {
    if (!ready) return;
    vita2d_start_drawing();
}

void UiRenderer::endFrame() {
    if (!ready) return;
    vita2d_end_drawing();
    vita2d_swap_buffers();
}

void UiRenderer::clear(u32 color) {
    vita2d_set_clear_color(color);
    vita2d_clear_screen();
}

void UiRenderer::putPixel(int x, int y, u32 color) {
    if (!ready || x < 0 || y < 0 || x >= Width || y >= Height) return;
    vita2d_draw_rectangle(sx(x), sy(y), 1.0f, 1.0f, color);
}

void UiRenderer::fillRect(int x, int y, int w, int h, u32 color) {
    if (!ready || w <= 0 || h <= 0) return;
    vita2d_draw_rectangle(static_cast<float>(sx(x)), static_cast<float>(sy(y)), static_cast<float>(sw(w)), static_cast<float>(sw(h)), color);
}

void UiRenderer::drawRect(int x, int y, int w, int h, u32 color, int thickness) {
    fillRect(x, y, w, thickness, color);
    fillRect(x, y + h - thickness, w, thickness, color);
    fillRect(x, y, thickness, h, color);
    fillRect(x + w - thickness, y, thickness, h, color);
}

const char* UiRenderer::glyph(char c) const {
    switch (c) {
        case 'A': return "01110" "10001" "10001" "11111" "10001" "10001" "10001";
        case 'B': return "11110" "10001" "10001" "11110" "10001" "10001" "11110";
        case 'C': return "01111" "10000" "10000" "10000" "10000" "10000" "01111";
        case 'D': return "11110" "10001" "10001" "10001" "10001" "10001" "11110";
        case 'E': return "11111" "10000" "10000" "11110" "10000" "10000" "11111";
        case 'F': return "11111" "10000" "10000" "11110" "10000" "10000" "10000";
        case 'G': return "01111" "10000" "10000" "10011" "10001" "10001" "01111";
        case 'H': return "10001" "10001" "10001" "11111" "10001" "10001" "10001";
        case 'I': return "11111" "00100" "00100" "00100" "00100" "00100" "11111";
        case 'J': return "00111" "00010" "00010" "00010" "10010" "10010" "01100";
        case 'K': return "10001" "10010" "10100" "11000" "10100" "10010" "10001";
        case 'L': return "10000" "10000" "10000" "10000" "10000" "10000" "11111";
        case 'M': return "10001" "11011" "10101" "10101" "10001" "10001" "10001";
        case 'N': return "10001" "11001" "10101" "10011" "10001" "10001" "10001";
        case 'O': return "01110" "10001" "10001" "10001" "10001" "10001" "01110";
        case 'P': return "11110" "10001" "10001" "11110" "10000" "10000" "10000";
        case 'Q': return "01110" "10001" "10001" "10001" "10101" "10010" "01101";
        case 'R': return "11110" "10001" "10001" "11110" "10100" "10010" "10001";
        case 'S': return "01111" "10000" "10000" "01110" "00001" "00001" "11110";
        case 'T': return "11111" "00100" "00100" "00100" "00100" "00100" "00100";
        case 'U': return "10001" "10001" "10001" "10001" "10001" "10001" "01110";
        case 'V': return "10001" "10001" "10001" "10001" "10001" "01010" "00100";
        case 'W': return "10001" "10001" "10001" "10101" "10101" "10101" "01010";
        case 'X': return "10001" "10001" "01010" "00100" "01010" "10001" "10001";
        case 'Y': return "10001" "10001" "01010" "00100" "00100" "00100" "00100";
        case 'Z': return "11111" "00001" "00010" "00100" "01000" "10000" "11111";
        case '0': return "01110" "10001" "10011" "10101" "11001" "10001" "01110";
        case '1': return "00100" "01100" "00100" "00100" "00100" "00100" "01110";
        case '2': return "01110" "10001" "00001" "00010" "00100" "01000" "11111";
        case '3': return "11110" "00001" "00001" "01110" "00001" "00001" "11110";
        case '4': return "00010" "00110" "01010" "10010" "11111" "00010" "00010";
        case '5': return "11111" "10000" "10000" "11110" "00001" "00001" "11110";
        case '6': return "01110" "10000" "10000" "11110" "10001" "10001" "01110";
        case '7': return "11111" "00001" "00010" "00100" "01000" "01000" "01000";
        case '8': return "01110" "10001" "10001" "01110" "10001" "10001" "01110";
        case '9': return "01110" "10001" "10001" "01111" "00001" "00001" "01110";
        case '.': return "00000" "00000" "00000" "00000" "00000" "01100" "01100";
        case ',': return "00000" "00000" "00000" "00000" "01100" "00100" "01000";
        case ':': return "00000" "01100" "01100" "00000" "01100" "01100" "00000";
        case '-': return "00000" "00000" "00000" "11111" "00000" "00000" "00000";
        case '_': return "00000" "00000" "00000" "00000" "00000" "00000" "11111";
        case '/': return "00001" "00010" "00010" "00100" "01000" "01000" "10000";
        case '\\': return "10000" "01000" "01000" "00100" "00010" "00010" "00001";
        case '|': return "00100" "00100" "00100" "00100" "00100" "00100" "00100";
        case '(': return "00010" "00100" "01000" "01000" "01000" "00100" "00010";
        case ')': return "01000" "00100" "00010" "00010" "00010" "00100" "01000";
        case '[': return "01110" "01000" "01000" "01000" "01000" "01000" "01110";
        case ']': return "01110" "00010" "00010" "00010" "00010" "00010" "01110";
        case '+': return "00000" "00100" "00100" "11111" "00100" "00100" "00000";
        case '*': return "00000" "10101" "01110" "11111" "01110" "10101" "00000";
        case '%': return "11001" "11010" "00100" "01000" "10110" "00110" "00000";
        case '&': return "01100" "10010" "10100" "01000" "10101" "10010" "01101";
        case '!': return "00100" "00100" "00100" "00100" "00100" "00000" "00100";
        case '?': return "01110" "10001" "00001" "00010" "00100" "00000" "00100";
        default: return nullptr;
    }
}

void UiRenderer::drawChar(int x, int y, char c, u32 color, int scale) {
    if (c == ' ') return;
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    const char* data = glyph(c);
    if (!data) return;

    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            if (data[row * 5 + col] == '1') {
                fillRect(x + col * scale, y + row * scale, scale, scale, color);
            }
        }
    }
}

void UiRenderer::drawText(int x, int y, const std::string& text, u32 color, int scale) {
    int cursor = x;
    for (char c : text) {
        if (c == '\n') {
            y += 9 * scale;
            cursor = x;
            continue;
        }
        drawChar(cursor, y, c, color, scale);
        cursor += 6 * scale;
    }
}

void UiRenderer::drawTextCentered(int x, int y, int w, const std::string& text, u32 color, int scale) {
    const int textWidth = static_cast<int>(text.size()) * 6 * scale;
    drawText(x + std::max(0, (w - textWidth) / 2), y, text, color, scale);
}

void UiRenderer::drawCard(int x, int y, int w, int h, const std::string& title) {
    fillRect(x, y, w, h, rgb(28, 24, 42));
    drawRect(x, y, w, h, rgb(74, 58, 112), 2);
    fillRect(x, y, 6, h, rgb(143, 84, 255));
    if (!title.empty()) drawText(x + 28, y + 22, title, rgb(238, 232, 255), 3);
}

void UiRenderer::drawButton(int x, int y, int w, int h, const std::string& label, bool selected, bool danger, bool disabled) {
    const u32 disabledBase = rgb(25, 23, 34);
    const u32 disabledBorder = rgb(48, 43, 64);
    const u32 disabledText = rgb(104, 96, 126);

    if (disabled) {
        fillRect(x, y, w, h, disabledBase);
        drawRect(x, y, w, h, disabledBorder, 2);
        drawText(x + 28, y + (h - 14) / 2, label, disabledText, 2);
        return;
    }

    const u32 base = danger ? rgb(58, 30, 44) : rgb(36, 31, 54);
    const u32 active = danger ? rgb(155, 54, 80) : rgb(124, 70, 220);
    const u32 border = selected ? active : rgb(67, 57, 92);
    fillRect(x, y, w, h, selected ? active : base);
    drawRect(x, y, w, h, border, selected ? 4 : 2);
    drawText(x + 28, y + (h - 14) / 2, label, rgb(248, 245, 255), 2);
}

void UiRenderer::drawTab(int x, int y, int w, const std::string& label, bool active) {
    fillRect(x, y, w, 46, active ? rgb(124, 70, 220) : rgb(32, 28, 48));
    drawRect(x, y, w, 46, active ? rgb(184, 140, 255) : rgb(68, 58, 92), 2);
    drawTextCentered(x, y + 14, w, label, rgb(248, 245, 255), 2);
}

void UiRenderer::drawStatusPill(int x, int y, const std::string& label, bool active) {
    const int w = 220;
    fillRect(x, y, w, 34, active ? rgb(38, 92, 65) : rgb(82, 48, 62));
    drawRect(x, y, w, 34, active ? rgb(78, 185, 128) : rgb(168, 78, 108), 2);
    drawTextCentered(x, y + 9, w, label, rgb(248, 245, 255), 2);
}


void UiRenderer::drawImageRgba(int x, int y, int w, int h, const unsigned char* rgba, int imageW, int imageH) {
    if (!ready || !rgba || w <= 0 || h <= 0 || imageW <= 0 || imageH <= 0) return;

    const float srcAspect = static_cast<float>(imageW) / static_cast<float>(imageH);
    const float dstAspect = static_cast<float>(w) / static_cast<float>(h);

    int drawW = w;
    int drawH = h;
    if (srcAspect > dstAspect) {
        drawH = std::max(1, static_cast<int>(w / srcAspect));
    } else {
        drawW = std::max(1, static_cast<int>(h * srcAspect));
    }

    const int drawX = x + (w - drawW) / 2;
    const int drawY = y + (h - drawH) / 2;

    // First-pass Vita renderer: draw the RGBA preview directly through scaled rectangles.
    // This keeps the NX preview code working without introducing texture lifetime issues.
    // It can be optimized later by caching vita2d textures.
    const int step = std::max(1, drawW / 180);
    for (int yy = 0; yy < drawH; yy += step) {
        const int syy = std::min(imageH - 1, yy * imageH / drawH);
        for (int xx = 0; xx < drawW; xx += step) {
            const int sxx = std::min(imageW - 1, xx * imageW / drawW);
            const unsigned char* px = rgba + ((syy * imageW + sxx) * 4);
            if (px[3] == 0) continue;
            fillRect(drawX + xx, drawY + yy, step, step, RGBA8(px[0], px[1], px[2], px[3]));
        }
    }
}

void UiRenderer::drawFooter(const std::string& text) {
    fillRect(0, Height - 54, Width, 54, rgb(20, 17, 31));
    drawRect(0, Height - 54, Width, 2, rgb(74, 58, 112), 2);
    drawText(40, Height - 34, text, rgb(194, 184, 218), 2);
}

void UiRenderer::drawMessage(const std::string& message) {
    if (message.empty()) return;
    fillRect(40, Height - 112, Width - 80, 42, rgb(39, 34, 57));
    drawRect(40, Height - 112, Width - 80, 42, rgb(143, 84, 255), 2);
    drawText(62, Height - 99, message, rgb(248, 245, 255), 2);
}
