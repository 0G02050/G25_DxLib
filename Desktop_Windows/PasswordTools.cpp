#include "PasswordTools.h"
#include "DxLib.h"
#include "qrcodegen.hpp" // <--- 引入你刚才复制进去的库
#include <string>
#include <vector>
#include <random>
#include <functional>

// 使用库的命名空间，方便调用
using namespace qrcodegen;

// 1. 密码生成逻辑 (保持不变)
void GeneratePassword(const char* seedStr, const char* extraHint, int length, char* outBuffer, int outBufferSize) {
    std::string rawSource = std::string(seedStr) + extraHint;
    std::hash<std::string> hasher;
    size_t seedValue = hasher(rawSource);
    std::mt19937 gen((unsigned int)seedValue);

    const std::string charset =
        "abcdefghijkmnpqrstuvwxyz"
        "ABCDEFGHJKLMNPQRSTUVWXYZ"
        "23456789!@#$%&*+?";

    std::uniform_int_distribution<> dist(0, (int)charset.size() - 1);
    int actualLen = (length < outBufferSize - 1) ? length : outBufferSize - 1;
    for (int i = 0; i < actualLen; i++) outBuffer[i] = charset[dist(gen)];
    outBuffer[actualLen] = '\0';
}

// 2. 【修改版】绘制 真·二维码
void DrawFakeQRCode(int x, int y, int size, const char* data) {
    // A. 创建二维码数据对象
    // Ecc::LOW = 容错率低 (适合屏幕显示，且密度小容易扫)
    QrCode qr = QrCode::encodeText(data, QrCode::Ecc::LOW);

    int gridSize = qr.getSize(); // 获取二维码的矩阵大小 (比如 21x21, 25x25)

    // B. 计算每个格子在屏幕上的像素大小
    int padding = 10; // 留白边距
    int drawSize = size - (padding * 2);
    int cellSize = drawSize / gridSize;

    // 防止太小画不出来
    if (cellSize < 1) cellSize = 1;

    // 重新计算实际居中的起始位置
    int startX = x + (size - (cellSize * gridSize)) / 2;
    int startY = y + (size - (cellSize * gridSize)) / 2;

    // C. 绘制白色背景 (必须画，否则手机扫不出来)
    DrawBox(x, y, x + size, y + size, GetColor(255, 255, 255), TRUE);

    // D. 遍历矩阵画黑块
    for (int r = 0; r < gridSize; r++) {
        for (int c = 0; c < gridSize; c++) {
            // qr.getModule(c, r) 返回 true 代表黑色，false 代表白色
            if (qr.getModule(c, r)) {
                DrawBox(startX + c * cellSize,
                    startY + r * cellSize,
                    startX + (c + 1) * cellSize,
                    startY + (r + 1) * cellSize,
                    GetColor(0, 0, 0), TRUE);
            }
        }
    }
}