#pragma once
#include "DxLib.h"

namespace AppStyle {
    // 使用 extern 告诉编译器：这些变量在“别的地方”定义了，这里只是引用
    extern const int SCREEN_W;
    extern const int SCREEN_H;
    extern const int WIN_W;
    extern const int WIN_H;

    struct Palette {
        int BG, CARD, BORDER, ACCENT, TEXT_MAIN, TEXT_SUB, INPUT_BG, WARNING, SUCCESS, QR_BG;
        void Init(); // 这里只写函数名，不写里面的代码
    };

    // 声明全局共用的颜色对象
    extern Palette Colors;
}