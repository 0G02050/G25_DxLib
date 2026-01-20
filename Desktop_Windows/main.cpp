#include "DxLib.h"
#include "Utils/AppStyle.h"      // スタイル・配色設定のインクルード
#include "Utils/UIComponents.h"  // UI描画コンポーネントのインクルード
#include "Utils/PasswordTools.h" // 暗号化コアロジックのインクルード
#include <string>
#include <stdio.h>
#include <time.h>

// ============================================================================
// データ構造定義
// ============================================================================
enum FocusTarget { FOCUS_SEED, FOCUS_HINT };
struct HintField { char text[128]; int len; };

// ============================================================================
// メインエントリーポイント (WinMain)
// ============================================================================
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    // 1. システムとウィンドウの初期化
    SetWindowSizeChangeEnableFlag(TRUE, TRUE);
    ChangeWindowMode(TRUE);
    SetGraphMode(AppStyle::SCREEN_W, AppStyle::SCREEN_H, 32);
    SetWindowSize(AppStyle::WIN_W, AppStyle::WIN_H);
    SetWindowText("ColdVault Generator");
    SetFullSceneAntiAliasingMode(4, 2);

    if (DxLib_Init() == -1) return -1;

    SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8);
    SetDrawScreen(DX_SCREEN_BACK);
    AppStyle::Colors.Init();

    // 2. リソース読み込み (フォント)
    int fontTitle = CreateFontToHandle("Roboto", 64, 4, DX_FONTTYPE_ANTIALIASING_EDGE);
    int fontMedium = CreateFontToHandle("Segoe UI", 32, 4, DX_FONTTYPE_ANTIALIASING_8X8);
    int fontMain = CreateFontToHandle("Segoe UI", 22, 4, DX_FONTTYPE_ANTIALIASING_8X8);
    int fontSmall = CreateFontToHandle("Segoe UI", 18, 4, DX_FONTTYPE_ANTIALIASING_8X8);
    int fontCode = CreateFontToHandle("Consolas", 26, 4, DX_FONTTYPE_ANTIALIASING_8X8);

    // 3. 実行時状態変数の初期化
    char seedText[256] = { 0 }; int seedLen = 0;
    int passLen = 16;
    HintField hints[16];
    int hintCount = 1;
    for (int i = 0; i < 16; i++) { hints[i].text[0] = '\0'; hints[i].len = 0; }

    FocusTarget focusTarget = FOCUS_SEED;
    int focusHintIndex = 0;
    int cursorTimer = 0;
    int saveMsgTimer = 0;
    int lastMouseInput = 0;

    bool showResult = false;
    char generatedPass[128] = { 0 };
    std::string cachedEncryptedData = "";

    // 管理者用ヒント編集状態
    bool isEditingPlaceholder = false;
    char tempPlaceholderInput[128] = { 0 };

    // ========================================================================
    // メインループ
    // ========================================================================
    while (ProcessMessage() == 0) {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;

        // Step 1: 入力状態の取得
        int mouseInput = GetMouseInput();
        int mouseX, mouseY; GetMousePoint(&mouseX, &mouseY);
        bool mouseDown = (mouseInput & MOUSE_INPUT_LEFT) != 0;
        bool mouseClick = (mouseDown && !(lastMouseInput & MOUSE_INPUT_LEFT));
        lastMouseInput = mouseInput;
        cursorTimer++;
        if (saveMsgTimer > 0) saveMsgTimer--;

        // --------------------------------------------------------------------
        // Step 2: 動的レイアウト計算 (主要修正エリア)
        // --------------------------------------------------------------------
        const int PADDING = 40;
        int panelX = PADDING, panelY = 140;
        int panelW = AppStyle::SCREEN_W - PADDING * 2, panelH = AppStyle::SCREEN_H - panelY - PADDING;
        int contentX = panelX + PADDING, contentW = panelW - PADDING * 2;

        int seedY = panelY + 40, seedH = 50;

        int lenY = seedY + seedH + 30, lenBtnSize = 40;
        int lenMinusX = contentX + 200, lenPlusX = lenMinusX + 140;

        int hintHeaderY = lenY + 50, hintBtnSize = 35;

        // 【レイアウト再構築】ヒント操作エリア：右から左へ配置 [ EDIT ] [ + ] [ テキスト ] [ - ]
        // 1. 最右端の EDIT ボタンの配置
        int editBtnW = 80, editBtnH = 30;
        int editBtnX = contentX + contentW - editBtnW; // コンテンツエリア右端に寄せる

        // 2. [+] ボタンの配置 (EDITの左側、間隔 20px)
        int hintPlusX = editBtnX - 20 - hintBtnSize;

        // 3. テキスト領域幅の確保
        int textAreaWidth = 100;

        // 4. [-] ボタンの配置 (テキスト領域の左側)
        int hintMinusX = hintPlusX - textAreaWidth - hintBtnSize;

        // マウスホバー判定
        bool hoverEdit = (mouseX >= editBtnX && mouseX <= editBtnX + editBtnW && mouseY >= hintHeaderY && mouseY <= hintHeaderY + editBtnH);
        bool hoverHintMinus = (mouseX >= hintMinusX && mouseX <= hintMinusX + hintBtnSize && mouseY >= hintHeaderY && mouseY <= hintHeaderY + hintBtnSize);
        bool hoverHintPlus = (mouseX >= hintPlusX && mouseX <= hintPlusX + hintBtnSize && mouseY >= hintHeaderY && mouseY <= hintHeaderY + hintBtnSize);

        int listY = hintHeaderY + 45, itemH = 45, itemGap = 6;
        int genBtnH = 60, genBtnY = panelY + panelH - genBtnH - 30;

        // --------------------------------------------------------------------
        // Step 3: インタラクションロジック処理
        // --------------------------------------------------------------------
        if (!showResult) {
            // [A] EDIT/SAVE ボタン処理
            if (mouseClick && hoverEdit) {
                if (!isEditingPlaceholder) {
                    isEditingPlaceholder = true;
                    strcpy_s(tempPlaceholderInput, sizeof(tempPlaceholderInput), SecureLogic::GetHintPlaceholder());
                    while (GetInputChar(TRUE) != 0);
                }
                else {
                    SecureLogic::SetHintPlaceholder(tempPlaceholderInput);
                    isEditingPlaceholder = false;
                }
            }

            // [B] 通常インタラクション
            if (mouseClick && !isEditingPlaceholder) {
                // Seed 入力フィールド
                if (mouseX >= contentX && mouseX <= contentX + contentW && mouseY >= seedY && mouseY <= seedY + seedH) focusTarget = FOCUS_SEED;
                // パスワード長
                if (mouseX >= lenMinusX && mouseX <= lenMinusX + lenBtnSize && mouseY >= lenY && mouseY <= lenY + lenBtnSize && passLen > 4) passLen--;
                if (mouseX >= lenPlusX && mouseX <= lenPlusX + lenBtnSize && mouseY >= lenY && mouseY <= lenY + lenBtnSize && passLen < 64) passLen++;
                // ヒント数
                if (hoverHintMinus && hintCount > 1) {
                    hintCount--; if (focusTarget == FOCUS_HINT && focusHintIndex >= hintCount) focusHintIndex = hintCount - 1;
                }
                if (hoverHintPlus && hintCount < 16) {
                    hints[hintCount].text[0] = '\0'; hints[hintCount].len = 0; hintCount++; focusTarget = FOCUS_HINT; focusHintIndex = hintCount - 1;
                }
                // 生成ボタン
                if (mouseX >= contentX && mouseX <= contentX + contentW && mouseY >= genBtnY && mouseY <= genBtnY + genBtnH) {
                    std::string all; for (int i = 0; i < hintCount; i++) { all += hints[i].text; all += "|"; }
                    SecureLogic::GeneratePassword(seedText, all.c_str(), passLen, generatedPass, sizeof(generatedPass));
                    cachedEncryptedData = SecureLogic::EncryptForQRCode(generatedPass);
                    showResult = true; saveMsgTimer = 0;
                }
                // リストフォーカス
                if (mouseX >= contentX && mouseX <= contentX + contentW && mouseY >= listY && mouseY <= listY + hintCount * (itemH + itemGap)) {
                    int idx = (mouseY - listY) / (itemH + itemGap);
                    if (idx >= 0 && idx < hintCount) { focusTarget = FOCUS_HINT; focusHintIndex = idx; }
                }
            }

            // [C] キーボード入力
            if (isEditingPlaceholder) {
                char c;
                while ((c = GetInputChar(TRUE)) != 0) {
                    int curLen = (int)strlen(tempPlaceholderInput);
                    if (c == 13) { SecureLogic::SetHintPlaceholder(tempPlaceholderInput); isEditingPlaceholder = false; }
                    else if (c == 8 && curLen > 0) tempPlaceholderInput[curLen - 1] = '\0';
                    else if (c >= 32 && c <= 126 && curLen < 120) { tempPlaceholderInput[curLen] = c; tempPlaceholderInput[curLen + 1] = '\0'; }
                }
            }
            else {
                char* targetBuf = (focusTarget == FOCUS_SEED) ? seedText : hints[focusHintIndex].text;
                int* targetLen = (focusTarget == FOCUS_SEED) ? &seedLen : &hints[focusHintIndex].len;
                int maxS = (focusTarget == FOCUS_SEED) ? sizeof(seedText) : sizeof(hints[focusHintIndex].text);
                char c;
                while ((c = GetInputChar(TRUE)) != 0) {
                    if (c == 8 && *targetLen > 0) { (*targetLen)--; targetBuf[*targetLen] = '\0'; }
                    else if (c >= 32 && c <= 126 && *targetLen < maxS - 1) { targetBuf[*targetLen] = c; (*targetLen)++; targetBuf[*targetLen] = '\0'; }
                }
            }
        }

        // --------------------------------------------------------------------
        // Step 4: 描画レンダリング (主要修正エリア)
        // --------------------------------------------------------------------
        ClearDrawScreen();
        DrawBox(0, 0, AppStyle::SCREEN_W, AppStyle::SCREEN_H, AppStyle::Colors.BG, TRUE);
        DrawFormatStringToHandle(panelX, 40, AppStyle::Colors.TEXT_MAIN, fontTitle, "ColdVault");
        DrawFormatStringToHandle(panelX + 5, 105, AppStyle::Colors.TEXT_SUB, fontMedium, "Offline Secure Password Generator");
        UI::DrawRoundedBorderAA(panelX, panelY, panelW, panelH, 16, 2, AppStyle::Colors.BORDER, AppStyle::Colors.CARD);

        // 1. Seed 入力フィールド
        DrawStringToHandle(contentX, seedY - 25, "Master Key Phrase", AppStyle::Colors.TEXT_SUB, fontSmall);
        int inputBorderCol = (focusTarget == FOCUS_SEED && !isEditingPlaceholder) ? AppStyle::Colors.ACCENT : AppStyle::Colors.BORDER;
        UI::DrawRoundedBorderAA(contentX, seedY, contentW, seedH, 8, 2, inputBorderCol, AppStyle::Colors.INPUT_BG);
        int ty = seedY + (seedH - GetFontSizeToHandle(fontMain)) / 2;
        if (seedLen > 0) DrawStringToHandle(contentX + 15, ty, seedText, AppStyle::Colors.TEXT_MAIN, fontMain);
        else DrawStringToHandle(contentX + 15, ty, "Type your secret phrase here...", AppStyle::Colors.TEXT_SUB, fontMain);
        if (focusTarget == FOCUS_SEED && (cursorTimer / 30) % 2 == 0 && !showResult && !isEditingPlaceholder) {
            int tw = GetDrawStringWidthToHandle(seedText, seedLen, fontMain);
            DrawLine(contentX + 15 + tw, ty, contentX + 15 + tw, ty + 24, AppStyle::Colors.ACCENT, 2);
        }

        // 2. Length (長さ) コントロール
        DrawStringToHandle(contentX, lenY + 8, "Password Length", AppStyle::Colors.TEXT_SUB, fontSmall);
        bool hoverLenMinus = (mouseX >= lenMinusX && mouseX <= lenMinusX + lenBtnSize && mouseY >= lenY && mouseY <= lenY + lenBtnSize);
        bool hoverLenPlus = (mouseX >= lenPlusX && mouseX <= lenPlusX + lenBtnSize && mouseY >= lenY && mouseY <= lenY + lenBtnSize);
        // 【微調整】最後の引数に3を追加し、+/-記号を3px下にずらして中央揃えにする
        UI::DrawModernButton(lenMinusX, lenY, lenBtnSize, lenBtnSize, "-", fontMain, hoverLenMinus, AppStyle::Colors.BORDER, AppStyle::Colors.TEXT_MAIN, 0, 0);
        UI::DrawModernButton(lenPlusX, lenY, lenBtnSize, lenBtnSize, "+", fontMain, hoverLenPlus, AppStyle::Colors.BORDER, AppStyle::Colors.TEXT_MAIN, 0, 0);
        char lb[16]; sprintf_s(lb, "%d", passLen);
        int lbW = GetDrawStringWidthToHandle(lb, strlen(lb), fontMedium);
        DrawStringToHandle(lenMinusX + lenBtnSize + (lenPlusX - (lenMinusX + lenBtnSize) - lbW) / 2, lenY - 4, lb, AppStyle::Colors.ACCENT, fontMedium);

        // 3. Hints (ヒント) コントロール
        DrawStringToHandle(contentX, hintHeaderY, "Context Hints", AppStyle::Colors.TEXT_SUB, fontSmall);

        // [-] ボタンの描画
        UI::DrawModernButton(hintMinusX, hintHeaderY, hintBtnSize, hintBtnSize, "-", fontSmall, hoverHintMinus, AppStyle::Colors.BORDER, AppStyle::Colors.TEXT_MAIN, 0, 0);

        // 中央の数量テキスト描画 (中央揃え)
        char hcb[32]; sprintf_s(hcb, "%d / 16", hintCount);
        int hcbW = GetDrawStringWidthToHandle(hcb, strlen(hcb), fontSmall);
        // [-] と [+] の間の中央位置を計算
        int textStartX = hintMinusX + hintBtnSize + (textAreaWidth - hcbW) / 2;
        DrawStringToHandle(textStartX, hintHeaderY + 5, hcb, AppStyle::Colors.TEXT_SUB, fontSmall);

        // [+] ボタンの描画
        UI::DrawModernButton(hintPlusX, hintHeaderY, hintBtnSize, hintBtnSize, "+", fontSmall, hoverHintPlus, AppStyle::Colors.BORDER, AppStyle::Colors.TEXT_MAIN, 0, 0);

        // [EDIT/SAVE] ボタンの描画
        const char* btnText = isEditingPlaceholder ? "SAVE" : "EDIT";
        int btnColor = isEditingPlaceholder ? AppStyle::Colors.ACCENT : AppStyle::Colors.BORDER;
        UI::DrawModernButton(editBtnX, hintHeaderY, editBtnW, editBtnH, btnText, fontSmall, hoverEdit, btnColor, AppStyle::Colors.TEXT_MAIN, 0);

        // 4. Hints リスト
        for (int i = 0; i < hintCount; i++) {
            int y = listY + i * (itemH + itemGap);
            bool isFocus = (focusTarget == FOCUS_HINT && focusHintIndex == i);
            if (isFocus) UI::DrawRoundedBorderAA(contentX, y, contentW, itemH, 6, 2, AppStyle::Colors.ACCENT, AppStyle::Colors.INPUT_BG);
            else UI::DrawRoundedBoxAA(contentX, y, contentW, itemH, 6, AppStyle::Colors.INPUT_BG);
            int hy = y + (itemH - GetFontSizeToHandle(fontMain)) / 2;
            if (hints[i].len > 0) {
                DrawStringToHandle(contentX + 15, hy, hints[i].text, AppStyle::Colors.TEXT_MAIN, fontMain);
            }
            else {
                if (isEditingPlaceholder) {
                    DrawStringToHandle(contentX + 15, hy, tempPlaceholderInput, AppStyle::Colors.ACCENT, fontMain);
                    int tw = GetDrawStringWidthToHandle(tempPlaceholderInput, strlen(tempPlaceholderInput), fontMain);
                    if ((cursorTimer / 30) % 2 == 0) DrawLine(contentX + 15 + tw, hy, contentX + 15 + tw, hy + 24, AppStyle::Colors.ACCENT, 2);
                }
                else {
                    DrawStringToHandle(contentX + 15, hy, SecureLogic::GetHintPlaceholder(), AppStyle::Colors.TEXT_SUB, fontMain);
                }
            }
            if (isFocus && !isEditingPlaceholder && (cursorTimer / 30) % 2 == 0 && !showResult) {
                int tw = GetDrawStringWidthToHandle(hints[i].text, hints[i].len, fontMain);
                DrawLine(contentX + 15 + tw, hy, contentX + 15 + tw, hy + 24, AppStyle::Colors.ACCENT, 2);
            }
        }

        // 5. 生成ボタン
        bool hoverGen = (mouseX >= contentX && mouseX <= contentX + contentW && mouseY >= genBtnY && mouseY <= genBtnY + genBtnH);
        UI::DrawModernButton(contentX, genBtnY, contentW, genBtnH, "GENERATE PASSWORD", fontMain, hoverGen, AppStyle::Colors.ACCENT, AppStyle::Colors.TEXT_MAIN, 0);

        // --------------------------------------------------------------------
        // Step 5: 結果ポップアップ
        // --------------------------------------------------------------------
        if (showResult) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200); DrawBox(0, 0, AppStyle::SCREEN_W, AppStyle::SCREEN_H, GetColor(0, 0, 0), TRUE); SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            int rw = 640, rh = 640, rx = (AppStyle::SCREEN_W - rw) / 2, ry = (AppStyle::SCREEN_H - rh) / 2;
            UI::DrawRoundedBorderAA(rx, ry, rw, rh, 16, 2, AppStyle::Colors.BORDER, AppStyle::Colors.CARD);
            DrawStringToHandle(rx + 40, ry + 30, "Generated Securely", AppStyle::Colors.TEXT_SUB, fontSmall);
            int passBoxH = 80, passBoxY = ry + 60;
            UI::DrawRoundedBoxAA(rx + 40, passBoxY, rw - 80, passBoxH, 12, AppStyle::Colors.INPUT_BG);
            int passW = GetDrawStringWidthToHandle(generatedPass, strlen(generatedPass), fontCode);
            DrawStringToHandle(rx + (rw - passW) / 2, passBoxY + (passBoxH - 30) / 2, generatedPass, AppStyle::Colors.ACCENT, fontCode);
            int qrSize = 280, qrY = passBoxY + passBoxH + 30, qrX = rx + (rw - qrSize) / 2;
            UI::DrawRealQRCode(qrX, qrY, qrSize, cachedEncryptedData.c_str());
            DrawStringToHandle(qrX + 40, qrY + qrSize + 10, "Scan with ColdVault App", AppStyle::Colors.TEXT_SUB, fontSmall);

            int btnY = qrY + qrSize + 50, btnW = 160, btnH = 50, gap = 20;
            int startX = rx + (rw - (btnW * 2 + gap)) / 2;
            bool hoverSave = (mouseX >= startX && mouseX <= startX + btnW && mouseY >= btnY && mouseY <= btnY + btnH);
            // 【微調整】オフセット2を追加
            UI::DrawModernButton(startX, btnY, btnW, btnH, "SAVE IMAGE", fontMain, hoverSave, AppStyle::Colors.BORDER, AppStyle::Colors.TEXT_MAIN, 2);
            int closeX = startX + btnW + gap;
            bool hoverClose = (mouseX >= closeX && mouseX <= closeX + btnW && mouseY >= btnY && mouseY <= btnY + btnH);
            // 【微調整】オフセット2を追加
            UI::DrawModernButton(closeX, btnY, btnW, btnH, "CLOSE", fontMain, hoverClose, AppStyle::Colors.WARNING, AppStyle::Colors.TEXT_MAIN, 2);

            if (saveMsgTimer > 0) {
                const char* msg = "Saved to folder!"; int mw = GetDrawStringWidthToHandle(msg, strlen(msg), fontMain);
                DrawStringToHandle(rx + (rw - mw) / 2, btnY + btnH + 15, msg, AppStyle::Colors.SUCCESS, fontMain);
            }
            if (mouseClick) {
                if (hoverSave) { char fn[64]; sprintf_s(fn, "Pass_%ld.png", time(NULL)); SetMouseDispFlag(FALSE); ScreenFlip(); SaveDrawScreenToPNG(0, 0, AppStyle::SCREEN_W, AppStyle::SCREEN_H, fn); SetMouseDispFlag(TRUE); saveMsgTimer = 120; }
                if (hoverClose) showResult = false;
            }
        }
        ScreenFlip();
    }
    DeleteFontToHandle(fontTitle); DeleteFontToHandle(fontMedium); DeleteFontToHandle(fontMain); DeleteFontToHandle(fontSmall); DeleteFontToHandle(fontCode);
    DxLib_End(); return 0;
}