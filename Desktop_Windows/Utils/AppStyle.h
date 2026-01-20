#pragma once
#include "DxLib.h"

namespace AppStyle {
    // extern宣言：実体は「別の場所（.cpp）」で定義されており、ここは参照のみ
    extern const int SCREEN_W;
    extern const int SCREEN_H;
    extern const int WIN_W;
    extern const int WIN_H;

    struct Palette {
        int BG, CARD, BORDER, ACCENT, TEXT_MAIN, TEXT_SUB, INPUT_BG, WARNING, SUCCESS, QR_BG;
        void Init(); // プロトタイプ宣言（実装は.cppに記述）
    };

    // グローバル共有カラーオブジェクトの宣言
    extern Palette Colors;
}