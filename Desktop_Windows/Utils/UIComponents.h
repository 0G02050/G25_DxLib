#pragma once
#include "DxLib.h"
#include "AppStyle.h" // 需要用到颜色

namespace UI {
    void DrawRoundedBoxAA(float x, float y, float w, float h, float r, int color);
    void DrawRoundedBorderAA(float x, float y, float w, float h, float r, float borderW, int borderColor, int innerColor);

    // 画真实二维码
    void DrawRealQRCode(int x, int y, int size, const char* data);

    // 画现代风格按钮
    void DrawModernButton(int x, int y, int w, int h, const char* text, int font, bool isHover, int baseColor, int textColor, int textOffsetY = 0, int textOffsetX = 0);
}