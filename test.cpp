#include "DxLib.h"
#include <string.h>
#include <stdio.h>

// タイトル画面のステート（状態）
enum TitleState {
    STATE_TYPE_JP,          // 日文打字中
    STATE_SHOW_JP,          // 日文完整显示
    STATE_DELETE_JP,        // 日文逐字删除
    STATE_BLANK_AFTER_JP,   // 日文删完后的空白

    STATE_TYPE_EN,          // 英文打字中
    STATE_SHOW_EN,          // 英文完整显示
    STATE_DELETE_EN,        // 英文逐字删除
    STATE_BLANK_AFTER_EN    // 英文删完后的空白
};

// 输入焦点是哪一块
enum FocusTarget {
    FOCUS_SEED,             // Seed phrase 输入框
    FOCUS_HINT              // 某一行 Hint 输入
};

// 一行 Hint 的数据
struct HintField {
    char text[128];
    int  len;
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

    const int SCREEN_W = 1920;
    const int SCREEN_H = 1080;

    SetWindowSizeExtendRate(0.9f);

    ChangeWindowMode(TRUE);
    SetGraphMode(SCREEN_W, SCREEN_H, 32);

    // 如果你觉得窗口太大，这行可以缩放窗口显示，不影响内部布局：
    // SetWindowSizeExtendRate(0.7f);

    if (DxLib_Init() == -1) return -1;
    SetDrawScreen(DX_SCREEN_BACK);

    // ========= 颜色 =========
    const int COL_BG = GetColor(10, 12, 16);       // 背景
    const int COL_PANEL = GetColor(34, 36, 56);       // 面板
    const int COL_BORDER = GetColor(110, 120, 150);
    const int COL_ACCENT = GetColor(140, 210, 255);
    const int COL_TEXT = GetColor(230, 235, 240);
    const int COL_PLACE = GetColor(160, 170, 190);
    const int COL_W = GetColor(255, 255, 255);
    const int COL_INPUT_BG = GetColor(0, 0, 0);
    const int COL_SCROLL_BG = GetColor(60, 65, 85);
    const int COL_SCROLL_BAR = GetColor(140, 150, 190);

    // ========= 字体 =========
    int fontTitle = CreateFontToHandle(NULL, 96, 4, DX_FONTTYPE_ANTIALIASING_8X8);
    int fontMain = CreateFontToHandle(NULL, 32, 4, DX_FONTTYPE_ANTIALIASING_8X8);
    int fontSmall = CreateFontToHandle(NULL, 24, 4, DX_FONTTYPE_ANTIALIASING_8X8);

    int titleFontSize = GetFontSizeToHandle(fontTitle);

    // ========= 标题动画 =========
    const char* titleJP = "オフライン暗号室";
    const char* titleEN = "ColdVault";

    int lenJPBytes = strlen(titleJP);
    int lenENChars = strlen(titleEN);

    TitleState state = STATE_TYPE_JP;
    int frameCounter = 0;
    int waitCounter = 0;
    int visibleJPBytes = 0;
    int visibleENChars = 0;

    // 5 秒 = 300 帧（60fps）
    const int STEP_JP = 6;
    const int STEP_EN = 5;
    const int STEP_DEL_JP = 4;
    const int STEP_DEL_EN = 3;
    const int WAIT_SHOW_JP = 300;
    const int WAIT_SHOW_EN = 300;
    const int WAIT_BLANK = 20;

    int cursorFrameTitle = 0;
    const int CURSOR_INTERVAL = 30;

    // ========= Seed 输入 =============
    char seedText[256] = { 0 };
    int seedLen = 0;

    int wordCount = 5;
    const int MIN_WORD = 1;
    const int MAX_WORD = 16;

    // ========= Hint 列表（可滚动） =========
    const int MAX_HINTS = 64;
    HintField hints[MAX_HINTS];
    int hintCount = 1;

    for (int i = 0; i < MAX_HINTS; i++) {
        hints[i].text[0] = '\0';
        hints[i].len = 0;
    }

    int hintScrollOffset = 0;
    int hintScrollMax = 0;
    const int HINT_SCROLL_STEP = 40;

    // ========= 输入焦点 =========
    FocusTarget focusTarget = FOCUS_SEED;
    int focusHintIndex = 0;

    int lastMouseInput = 0;
    char keyState[256] = { 0 };
    char prevKeyState[256] = { 0 };
    int cursorFrameInput = 0;
    while (ProcessMessage() == 0) {

        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;

        memcpy(prevKeyState, keyState, 256);
        GetHitKeyStateAll(keyState);

        int mouseInput = GetMouseInput();
        int mouseX, mouseY;
        GetMousePoint(&mouseX, &mouseY);

        bool mouseDown = (mouseInput & MOUSE_INPUT_LEFT) != 0;
        bool mouseLast = (lastMouseInput & MOUSE_INPUT_LEFT) != 0;
        bool mouseClick = (mouseDown && !mouseLast);
        lastMouseInput = mouseInput;

        int wheel = GetMouseWheelRotVol();
        if (wheel != 0) {
            hintScrollOffset -= wheel * HINT_SCROLL_STEP;
            if (hintScrollOffset < 0) hintScrollOffset = 0;
            if (hintScrollOffset > hintScrollMax) hintScrollOffset = hintScrollMax;
        }

        frameCounter++;
        cursorFrameTitle++;

        // ================= 标题动画逻辑 =================
        switch (state) {

        case STATE_TYPE_JP:
            if (frameCounter >= STEP_JP) {
                frameCounter = 0;
                if (visibleJPBytes < lenJPBytes) {
                    visibleJPBytes += 2;
                    if (visibleJPBytes > lenJPBytes) visibleJPBytes = lenJPBytes;
                }
                else {
                    state = STATE_SHOW_JP;
                    waitCounter = 0;
                }
            }
            break;

        case STATE_SHOW_JP:
            if (++waitCounter > WAIT_SHOW_JP) {
                waitCounter = 0;
                frameCounter = 0;
                state = STATE_DELETE_JP;
            }
            break;

        case STATE_DELETE_JP:
            if (frameCounter >= STEP_DEL_JP) {
                frameCounter = 0;
                if (visibleJPBytes > 0) {
                    visibleJPBytes -= 2;
                    if (visibleJPBytes < 0) visibleJPBytes = 0;
                }
                else {
                    state = STATE_BLANK_AFTER_JP;
                    waitCounter = 0;
                }
            }
            break;

        case STATE_BLANK_AFTER_JP:
            if (++waitCounter > WAIT_BLANK) {
                waitCounter = 0;
                frameCounter = 0;
                visibleENChars = 0;
                state = STATE_TYPE_EN;
            }
            break;

        case STATE_TYPE_EN:
            if (frameCounter >= STEP_EN) {
                frameCounter = 0;
                if (visibleENChars < lenENChars) {
                    visibleENChars++;
                }
                else {
                    state = STATE_SHOW_EN;
                    waitCounter = 0;
                }
            }
            break;

        case STATE_SHOW_EN:
            if (++waitCounter > WAIT_SHOW_EN) {
                waitCounter = 0;
                frameCounter = 0;
                state = STATE_DELETE_EN;
            }
            break;

        case STATE_DELETE_EN:
            if (frameCounter >= STEP_DEL_EN) {
                frameCounter = 0;
                if (visibleENChars > 0) {
                    visibleENChars--;
                }
                else {
                    state = STATE_BLANK_AFTER_EN;
                    waitCounter = 0;
                }
            }
            break;

        case STATE_BLANK_AFTER_EN:
            if (++waitCounter > WAIT_BLANK) {
                waitCounter = 0;
                frameCounter = 0;
                cursorFrameTitle = 0;
                visibleJPBytes = 0;
                state = STATE_TYPE_JP;
            }
            break;
        }

        // ================= 布局计算 =================

        const int titleX = 40;
        const int titleY = 40;
        int titleBottom = titleY + titleFontSize;

        const int marginSide = 40;
        const int marginBottom = 60;

        int panelX = marginSide;
        int panelW = SCREEN_W - marginSide * 2;
        int panelY = titleBottom + 40;
        int panelH = SCREEN_H - panelY - marginBottom;

        int paddingX = 40;
        int paddingY = 32;

        int seedLabelX = panelX + paddingX;
        int seedLabelY = panelY + paddingY;

        int seedInputX = seedLabelX;
        int seedInputY = seedLabelY + 40;
        int seedInputW = panelW - paddingX * 2;
        int seedInputH = 64;

        int wordsLabelX = seedLabelX;
        int wordsLabelY = seedInputY + seedInputH + 40;

        int minusX = wordsLabelX + 120;
        int minusY = wordsLabelY - 6;
        int minusW = 48;
        int minusH = 40;

        int plusX = minusX + 200;
        int plusY = minusY;
        int plusW = 48;
        int plusH = 40;

        int valueX = minusX + minusW + 20;
        int valueY = wordsLabelY;

        int hintsHeaderY = wordsLabelY + 60;
        int hintsLabelX = wordsLabelX;
        int hintsLabelY = hintsHeaderY;

        int hintsAddBtnW = 64;
        int hintsAddBtnH = 32;
        int hintsAddBtnX = panelX + panelW - paddingX - hintsAddBtnW;
        int hintsAddBtnY = hintsHeaderY - 4;

        int hintsViewX = panelX + paddingX;
        int hintsViewY = hintsHeaderY + 32;
        int hintsViewW = panelW - paddingX * 2;
        int hintsViewH = panelY + panelH - paddingY - hintsViewY - 32;

        const int hintRowH = 48;
        const int hintRowGap = 16;
        const int hintRowFullH = hintRowH + hintRowGap;

        int hintContentH = hintCount * hintRowFullH;

        if (hintContentH > hintsViewH) {
            hintScrollMax = hintContentH - hintsViewH;
            if (hintScrollOffset > hintScrollMax) hintScrollOffset = hintScrollMax;
        }
        else {
            hintScrollOffset = 0;
            hintScrollMax = 0;
        }

        int helpTextX = panelX + paddingX;
        int helpTextY = panelY + panelH - paddingY - 24;

        // ================= 鼠标焦点切换 =================
        if (mouseClick) {
            if (mouseX >= seedInputX && mouseX <= seedInputX + seedInputW &&
                mouseY >= seedInputY && mouseY <= seedInputY + seedInputH) {
                focusTarget = FOCUS_SEED;
            }
            else if (mouseX >= hintsAddBtnX && mouseX <= hintsAddBtnX + hintsAddBtnW &&
                mouseY >= hintsAddBtnY && mouseY <= hintsAddBtnY + hintsAddBtnH) {

                if (hintCount < MAX_HINTS) {
                    hints[hintCount].text[0] = '\0';
                    hints[hintCount].len = 0;
                    hintCount++;

                    hintContentH = hintCount * hintRowFullH;
                    if (hintContentH > hintsViewH) {
                        hintScrollMax = hintContentH - hintsViewH;
                        hintScrollOffset = hintScrollMax;
                    }

                    focusTarget = FOCUS_HINT;
                    focusHintIndex = hintCount - 1;
                }
            }
            else if (mouseX >= hintsViewX && mouseX <= hintsViewX + hintsViewW &&
                mouseY >= hintsViewY && mouseY <= hintsViewY + hintsViewH) {

                int localY = mouseY - hintsViewY + hintScrollOffset;
                int idx = localY / hintRowFullH;

                if (idx >= 0 && idx < hintCount) {
                    focusTarget = FOCUS_HINT;
                    focusHintIndex = idx;
                }
            }
        }

        // ================= 文本输入 =================

        char* activeBuf;
        int* activeLen;
        int   activeMaxSize;

        if (focusTarget == FOCUS_SEED) {
            activeBuf = seedText;
            activeLen = &seedLen;
            activeMaxSize = sizeof(seedText);
        }
        else {
            activeBuf = hints[focusHintIndex].text;
            activeLen = &hints[focusHintIndex].len;
            activeMaxSize = sizeof(hints[focusHintIndex].text);
        }

        auto pushChar = [&](char c) {
            if (*activeLen < activeMaxSize - 1) {
                activeBuf[*activeLen] = c;
                (*activeLen)++;
                activeBuf[*activeLen] = '\0';
            }
            };

        // Backspace
        if (keyState[KEY_INPUT_BACK] && !prevKeyState[KEY_INPUT_BACK]) {
            if (*activeLen > 0) {
                (*activeLen)--;
                activeBuf[*activeLen] = '\0';
            }
        }

        // 空格
        if (keyState[KEY_INPUT_SPACE] && !prevKeyState[KEY_INPUT_SPACE]) {
            pushChar(' ');
        }

        // 数字 0–9
        if (keyState[KEY_INPUT_0] && !prevKeyState[KEY_INPUT_0]) pushChar('0');
        if (keyState[KEY_INPUT_1] && !prevKeyState[KEY_INPUT_1]) pushChar('1');
        if (keyState[KEY_INPUT_2] && !prevKeyState[KEY_INPUT_2]) pushChar('2');
        if (keyState[KEY_INPUT_3] && !prevKeyState[KEY_INPUT_3]) pushChar('3');
        if (keyState[KEY_INPUT_4] && !prevKeyState[KEY_INPUT_4]) pushChar('4');
        if (keyState[KEY_INPUT_5] && !prevKeyState[KEY_INPUT_5]) pushChar('5');
        if (keyState[KEY_INPUT_6] && !prevKeyState[KEY_INPUT_6]) pushChar('6');
        if (keyState[KEY_INPUT_7] && !prevKeyState[KEY_INPUT_7]) pushChar('7');
        if (keyState[KEY_INPUT_8] && !prevKeyState[KEY_INPUT_8]) pushChar('8');
        if (keyState[KEY_INPUT_9] && !prevKeyState[KEY_INPUT_9]) pushChar('9');

        // 字母 A–Z（转小写）
        if (keyState[KEY_INPUT_A] && !prevKeyState[KEY_INPUT_A]) pushChar('a');
        if (keyState[KEY_INPUT_B] && !prevKeyState[KEY_INPUT_B]) pushChar('b');
        if (keyState[KEY_INPUT_C] && !prevKeyState[KEY_INPUT_C]) pushChar('c');
        if (keyState[KEY_INPUT_D] && !prevKeyState[KEY_INPUT_D]) pushChar('d');
        if (keyState[KEY_INPUT_E] && !prevKeyState[KEY_INPUT_E]) pushChar('e');
        if (keyState[KEY_INPUT_F] && !prevKeyState[KEY_INPUT_F]) pushChar('f');
        if (keyState[KEY_INPUT_G] && !prevKeyState[KEY_INPUT_G]) pushChar('g');
        if (keyState[KEY_INPUT_H] && !prevKeyState[KEY_INPUT_H]) pushChar('h');
        if (keyState[KEY_INPUT_I] && !prevKeyState[KEY_INPUT_I]) pushChar('i');
        if (keyState[KEY_INPUT_J] && !prevKeyState[KEY_INPUT_J]) pushChar('j');
        if (keyState[KEY_INPUT_K] && !prevKeyState[KEY_INPUT_K]) pushChar('k');
        if (keyState[KEY_INPUT_L] && !prevKeyState[KEY_INPUT_L]) pushChar('l');
        if (keyState[KEY_INPUT_M] && !prevKeyState[KEY_INPUT_M]) pushChar('m');
        if (keyState[KEY_INPUT_N] && !prevKeyState[KEY_INPUT_N]) pushChar('n');
        if (keyState[KEY_INPUT_O] && !prevKeyState[KEY_INPUT_O]) pushChar('o');
        if (keyState[KEY_INPUT_P] && !prevKeyState[KEY_INPUT_P]) pushChar('p');
        if (keyState[KEY_INPUT_Q] && !prevKeyState[KEY_INPUT_Q]) pushChar('q');
        if (keyState[KEY_INPUT_R] && !prevKeyState[KEY_INPUT_R]) pushChar('r');
        if (keyState[KEY_INPUT_S] && !prevKeyState[KEY_INPUT_S]) pushChar('s');
        if (keyState[KEY_INPUT_T] && !prevKeyState[KEY_INPUT_T]) pushChar('t');
        if (keyState[KEY_INPUT_U] && !prevKeyState[KEY_INPUT_U]) pushChar('u');
        if (keyState[KEY_INPUT_V] && !prevKeyState[KEY_INPUT_V]) pushChar('v');
        if (keyState[KEY_INPUT_W] && !prevKeyState[KEY_INPUT_W]) pushChar('w');
        if (keyState[KEY_INPUT_X] && !prevKeyState[KEY_INPUT_X]) pushChar('x');
        if (keyState[KEY_INPUT_Y] && !prevKeyState[KEY_INPUT_Y]) pushChar('y');
        if (keyState[KEY_INPUT_Z] && !prevKeyState[KEY_INPUT_Z]) pushChar('z');

        cursorFrameInput++;

        // Words 变化
        if (mouseClick &&
            mouseX >= minusX && mouseX <= minusX + minusW &&
            mouseY >= minusY && mouseY <= minusY + minusH) {
            if (wordCount > MIN_WORD) wordCount--;
        }

        if (mouseClick &&
            mouseX >= plusX && mouseX <= plusX + plusW &&
            mouseY >= plusY && mouseY <= plusY + plusH) {
            if (wordCount < MAX_WORD) wordCount++;
        }

        // ================= 绘制画面 =================

        ClearDrawScreen();
        DrawBox(0, 0, SCREEN_W, SCREEN_H, COL_BG, TRUE);

        // 标题动画
        {
            char buf[64] = { 0 };
            bool cursorOn = ((cursorFrameTitle / CURSOR_INTERVAL) % 2 == 0);

            bool isJP =
                state == STATE_TYPE_JP ||
                state == STATE_SHOW_JP ||
                state == STATE_DELETE_JP;

            if (isJP) {
                int n = visibleJPBytes;
                for (int i = 0; i < n; i++) buf[i] = titleJP[i];

                DrawStringToHandle(titleX, titleY, buf, COL_W, fontTitle);

                if (cursorOn) {
                    int tw = GetDrawStringWidthToHandle(buf, strlen(buf), fontTitle);
                    DrawBox(titleX + tw + 8, titleY,
                        titleX + tw + 16, titleY + titleFontSize,
                        COL_W, TRUE);
                }
            }
            else {
                int n = visibleENChars;
                for (int i = 0; i < n; i++) buf[i] = titleEN[i];

                DrawStringToHandle(titleX, titleY, buf, COL_W, fontTitle);

                if (cursorOn) {
                    int tw = GetDrawStringWidthToHandle(buf, strlen(buf), fontTitle);
                    DrawBox(titleX + tw + 8, titleY,
                        titleX + tw + 16, titleY + titleFontSize,
                        COL_W, TRUE);
                }
            }
        }

        // 主面板
        DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, COL_PANEL, TRUE);
        DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, COL_BORDER, FALSE);

        // Seed phrase
        DrawStringToHandle(seedLabelX, seedLabelY, "Seed phrase", COL_PLACE, fontSmall);

        int seedBorderColor = (focusTarget == FOCUS_SEED ? COL_ACCENT : COL_BORDER);
        DrawBox(seedInputX, seedInputY, seedInputX + seedInputW, seedInputY + seedInputH,
            COL_INPUT_BG, TRUE);
        DrawBox(seedInputX, seedInputY, seedInputX + seedInputW, seedInputY + seedInputH,
            seedBorderColor, FALSE);

        int stx = seedInputX + 16;
        int sty = seedInputY + (seedInputH - GetFontSizeToHandle(fontMain)) / 2;

        if (seedLen > 0)
            DrawStringToHandle(stx, sty, seedText, COL_TEXT, fontMain);
        else
            DrawStringToHandle(stx, sty, "type your seed words here...", COL_PLACE, fontMain);

        if (focusTarget == FOCUS_SEED) {
            bool cursorOn = (cursorFrameInput / 30) % 2 == 0;
            if (cursorOn) {
                int tw = seedLen > 0
                    ? GetDrawStringWidthToHandle(seedText, seedLen, fontMain)
                    : 0;

                DrawBox(stx + tw + 4, sty - 4, stx + tw + 6, sty + GetFontSizeToHandle(fontMain),
                    COL_TEXT, TRUE);
            }
        }

        // Words 行
        DrawStringToHandle(wordsLabelX, wordsLabelY, "Words", COL_PLACE, fontSmall);

        int mColor = COL_BORDER;
        if (mouseX >= minusX && mouseX <= minusX + minusW &&
            mouseY >= minusY && mouseY <= minusY + minusH)
            mColor = COL_ACCENT;

        DrawBox(minusX, minusY, minusX + minusW, minusY + minusH, COL_INPUT_BG, TRUE);
        DrawBox(minusX, minusY, minusX + minusW, minusY + minusH, mColor, FALSE);
        DrawStringToHandle(minusX + 16, minusY + 8, "-", mColor, fontMain);

        int pColor = COL_BORDER;
        if (mouseX >= plusX && mouseX <= plusX + plusW &&
            mouseY >= plusY && mouseY <= plusY + plusH)
            pColor = COL_ACCENT;

        DrawBox(plusX, plusY, plusX + plusW, plusY + plusH, COL_INPUT_BG, TRUE);
        DrawBox(plusX, plusY, plusX + plusW, plusY + plusH, pColor, FALSE);
        DrawStringToHandle(plusX + 14, plusY + 8, "+", pColor, fontMain);

        char countBuf[16];
        sprintf_s(countBuf, sizeof(countBuf), "%d", wordCount);
        DrawStringToHandle(valueX, valueY, countBuf, COL_TEXT, fontMain);

        // Hints 标题 + 添加按钮
        DrawStringToHandle(hintsLabelX, hintsLabelY, "Hints", COL_PLACE, fontSmall);

        int addColor = COL_BORDER;
        if (mouseX >= hintsAddBtnX && mouseX <= hintsAddBtnX + hintsAddBtnW &&
            mouseY >= hintsAddBtnY && mouseY <= hintsAddBtnY + hintsAddBtnH)
            addColor = COL_ACCENT;

        DrawBox(hintsAddBtnX, hintsAddBtnY,
            hintsAddBtnX + hintsAddBtnW, hintsAddBtnY + hintsAddBtnH,
            COL_INPUT_BG, TRUE);
        DrawBox(hintsAddBtnX, hintsAddBtnY,
            hintsAddBtnX + hintsAddBtnW, hintsAddBtnY + hintsAddBtnH,
            addColor, FALSE);
        {
            const char* sym = "+";
            int symW = GetDrawStringWidthToHandle(sym, 1, fontMain);
            int symH = GetFontSizeToHandle(fontMain);
            int sx = hintsAddBtnX + (hintsAddBtnW - symW) / 2;
            int sy = hintsAddBtnY + (hintsAddBtnH - symH) / 2;
            DrawStringToHandle(sx, sy, sym, addColor, fontMain);
        }

        // Hint 滚动区域背景
        DrawBox(hintsViewX, hintsViewY,
            hintsViewX + hintsViewW, hintsViewY + hintsViewH,
            COL_INPUT_BG, TRUE);

        // 绘制 Hint 行
        for (int i = 0; i < hintCount; i++) {
            int rowY = hintsViewY + i * hintRowFullH - hintScrollOffset;

            if (rowY + hintRowH < hintsViewY) continue;
            if (rowY > hintsViewY + hintsViewH) continue;

            int rowX = hintsViewX;
            int rowW = hintsViewW - 20; // scroll bar space

            int borderColor =
                (focusTarget == FOCUS_HINT && focusHintIndex == i)
                ? COL_ACCENT
                : COL_BORDER;

            DrawBox(rowX, rowY, rowX + rowW, rowY + hintRowH,
                COL_BG, TRUE);
            DrawBox(rowX, rowY, rowX + rowW, rowY + hintRowH,
                borderColor, FALSE);

            int tx = rowX + 12;
            int ty = rowY + (hintRowH - GetFontSizeToHandle(fontMain)) / 2;

            if (hints[i].len > 0)
                DrawStringToHandle(tx, ty, hints[i].text, COL_TEXT, fontMain);
            else
                DrawStringToHandle(tx, ty, "hint word...", COL_PLACE, fontMain);

            if (focusTarget == FOCUS_HINT && focusHintIndex == i) {
                bool cursorOn = (cursorFrameInput / 30) % 2 == 0;
                if (cursorOn) {
                    int tw = hints[i].len > 0
                        ? GetDrawStringWidthToHandle(hints[i].text, hints[i].len, fontMain)
                        : 0;

                    DrawBox(tx + tw + 4, ty - 4,
                        tx + tw + 6, ty + GetFontSizeToHandle(fontMain),
                        COL_TEXT, TRUE);
                }
            }
        }

        // 滚动条
        if (hintScrollMax > 0) {
            int barX1 = hintsViewX + hintsViewW - 12;
            int barX2 = hintsViewX + hintsViewW - 4;
            int barY1 = hintsViewY;
            int barY2 = hintsViewY + hintsViewH;

            DrawBox(barX1, barY1, barX2, barY2, COL_SCROLL_BG, TRUE);

            double ratio = (double)hintsViewH / hintContentH;
            if (ratio < 0.1) ratio = 0.1;

            int thumbH = (int)(hintsViewH * ratio);
            double posRatio = (double)hintScrollOffset / hintScrollMax;

            int thumbY1 = hintsViewY + (int)((hintsViewH - thumbH) * posRatio);
            int thumbY2 = thumbY1 + thumbH;

            DrawBox(barX1, thumbY1, barX2, thumbY2, COL_SCROLL_BAR, TRUE);
        }

        DrawStringToHandle(helpTextX, helpTextY,
            "ESC: quit | Click to select input | [+] add hint | Mouse wheel: scroll | [-][+]: word count",
            COL_PLACE, fontSmall);

        ScreenFlip();
        WaitTimer(16);
    }

    DeleteFontToHandle(fontTitle);
    DeleteFontToHandle(fontMain);
    DeleteFontToHandle(fontSmall);
    DxLib_End();
    return 0;
}