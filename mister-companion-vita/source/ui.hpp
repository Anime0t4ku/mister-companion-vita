#pragma once

#include "switch.h"

#include <string>

class UiRenderer {
public:
    static constexpr int Width = 1280;
    static constexpr int Height = 720;
    static constexpr int VitaWidth = 960;
    static constexpr int VitaHeight = 544;
    static constexpr float Scale = 0.75f;
    static constexpr float OffsetX = 0.0f;
    static constexpr float OffsetY = 2.0f;

    bool initialize();
    void shutdown();

    void beginFrame();
    void endFrame();

    void clear(u32 color);
    void fillRect(int x, int y, int w, int h, u32 color);
    void drawRect(int x, int y, int w, int h, u32 color, int thickness = 2);
    void drawText(int x, int y, const std::string& text, u32 color, int scale = 2);
    void drawTextCentered(int x, int y, int w, const std::string& text, u32 color, int scale = 2);
    void drawCard(int x, int y, int w, int h, const std::string& title = "");
    void drawButton(int x, int y, int w, int h, const std::string& label, bool selected, bool danger = false, bool disabled = false);
    void drawTab(int x, int y, int w, const std::string& label, bool active);
    void drawStatusPill(int x, int y, const std::string& label, bool active);
    void drawFooter(const std::string& text);
    void drawMessage(const std::string& message);
    void drawImageRgba(int x, int y, int w, int h, const unsigned char* rgba, int imageW, int imageH);

    static u32 rgb(u8 r, u8 g, u8 b);

private:
    bool ready = false;

    int sx(int value) const;
    int sy(int value) const;
    int sw(int value) const;
    void putPixel(int x, int y, u32 color);
    void drawChar(int x, int y, char c, u32 color, int scale);
    const char* glyph(char c) const;
};
