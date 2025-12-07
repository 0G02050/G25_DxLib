#include "DxLib.h"
#include "PasswordTools.h"
#include <string.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <random>
#include <functional>
#include <time.h> // 用于生成唯一文件名
// ================= 数据结构 =================

enum TitleState { STATE_TYPE_JP, STATE_SHOW_JP, STATE_DELETE_JP, STATE_BLANK_AFTER_JP, STATE_TYPE_EN, STATE_SHOW_EN, STATE_DELETE_EN, STATE_BLANK_AFTER_EN };
enum FocusTarget { FOCUS_SEED, FOCUS_HINT };
struct HintField { char text[128]; int len; };

// ================= 主程序 =================

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    const int SCREEN_W = 1280;
    const int SCREEN_H = 720;

    ChangeWindowMode(TRUE);
    SetGraphMode(SCREEN_W, SCREEN_H, 32);
    if (DxLib_Init() == -1) return -1;

    SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8);
    SetDrawScreen(DX_SCREEN_BACK);

    // 配色方案
    const int COL_BG = GetColor(15, 17, 26);
    const int COL_PANEL = GetColor(30, 34, 46);
    const int COL_BORDER = GetColor(80, 90, 110);
    const int COL_ACCENT = GetColor(0, 200, 255);
    const int COL_TEXT = GetColor(230, 230, 230);
    const int COL_PLACE = GetColor(120, 130, 150);
    const int COL_INPUT_BG = GetColor(10, 12, 16);
    const int COL_DISABLED = GetColor(60, 60, 70);
    const int COL_SUCCESS = GetColor(50, 220, 100);
    const int COL_WARNING = GetColor(255, 80, 80);   // 警告色 (红色)

    // 字体加载
    int fontTitle = CreateFontToHandle(NULL, 64, 4, DX_FONTTYPE_ANTIALIASING_8X8);
    int fontMedium = CreateFontToHandle(NULL, 40, 4, DX_FONTTYPE_ANTIALIASING_8X8);
    int fontMain = CreateFontToHandle(NULL, 24, 4, DX_FONTTYPE_ANTIALIASING_8X8);
    int fontSmall = CreateFontToHandle(NULL, 18, 4, DX_FONTTYPE_ANTIALIASING_8X8);
    int titleFontSize = GetFontSizeToHandle(fontTitle);

    // 标题动画变量
    const char* titleJP = "オフライン暗号室";
    const char* titleEN = "ColdVault Protocol";
    int lenJPBytes = (int)strlen(titleJP);
    int lenENChars = (int)strlen(titleEN);
    TitleState state = STATE_TYPE_JP;
    int frameCounter = 0; int waitCounter = 0;
    int visibleJPBytes = 0; int visibleENChars = 0;
    const int STEP_TYPE = 8; const int STEP_DELETE = 4;
    const int WAIT_SHOW = 300; const int WAIT_BLANK = 30;

    // 核心数据
    char seedText[256] = { 0 }; int seedLen = 0;
    int passLen = 16;
    HintField hints[16];
    int hintCount = 1;
    for (int i = 0; i < 16; i++) { hints[i].text[0] = '\0'; hints[i].len = 0; }

    // 交互状态
    FocusTarget focusTarget = FOCUS_SEED;
    int focusHintIndex = 0;
    int cursorTimer = 0;
    bool showResult = false;
    char generatedPass[128] = { 0 };
    int saveMsgTimer = 0; // 保存成功的提示倒计时

    // 输入控制
    int lastMouseInput = 0;
    char keyState[256] = { 0 }; char prevKeyState[256] = { 0 };

    while (ProcessMessage() == 0) {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;
        memcpy(prevKeyState, keyState, 256); GetHitKeyStateAll(keyState);
        int mouseInput = GetMouseInput();
        int mouseX, mouseY; GetMousePoint(&mouseX, &mouseY);
        bool mouseDown = (mouseInput & MOUSE_INPUT_LEFT) != 0;
        bool mouseClick = (mouseDown && !(lastMouseInput & MOUSE_INPUT_LEFT));
        lastMouseInput = mouseInput;

        frameCounter++; cursorTimer++;
        if (saveMsgTimer > 0) saveMsgTimer--;

        // ------------------ 动画逻辑 ------------------
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

        // ------------------ 布局参数 ------------------
        const int PADDING = 40;
        int panelX = PADDING; int panelY = 120;
        int panelW = SCREEN_W - PADDING * 2; int panelH = SCREEN_H - panelY - PADDING;
        int contentX = panelX + PADDING; int contentW = panelW - PADDING * 2;

        int seedY = panelY + PADDING; int seedH = 50;
        int lenY = seedY + seedH + 30; int lenBtnSize = 40;
        int hintHeaderY = lenY + 60;
        int hintBtnSize = 30;
        int genBtnH = 60;
        int genBtnY = panelY + panelH - genBtnH - 20;
        int listY = hintHeaderY + 40;
        int listMaxH = genBtnY - listY - 20;
        int itemH = 45; int itemGap = 5;
        int maxVisibleHints = listMaxH / (itemH + itemGap);
        if (maxVisibleHints < 1) maxVisibleHints = 1;

        int lenMinusX = contentX + 160; int lenPlusX = lenMinusX + 160;
        int hintCtrlX = contentX + contentW - 200;
        int hintMinusX = hintCtrlX; int hintPlusX = hintCtrlX + 60;

        // ================= 输入交互逻辑 =================
        if (!showResult) {
            // 鼠标点击逻辑
            if (mouseClick) {
                if (mouseX >= contentX && mouseX <= contentX + contentW && mouseY >= seedY && mouseY <= seedY + seedH) focusTarget = FOCUS_SEED;
                if (mouseX >= lenMinusX && mouseX <= lenMinusX + lenBtnSize && mouseY >= lenY && mouseY <= lenY + lenBtnSize) if (passLen > 4) passLen--;
                if (mouseX >= lenPlusX && mouseX <= lenPlusX + lenBtnSize && mouseY >= lenY && mouseY <= lenY + lenBtnSize) if (passLen < 64) passLen++;
                if (mouseX >= hintMinusX && mouseX <= hintMinusX + hintBtnSize && mouseY >= hintHeaderY && mouseY <= hintHeaderY + hintBtnSize) {
                    if (hintCount > 1) { hintCount--; if (focusTarget == FOCUS_HINT && focusHintIndex >= hintCount) focusHintIndex = hintCount - 1; }
                }
                if (mouseX >= hintPlusX && mouseX <= hintPlusX + hintBtnSize && mouseY >= hintHeaderY && mouseY <= hintHeaderY + hintBtnSize) {
                    if (hintCount < maxVisibleHints) { hints[hintCount].text[0] = '\0'; hints[hintCount].len = 0; hintCount++; focusTarget = FOCUS_HINT; focusHintIndex = hintCount - 1; }
                }
                if (mouseX >= contentX && mouseX <= contentX + contentW && mouseY >= genBtnY && mouseY <= genBtnY + genBtnH) {
                    std::string all; for (int i = 0; i < hintCount; i++) { all += hints[i].text; all += "|"; }
                    GeneratePassword(seedText, all.c_str(), passLen, generatedPass, sizeof(generatedPass)); showResult = true;
                    saveMsgTimer = 0; // 重置保存提示
                }
                if (mouseX >= contentX && mouseX <= contentX + contentW && mouseY >= listY && mouseY <= listY + hintCount * (itemH + itemGap)) {
                    int idx = (mouseY - listY) / (itemH + itemGap);
                    if (idx >= 0 && idx < hintCount) { focusTarget = FOCUS_HINT; focusHintIndex = idx; }
                }
            }

            // ================= 键盘输入逻辑 (修复版) =================
            char* targetBuf = NULL;
            int* targetLen = NULL;
            int maxS = 0;

            // 确定当前输入焦点
            if (focusTarget == FOCUS_SEED) {
                targetBuf = seedText; targetLen = &seedLen; maxS = sizeof(seedText);
            }
            else {
                targetBuf = hints[focusHintIndex].text; targetLen = &hints[focusHintIndex].len; maxS = sizeof(hints[focusHintIndex].text);
            }

            // 使用 GetInputChar 处理所有输入
            char c;
            while ((c = GetInputChar(TRUE)) != 0) {
                // 处理退格键 (Backspace ASCII = 8)
                if (c == 8) {
                    if (*targetLen > 0) {
                        (*targetLen)--;
                        targetBuf[*targetLen] = '\0';
                    }
                }
                // 处理可打印字符 (ASCII 32空格 ~ 126波浪号)
                else if (c >= 32 && c <= 126) {
                    if (*targetLen < maxS - 1) {
                        targetBuf[*targetLen] = c;
                        (*targetLen)++;
                        targetBuf[*targetLen] = '\0';
                    }
                }
            }
        }

        // ================= 绘图 (Drawing) =================
        ClearDrawScreen();
        DrawBox(0, 0, SCREEN_W, SCREEN_H, COL_BG, TRUE);

        char tBuf[64] = { 0 };
        if (state <= STATE_BLANK_AFTER_JP) { for (int i = 0; i < visibleJPBytes; i++) tBuf[i] = titleJP[i]; }
        else { for (int i = 0; i < visibleENChars; i++) tBuf[i] = titleEN[i]; }
        DrawStringToHandle(panelX, 40, tBuf, COL_TEXT, fontTitle);

        DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, COL_PANEL, TRUE);
        DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, COL_BORDER, FALSE);

        // 1. Seed Input
        DrawStringToHandle(contentX, seedY - 25, "Seed Phrase (Master Key)", COL_PLACE, fontSmall);
        DrawBox(contentX, seedY, contentX + contentW, seedY + seedH, COL_INPUT_BG, TRUE);
        DrawBox(contentX, seedY, contentX + contentW, seedY + seedH, focusTarget == FOCUS_SEED ? COL_ACCENT : COL_BORDER, FALSE);
        int ty = seedY + (seedH - GetFontSizeToHandle(fontMain)) / 2;
        if (seedLen > 0) DrawStringToHandle(contentX + 10, ty, seedText, COL_TEXT, fontMain);
        else DrawStringToHandle(contentX + 10, ty, "Type your secret phrase...", COL_PLACE, fontMain);

        if (focusTarget == FOCUS_SEED && (cursorTimer / 60) % 2 == 0 && !showResult) {
            int tw = GetDrawStringWidthToHandle(seedText, seedLen, fontMain);
            DrawLine(contentX + 10 + tw, ty, contentX + 10 + tw, ty + 24, COL_ACCENT, 2);
        }

        // 2. Length Control
        DrawStringToHandle(contentX, lenY + 5, "Password Length", COL_PLACE, fontSmall);
        auto DrawBtn = [&](int x, int y, const char* t, bool active) {
            DrawBox(x, y, x + lenBtnSize, y + lenBtnSize, COL_INPUT_BG, TRUE);
            DrawBox(x, y, x + lenBtnSize, y + lenBtnSize, active ? COL_BORDER : COL_DISABLED, FALSE);
            int lw = GetDrawStringWidthToHandle(t, strlen(t), fontMain);
            DrawStringToHandle(x + (lenBtnSize - lw) / 2, y + (lenBtnSize - 24) / 2, t, active ? COL_TEXT : COL_DISABLED, fontMain);
            };
        DrawBtn(lenMinusX, lenY, "-", passLen > 4);
        DrawBtn(lenPlusX, lenY, "+", passLen < 64);

        char lb[16]; sprintf_s(lb, "%d", passLen);
        int lbW = GetDrawStringWidthToHandle(lb, strlen(lb), fontMedium);
        int lbH = GetFontSizeToHandle(fontMedium);
        int centerAreaStart = lenMinusX + lenBtnSize;
        int centerAreaWidth = lenPlusX - centerAreaStart;
        int lbX = centerAreaStart + (centerAreaWidth - lbW) / 2;
        int lbY = lenY + (lenBtnSize - lbH) / 2 + 2;
        DrawStringToHandle(lbX, lbY, lb, COL_ACCENT, fontMedium);

        // 3. Hint Header
        DrawStringToHandle(contentX, hintHeaderY, "Hints", COL_PLACE, fontSmall);
        char hcb[32];
        if (hintCount >= maxVisibleHints) sprintf_s(hcb, "Full (%d/%d)", hintCount, maxVisibleHints);
        else sprintf_s(hcb, "Count: %d", hintCount);
        DrawStringToHandle(hintPlusX + hintBtnSize + 10, hintHeaderY + 5, hcb, (hintCount >= maxVisibleHints) ? COL_ACCENT : COL_PLACE, fontSmall);

        DrawBtn(hintMinusX, hintHeaderY, "-", hintCount > 1);
        DrawBtn(hintPlusX, hintHeaderY, "+", hintCount < maxVisibleHints);

        // 4. Hint List
        for (int i = 0; i < hintCount; i++) {
            int y = listY + i * (itemH + itemGap);
            int hCol = (focusTarget == FOCUS_HINT && focusHintIndex == i) ? COL_ACCENT : COL_BORDER;

            DrawBox(contentX, y, contentX + contentW, y + itemH, COL_INPUT_BG, TRUE);
            DrawBox(contentX, y, contentX + contentW, y + itemH, hCol, FALSE);

            int hy = y + (itemH - GetFontSizeToHandle(fontMain)) / 2;
            if (hints[i].len > 0) DrawStringToHandle(contentX + 10, hy, hints[i].text, COL_TEXT, fontMain);
            else DrawStringToHandle(contentX + 10, hy, "e.g. Gmail, Steam...", COL_PLACE, fontMain);

            if (focusTarget == FOCUS_HINT && focusHintIndex == i && (cursorTimer / 60) % 2 == 0 && !showResult) {
                int tw = GetDrawStringWidthToHandle(hints[i].text, hints[i].len, fontMain);
                DrawLine(contentX + 10 + tw, hy, contentX + 10 + tw, hy + 24, COL_ACCENT, 2);
            }
        }

        // 5. Generate Button
        int genCol = COL_BORDER;
        if (mouseX >= contentX && mouseX <= contentX + contentW && mouseY >= genBtnY && mouseY <= genBtnY + genBtnH) genCol = COL_ACCENT;
        DrawBox(contentX, genBtnY, contentX + contentW, genBtnY + genBtnH, COL_INPUT_BG, TRUE);
        DrawBox(contentX, genBtnY, contentX + contentW, genBtnY + genBtnH, genCol, FALSE);
        DrawStringToHandle(contentX + (contentW - 200) / 2, genBtnY + 18, "GENERATE PASSWORD", genCol, fontMain);

        // ================= 结果弹窗 (Result Popup) =================
        if (showResult) {
            // 半透明遮罩
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 240); // 加深一点
            DrawBox(0, 0, SCREEN_W, SCREEN_H, GetColor(5, 5, 10), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            // 弹窗参数
            int rw = 700; int rh = 600;
            int rx = (SCREEN_W - rw) / 2;
            int ry = (SCREEN_H - rh) / 2;

            // 弹窗背景
            DrawBox(rx, ry, rx + rw, ry + rh, COL_PANEL, TRUE);
            DrawBox(rx, ry, rx + rw, ry + rh, COL_ACCENT, FALSE);

            // 标题
            DrawStringToHandle(rx + 40, ry + 40, "SECURE TOKEN GENERATED", COL_PLACE, fontSmall);

            // 密码显示框 (居中、大号)
            int passBoxH = 80;
            int passBoxY = ry + 80;
            DrawBox(rx + 40, passBoxY, rx + rw - 40, passBoxY + passBoxH, COL_INPUT_BG, TRUE);
            DrawBox(rx + 40, passBoxY, rx + rw - 40, passBoxY + passBoxH, COL_BORDER, FALSE);

            // 计算密码文字宽度以居中
            int passW = GetDrawStringWidthToHandle(generatedPass, strlen(generatedPass), fontMedium);
            DrawStringToHandle(rx + (rw - passW) / 2, passBoxY + (passBoxH - 40) / 2, generatedPass, COL_ACCENT, fontMedium);

            // 二维码
            int qrSize = 250;
            int qrY = passBoxY + passBoxH + 40;
            DrawFakeQRCode(rx + (rw - qrSize) / 2, qrY, qrSize, generatedPass);

            // 底部按钮区域
            int btnY = qrY + qrSize + 40;
            int btnW = 200;
            int btnH = 50;
            int gap = 40;
            int startX = rx + (rw - (btnW * 2 + gap)) / 2;
            int saveX = startX;
            int closeX = startX + btnW + gap;

            // Save Button
            bool hoverSave = (mouseX >= saveX && mouseX <= saveX + btnW && mouseY >= btnY && mouseY <= btnY + btnH);
            DrawBox(saveX, btnY, saveX + btnW, btnY + btnH, COL_INPUT_BG, TRUE);
            DrawBox(saveX, btnY, saveX + btnW, btnY + btnH, hoverSave ? COL_ACCENT : COL_BORDER, FALSE);

            const char* saveTxt = "SAVE IMAGE";
            int sw = GetDrawStringWidthToHandle(saveTxt, strlen(saveTxt), fontMain);
            DrawStringToHandle(saveX + (btnW - sw) / 2, btnY + 12, saveTxt, COL_TEXT, fontMain);

            // Close Button
            bool hoverClose = (mouseX >= closeX && mouseX <= closeX + btnW && mouseY >= btnY && mouseY <= btnY + btnH);
            DrawBox(closeX, btnY, closeX + btnW, btnY + btnH, COL_INPUT_BG, TRUE);
            DrawBox(closeX, btnY, closeX + btnW, btnY + btnH, hoverClose ? COL_WARNING : COL_BORDER, FALSE);

            const char* closeTxt = "CLOSE";
            int cw = GetDrawStringWidthToHandle(closeTxt, strlen(closeTxt), fontMain);
            DrawStringToHandle(closeX + (btnW - cw) / 2, btnY + 12, closeTxt, COL_TEXT, fontMain);

            // 保存成功的提示 (显示2秒)
            if (saveMsgTimer > 0) {
                const char* msg = "FILE SAVED SUCCESSFULLY!";
                int mw = GetDrawStringWidthToHandle(msg, strlen(msg), fontMain);
                DrawStringToHandle(rx + (rw - mw) / 2, btnY + btnH + 15, msg, COL_SUCCESS, fontMain);
            }

            // 处理结果界面的点击
            if (mouseClick) {
                // 点击 Save
                if (hoverSave) {
                    char filename[64];
                    // 使用时间戳防止覆盖
                    sprintf_s(filename, "Password_%ld.png", time(NULL));
                    // 临时隐藏鼠标，防止截图中出现鼠标
                    SetMouseDispFlag(FALSE);
                    ScreenFlip(); // 刷新一帧确保没有鼠标
                    SaveDrawScreenToPNG(0, 0, SCREEN_W, SCREEN_H, filename);
                    SetMouseDispFlag(TRUE);
                    saveMsgTimer = 120; // 显示2秒 (60fps * 2)
                }
                // 点击 Close
                if (hoverClose) {
                    showResult = false;
                }
            }
        }

        ScreenFlip();
    }

    DeleteFontToHandle(fontTitle); DeleteFontToHandle(fontMedium); DeleteFontToHandle(fontMain); DeleteFontToHandle(fontSmall);
    DxLib_End();
    return 0;
}