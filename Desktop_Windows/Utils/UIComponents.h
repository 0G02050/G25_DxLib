#pragma once
#include "DxLib.h"
#include "AppStyle.h" // カラー定義の使用

namespace UI {
    void DrawRoundedBoxAA(float x, float y, float w, float h, float r, int color);
    void DrawRoundedBorderAA(float x, float y, float w, float h, float r, float borderW, int borderColor, int innerColor);

    // リアルQRコードの描画
    void DrawRealQRCode(int x, int y, int size, const char* data);

    // モダンなスタイルのボタン描画
    void DrawModernButton(int x, int y, int w, int h, const char* text, int font, bool isHover, int baseColor, int textColor, int textOffsetY = 0, int textOffsetX = 0);
}