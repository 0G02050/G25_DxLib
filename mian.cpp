#include "DxLib.h"
#include "PasswordTools.h"
#include <string>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <time.h> 

// ================= 全局配置 =================
const int SCREEN_W = 1920;
const int SCREEN_H = 1080;
const int WIN_W = 1280; // 窗口显示大小
const int WIN_H = 720;

// ================= 颜色配置 =================
int COL_BG, COL_CARD, COL_BORDER, COL_ACCENT, COL_TEXT_MAIN, COL_TEXT_SUB, COL_INPUT_BG, COL_WARNING, COL_SUCCESS;

void InitColors() {
    COL_BG = GetColor(18, 18, 18);
    COL_CARD = GetColor(28, 28, 30);
    COL_BORDER = GetColor(58, 58, 60);
    COL_ACCENT = GetColor(10, 132, 255);
    COL_TEXT_MAIN = GetColor(255, 255, 255);
    COL_TEXT_SUB = GetColor(142, 142, 147);
    COL_INPUT_BG = GetColor(10, 10, 10);
    COL_WARNING = GetColor(255, 69, 58);
    COL_SUCCESS = GetColor(48, 209, 88);
}

// ================= 数据结构与加密 =================
enum TitleState { STATE_TYPE_JP, STATE_SHOW_JP, STATE_DELETE_JP, STATE_BLANK_AFTER_JP, STATE_TYPE_EN, STATE_SHOW_EN, STATE_DELETE_EN, STATE_BLANK_AFTER_EN };
enum FocusTarget { FOCUS_SEED, FOCUS_HINT };
struct HintField { char text[128]; int len; };

const std::string SECRET_KEY = "user0001";

std::string EncryptPassword(const char* rawPass) {
    std::string result = "COLDVAULT:";
    int keyLen = (int)SECRET_KEY.length();
    int passLen = (int)strlen(rawPass);
    char hexBuf[4];
    for (int i = 0; i < passLen; i++) {
        unsigned char encryptedChar = rawPass[i] ^ SECRET_KEY[i % keyLen];
        sprintf_s(hexBuf, "%02X", encryptedChar);
        result += hexBuf;
    }
    return result;
}

// ================= UI 绘制函数 =================
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

void DrawModernButton(int x, int y, int w, int h, const char* text, int font, bool isHover, int baseColor, int textColor, int textOffsetY = 0) {
    int drawColor = isHover ? GetColor(64, 156, 255) : baseColor;
    if (baseColor == COL_BORDER || baseColor == COL_WARNING) {
        int inner = isHover ? GetColor(40, 44, 50) : COL_INPUT_BG;
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
    int strW = GetDrawStringWidthToHandle(text, (int)strlen(text), font);
    int strH = GetFontSizeToHandle(font);
    DrawStringToHandle(x + (w - strW) / 2, y + (h - strH) / 2 + 2 + textOffsetY, text, textColor, font);
}

// ================= 主程序入口 =================
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    SetWindowSizeChangeEnableFlag(TRUE, FALSE);
    ChangeWindowMode(TRUE);
    SetGraphMode(SCREEN_W, SCREEN_H, 32);
    SetWindowSize(WIN_W, WIN_H);
    SetWindowText("ColdVault Generator");
    SetFullSceneAntiAliasingMode(4, 2);

    if (DxLib_Init() == -1) return -1;

    SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8);
    SetDrawScreen(DX_SCREEN_BACK);
    InitColors();

    // 字体资源
    int fontTitle = CreateFontToHandle("Segoe UI", 56, 4, DX_FONTTYPE_ANTIALIASING_8X8);
    int fontMedium = CreateFontToHandle("Segoe UI", 32, 4, DX_FONTTYPE_ANTIALIASING_8X8);
    int fontMain = CreateFontToHandle("Segoe UI", 22, 4, DX_FONTTYPE_ANTIALIASING_8X8);
    int fontSmall = CreateFontToHandle("Segoe UI", 18, 4, DX_FONTTYPE_ANTIALIASING_8X8);
    int fontCode = CreateFontToHandle("Consolas", 26, 4, DX_FONTTYPE_ANTIALIASING_8X8);

    // 动画状态
    const char* titleJP = "オフライン暗号室";
    const char* titleEN = "ColdVault Protocol";
    int lenJPBytes = (int)strlen(titleJP);
    int lenENChars = (int)strlen(titleEN);
    TitleState state = STATE_TYPE_JP;
    int frameCounter = 0, waitCounter = 0, visibleJPBytes = 0, visibleENChars = 0;
    const int STEP_TYPE = 6, STEP_DELETE = 3, WAIT_SHOW = 200, WAIT_BLANK = 20;

    // 业务数据
    char seedText[256] = { 0 }; int seedLen = 0;
    int passLen = 16;
    HintField hints[16];
    int hintCount = 1;
    for (int i = 0; i < 16; i++) { hints[i].text[0] = '\0'; hints[i].len = 0; }

    FocusTarget focusTarget = FOCUS_SEED;
    int focusHintIndex = 0;
    int cursorTimer = 0;
    bool showResult = false;

    char generatedPass[128] = { 0 };
    std::string cachedEncryptedData = ""; //  缓存变量：解决内存暴涨的关键

    int saveMsgTimer = 0;
    int lastMouseInput = 0;

    while (ProcessMessage() == 0) {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;
        int mouseInput = GetMouseInput();
        int mouseX, mouseY; GetMousePoint(&mouseX, &mouseY);
        bool mouseDown = (mouseInput & MOUSE_INPUT_LEFT) != 0;
        bool mouseClick = (mouseDown && !(lastMouseInput & MOUSE_INPUT_LEFT));
        lastMouseInput = mouseInput;

        frameCounter++; cursorTimer++;
        if (saveMsgTimer > 0) saveMsgTimer--;

        // 标题打字机动画
        switch (state) {
        case STATE_TYPE_JP: if (frameCounter >= STEP_TYPE) { frameCounter = 0; if (visibleJPBytes < lenJPBytes) visibleJPBytes += 3; else { state = STATE_SHOW_JP; waitCounter = 0; } } break;
        case STATE_SHOW_JP: if (++waitCounter > WAIT_SHOW) { waitCounter = 0; frameCounter = 0; state = STATE_DELETE_JP; } break;
        case STATE_DELETE_JP: if (frameCounter >= STEP_DELETE) { frameCounter = 0; if (visibleJPBytes > 0) visibleJPBytes -= 3; else { state = STATE_BLANK_AFTER_JP; waitCounter = 0; } } break;
        case STATE_BLANK_AFTER_JP: if (++waitCounter > WAIT_BLANK) { waitCounter = 0; frameCounter = 0; visibleENChars = 0; state = STATE_TYPE_EN; } break;
        case STATE_TYPE_EN: if (frameCounter >= STEP_TYPE) { frameCounter = 0; if (visibleENChars < lenENChars) visibleENChars++; else { state = STATE_SHOW_EN; waitCounter = 0; } } break;
        case STATE_SHOW_EN: if (++waitCounter > WAIT_SHOW) { waitCounter = 0; frameCounter = 0; state = STATE_DELETE_EN; } break;
        case STATE_DELETE_EN: if (frameCounter >= STEP_DELETE) { frameCounter = 0; if (visibleENChars > 0) visibleENChars--; else { state = STATE_BLANK_AFTER_EN; waitCounter = 0; } } break;
        case STATE_BLANK_AFTER_EN: if (++waitCounter > WAIT_BLANK) { waitCounter = 0; frameCounter = 0; visibleJPBytes = 0; state = STATE_TYPE_JP; } break;
        }

        // 布局参数
        const int PADDING = 40;
        int panelX = PADDING, panelY = 100;
        int panelW = SCREEN_W - PADDING * 2, panelH = SCREEN_H - panelY - PADDING;
        int contentX = panelX + PADDING, contentW = panelW - PADDING * 2;

        int seedY = panelY + 40, seedH = 50;
        int lenY = seedY + seedH + 30, lenBtnSize = 40;
        int hintHeaderY = lenY + 50, hintBtnSize = 35;
        int genBtnH = 60, genBtnY = panelY + panelH - genBtnH - 30;
        int listY = hintHeaderY + 45, itemH = 45, itemGap = 6;

        int lenMinusX = contentX + 200, lenPlusX = lenMinusX + 140;
        int hintCtrlX = contentX + contentW - 220, hintMinusX = hintCtrlX, hintPlusX = hintCtrlX + 70;

        // ================= 输入交互 =================
        if (!showResult) {
            if (mouseClick) {
                if (mouseX >= contentX && mouseX <= contentX + contentW && mouseY >= seedY && mouseY <= seedY + seedH) focusTarget = FOCUS_SEED;
                if (mouseX >= lenMinusX && mouseX <= lenMinusX + lenBtnSize && mouseY >= lenY && mouseY <= lenY + lenBtnSize && passLen > 4) passLen--;
                if (mouseX >= lenPlusX && mouseX <= lenPlusX + lenBtnSize && mouseY >= lenY && mouseY <= lenY + lenBtnSize && passLen < 64) passLen++;
                if (mouseX >= hintMinusX && mouseX <= hintMinusX + hintBtnSize && mouseY >= hintHeaderY && mouseY <= hintHeaderY + hintBtnSize && hintCount > 1) {
                    hintCount--; if (focusTarget == FOCUS_HINT && focusHintIndex >= hintCount) focusHintIndex = hintCount - 1;
                }
                if (mouseX >= hintPlusX && mouseX <= hintPlusX + hintBtnSize && mouseY >= hintHeaderY && mouseY <= hintHeaderY + hintBtnSize && hintCount < 16) {
                    hints[hintCount].text[0] = '\0'; hints[hintCount].len = 0; hintCount++; focusTarget = FOCUS_HINT; focusHintIndex = hintCount - 1;
                }
                // 点击生成按钮
                if (mouseX >= contentX && mouseX <= contentX + contentW && mouseY >= genBtnY && mouseY <= genBtnY + genBtnH) {
                    std::string all; for (int i = 0; i < hintCount; i++) { all += hints[i].text; all += "|"; }
                    GeneratePassword(seedText, all.c_str(), passLen, generatedPass, sizeof(generatedPass));

                    // ✅ 关键修复：只在点击时计算一次，防止内存泄露
                    cachedEncryptedData = EncryptPassword(generatedPass);

                    showResult = true; saveMsgTimer = 0;
                }
                if (mouseX >= contentX && mouseX <= contentX + contentW && mouseY >= listY && mouseY <= listY + hintCount * (itemH + itemGap)) {
                    int idx = (mouseY - listY) / (itemH + itemGap);
                    if (idx >= 0 && idx < hintCount) { focusTarget = FOCUS_HINT; focusHintIndex = idx; }
                }
            }
            // 键盘处理
            char* targetBuf = NULL; int* targetLen = NULL; int maxS = 0;
            if (focusTarget == FOCUS_SEED) { targetBuf = seedText; targetLen = &seedLen; maxS = sizeof(seedText); }
            else { targetBuf = hints[focusHintIndex].text; targetLen = &hints[focusHintIndex].len; maxS = sizeof(hints[focusHintIndex].text); }
            char c;
            while ((c = GetInputChar(TRUE)) != 0) {
                if (c == 8 && *targetLen > 0) { (*targetLen)--; targetBuf[*targetLen] = '\0'; }
                else if (c >= 32 && c <= 126 && *targetLen < maxS - 1) { targetBuf[*targetLen] = c; (*targetLen)++; targetBuf[*targetLen] = '\0'; }
            }
        }

        // ================= 画面绘制 =================
        ClearDrawScreen();
        DrawBox(0, 0, SCREEN_W, SCREEN_H, COL_BG, TRUE);

        // 标题
        char tBuf[64] = { 0 };
        if (state <= STATE_BLANK_AFTER_JP) for (int i = 0; i < visibleJPBytes; i++) tBuf[i] = titleJP[i];
        else for (int i = 0; i < visibleENChars; i++) tBuf[i] = titleEN[i];
        DrawStringToHandle(panelX, 35, tBuf, COL_TEXT_MAIN, fontTitle);

        // 主面板
        DrawRoundedBorderAA(panelX, panelY, panelW, panelH, 16, 2, COL_BORDER, COL_CARD);

        // Seed
        DrawStringToHandle(contentX, seedY - 25, "Master Key Phrase", COL_TEXT_SUB, fontSmall);
        int inputBorderCol = (focusTarget == FOCUS_SEED) ? COL_ACCENT : COL_BORDER;
        DrawRoundedBorderAA(contentX, seedY, contentW, seedH, 8, 2, inputBorderCol, COL_INPUT_BG);

        int ty = seedY + (seedH - GetFontSizeToHandle(fontMain)) / 2;
        if (seedLen > 0) DrawStringToHandle(contentX + 15, ty, seedText, COL_TEXT_MAIN, fontMain);
        else DrawStringToHandle(contentX + 15, ty, "Type your secret phrase here...", COL_TEXT_SUB, fontMain);

        if (focusTarget == FOCUS_SEED && (cursorTimer / 30) % 2 == 0 && !showResult) {
            int tw = GetDrawStringWidthToHandle(seedText, seedLen, fontMain);
            DrawLine(contentX + 15 + tw, ty, contentX + 15 + tw, ty + 24, COL_ACCENT, 2);
        }

        // Length
        DrawStringToHandle(contentX, lenY + 8, "Password Length", COL_TEXT_SUB, fontSmall);
        bool hoverLenMinus = (mouseX >= lenMinusX && mouseX <= lenMinusX + lenBtnSize && mouseY >= lenY && mouseY <= lenY + lenBtnSize);
        DrawModernButton(lenMinusX, lenY, lenBtnSize, lenBtnSize, "-", fontMain, hoverLenMinus, COL_BORDER, COL_TEXT_MAIN, -2);
        bool hoverLenPlus = (mouseX >= lenPlusX && mouseX <= lenPlusX + lenBtnSize && mouseY >= lenY && mouseY <= lenY + lenBtnSize);
        DrawModernButton(lenPlusX, lenY, lenBtnSize, lenBtnSize, "+", fontMain, hoverLenPlus, COL_BORDER, COL_TEXT_MAIN, -1);
        char lb[16]; sprintf_s(lb, "%d", passLen);
        int lbW = GetDrawStringWidthToHandle(lb, strlen(lb), fontMedium);
        DrawStringToHandle(lenMinusX + lenBtnSize + (lenPlusX - (lenMinusX + lenBtnSize) - lbW) / 2, lenY - 4, lb, COL_ACCENT, fontMedium);

        // Hints
        DrawStringToHandle(contentX, hintHeaderY, "Context Hints", COL_TEXT_SUB, fontSmall);
        char hcb[32]; sprintf_s(hcb, "%d / 16", hintCount);
        DrawStringToHandle(hintPlusX + hintBtnSize + 15, hintHeaderY + 5, hcb, COL_TEXT_SUB, fontSmall);
        bool hoverHintMinus = (mouseX >= hintMinusX && mouseX <= hintMinusX + hintBtnSize && mouseY >= hintHeaderY && mouseY <= hintHeaderY + hintBtnSize);
        DrawModernButton(hintMinusX, hintHeaderY, hintBtnSize, hintBtnSize, "-", fontSmall, hoverHintMinus, COL_BORDER, COL_TEXT_MAIN, -2);
        bool hoverHintPlus = (mouseX >= hintPlusX && mouseX <= hintPlusX + hintBtnSize && mouseY >= hintHeaderY && mouseY <= hintHeaderY + hintBtnSize);
        DrawModernButton(hintPlusX, hintHeaderY, hintBtnSize, hintBtnSize, "+", fontSmall, hoverHintPlus, COL_BORDER, COL_TEXT_MAIN, -1);

        for (int i = 0; i < hintCount; i++) {
            int y = listY + i * (itemH + itemGap);
            bool isFocus = (focusTarget == FOCUS_HINT && focusHintIndex == i);
            if (isFocus) DrawRoundedBorderAA(contentX, y, contentW, itemH, 6, 2, COL_ACCENT, COL_INPUT_BG);
            else DrawRoundedBoxAA(contentX, y, contentW, itemH, 6, COL_INPUT_BG);
            int hy = y + (itemH - GetFontSizeToHandle(fontMain)) / 2;
            if (hints[i].len > 0) DrawStringToHandle(contentX + 15, hy, hints[i].text, COL_TEXT_MAIN, fontMain);
            else DrawStringToHandle(contentX + 15, hy, "e.g. 'Gmail', 'Bank'...", COL_TEXT_SUB, fontMain);
            if (isFocus && (cursorTimer / 30) % 2 == 0 && !showResult) {
                int tw = GetDrawStringWidthToHandle(hints[i].text, hints[i].len, fontMain);
                DrawLine(contentX + 15 + tw, hy, contentX + 15 + tw, hy + 24, COL_ACCENT, 2);
            }
        }

        // Gen Button
        bool hoverGen = (mouseX >= contentX && mouseX <= contentX + contentW && mouseY >= genBtnY && mouseY <= genBtnY + genBtnH);
        DrawModernButton(contentX, genBtnY, contentW, genBtnH, "GENERATE PASSWORD", fontMain, hoverGen, COL_ACCENT, COL_TEXT_MAIN, 0);

        // ================= 结果弹窗 =================
        if (showResult) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
            DrawBox(0, 0, SCREEN_W, SCREEN_H, GetColor(0, 0, 0), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            int rw = 640, rh = 640;
            int rx = (SCREEN_W - rw) / 2, ry = (SCREEN_H - rh) / 2;

            DrawRoundedBorderAA(rx, ry, rw, rh, 16, 2, COL_BORDER, COL_CARD);
            DrawStringToHandle(rx + 40, ry + 30, "Generated Securely", COL_TEXT_SUB, fontSmall);

            int passBoxH = 80, passBoxY = ry + 60;
            DrawRoundedBoxAA(rx + 40, passBoxY, rw - 80, passBoxH, 12, COL_INPUT_BG);
            int passW = GetDrawStringWidthToHandle(generatedPass, strlen(generatedPass), fontCode);
            DrawStringToHandle(rx + (rw - passW) / 2, passBoxY + (passBoxH - 30) / 2, generatedPass, COL_ACCENT, fontCode);

            int qrSize = 280, qrY = passBoxY + passBoxH + 30;

            // ✅ 关键修复：直接使用缓存的字符串进行绘制，不再每帧重复计算
            int qrX = rx + (rw - qrSize) / 2;
            DrawFakeQRCode(qrX, qrY, qrSize, cachedEncryptedData.c_str());

            DrawStringToHandle(qrX + 40, qrY + qrSize + 10, "Scan with ColdVault App", COL_TEXT_SUB, fontSmall);

            int btnY = qrY + qrSize + 50;
            int btnW = 160, btnH = 50, gap = 20;
            int startX = rx + (rw - (btnW * 2 + gap)) / 2;

            bool hoverSave = (mouseX >= startX && mouseX <= startX + btnW && mouseY >= btnY && mouseY <= btnY + btnH);
            DrawModernButton(startX, btnY, btnW, btnH, "SAVE IMAGE", fontMain, hoverSave, COL_BORDER, COL_TEXT_MAIN, 0);

            int closeX = startX + btnW + gap;
            bool hoverClose = (mouseX >= closeX && mouseX <= closeX + btnW && mouseY >= btnY && mouseY <= btnY + btnH);
            DrawModernButton(closeX, btnY, btnW, btnH, "CLOSE", fontMain, hoverClose, COL_WARNING, COL_TEXT_MAIN, 0);

            if (saveMsgTimer > 0) {
                const char* msg = "Saved to folder!";
                int mw = GetDrawStringWidthToHandle(msg, strlen(msg), fontMain);
                DrawStringToHandle(rx + (rw - mw) / 2, btnY + btnH + 15, msg, COL_SUCCESS, fontMain);
            }

            if (mouseClick) {
                if (hoverSave) {
                    char filename[64]; sprintf_s(filename, "Pass_%ld.png", time(NULL));
                    SetMouseDispFlag(FALSE); ScreenFlip();
                    SaveDrawScreenToPNG(0, 0, SCREEN_W, SCREEN_H, filename);
                    SetMouseDispFlag(TRUE); saveMsgTimer = 120;
                }
                if (hoverClose) showResult = false;
            }
        }
        ScreenFlip();
    }

    DeleteFontToHandle(fontTitle); DeleteFontToHandle(fontMedium); DeleteFontToHandle(fontMain); DeleteFontToHandle(fontSmall); DeleteFontToHandle(fontCode);
    DxLib_End();
    return 0;
}