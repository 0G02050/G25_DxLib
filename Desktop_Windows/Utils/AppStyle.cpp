#include "AppStyle.h" // 引用自己的头文件

namespace AppStyle {
    // 这里才是真正的定义
    const int SCREEN_W = 1920;
    const int SCREEN_H = 1080;
    const int WIN_W = 1280;
    const int WIN_H = 720;

    // 定义对象
    Palette Colors;

    // 实现 Init 函数
    void Palette::Init() {
        BG = GetColor(18, 18, 18);
        CARD = GetColor(28, 28, 30);
        BORDER = GetColor(58, 58, 60);
        ACCENT = GetColor(10, 132, 255);
        TEXT_MAIN = GetColor(255, 255, 255);
        TEXT_SUB = GetColor(142, 142, 147);
        INPUT_BG = GetColor(10, 10, 10);
        WARNING = GetColor(255, 69, 58);
        SUCCESS = GetColor(48, 209, 88);
        QR_BG = GetColor(255, 255, 255);
    }
}