#include "UIComponents.h"
#include "../External/qrcodegen.hpp" // 引用二维码库
using namespace qrcodegen;

namespace UI {
    void DrawRoundedBoxAA(float x, float y, float w, float h, float r, int color) {
        DrawCircleAA(x + r, y + r, r, 64, color, TRUE);
        DrawCircleAA(x + w - r, y + r, r, 64, color, TRUE);
        DrawCircleAA(x + r, y + h - r, r, 64, color, TRUE);
        DrawCircleAA(x + w - r, y + h - r, r, 64, color, TRUE);
        DrawBoxAA(x, y + r, x + w + 1, y + h - r, color, TRUE);
        DrawBoxAA(x + r, y, x + w - r, y + h + 1, color, TRUE);
    }

    void DrawRoundedBorderAA(float x, float y, float w, float h, float r, float borderW, int borderColor, int innerColor) {
        DrawRoundedBoxAA(x, y, w, h, r, borderColor);
        DrawRoundedBoxAA(x + borderW, y + borderW, w - borderW * 2, h - borderW * 2, r - 1, innerColor);
    }

    void DrawRealQRCode(int x, int y, int size, const char* data) {
        QrCode qr = QrCode::encodeText(data, QrCode::Ecc::LOW);
        int gridSize = qr.getSize();
        int padding = 10;
        int drawSize = size - (padding * 2);
        int cellSize = drawSize / gridSize;
        if (cellSize < 1) cellSize = 1;
        int startX = x + (size - (cellSize * gridSize)) / 2;
        int startY = y + (size - (cellSize * gridSize)) / 2;

        DrawBox(x, y, x + size, y + size, AppStyle::Colors.QR_BG, TRUE);

        for (int r = 0; r < gridSize; r++) {
            for (int c = 0; c < gridSize; c++) {
                if (qr.getModule(c, r)) {
                    DrawBox(startX + c * cellSize, startY + r * cellSize, startX + (c + 1) * cellSize, startY + (r + 1) * cellSize, GetColor(0, 0, 0), TRUE);
                }
            }
        }
    }

    // 修改后的 DrawModernButton，支持上下(Y)和左右(X)微调
    void DrawModernButton(int x, int y, int w, int h, const char* text, int font, bool isHover, int baseColor, int textColor, int textOffsetY, int textOffsetX) {
        int drawColor = isHover ? GetColor(64, 156, 255) : baseColor;

        // 绘制按钮背景
        if (baseColor == AppStyle::Colors.BORDER || baseColor == AppStyle::Colors.WARNING) {
            int inner = isHover ? GetColor(40, 44, 50) : AppStyle::Colors.INPUT_BG;
            DrawRoundedBorderAA(x, y, w, h, 8, 2, drawColor, inner);
        }
        else {
            if (!isHover) {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 80);
                DrawRoundedBoxAA(x + 2, y + 3, w, h, 8, GetColor(0, 0, 0));
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            }
            DrawRoundedBoxAA(x, y, w, h, 8, drawColor);
        }

        // 计算文字宽高
        int strW = GetDrawStringWidthToHandle(text, (int)strlen(text), font);
        int strH = GetFontSizeToHandle(font);

        // calculate X: 居中位置 + textOffsetX
        // calculate Y: 居中位置 + 2(视觉修正) + textOffsetY
        DrawStringToHandle(
            x + (w - strW) / 2 + textOffsetX,       // <--- 这里加了 X 偏移
            y + (h - strH) / 2 + 2 + textOffsetY,   // <--- 这里是 Y 偏移
            text, textColor, font
        );
    }
}