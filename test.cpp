#include "DxLib.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    ChangeWindowMode(TRUE);
    SetGraphMode(1920, 1080, 32);
    if (DxLib_Init() == -1) return -1;
    SetDrawScreen(DX_SCREEN_BACK);

    const int COL_BG = GetColor(0x23, 0x3D, 0x4D), COL_W = GetColor(255, 255, 255);
    int font = CreateFontToHandle(NULL, 36, 4, DX_FONTTYPE_ANTIALIASING_8X8);
    bool started = false;

    while (ProcessMessage() == 0) {
        ClearDrawScreen();
        DrawBox(0, 0, 428, 926, COL_BG, TRUE);

		// button
        int w = 220, h = 64, x = 214 - w / 2, y = 463 - h / 2;
        int mx, my; GetMousePoint(&mx, &my);
        int mouse = GetMouseInput();
        bool hover = (mx >= x && mx <= x + w && my >= y && my <= y + h);
        bool down = (mouse & MOUSE_INPUT_LEFT) != 0;
		
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, hover ? (down ? 90 : 70) : 46);
        DrawBox(x, y, x + w, y + h, COL_W, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        const char* txt = "Start";
        int tw = GetDrawStringWidthToHandle(txt, (int)strlen(txt), font);
        int th = GetFontSizeToHandle(font);
        DrawStringToHandle(x + (w - tw) / 2, y + (h - th) / 2, txt, COL_W, font);

		// key logic
        static bool last = false;
        if (hover && down && !last) started = true;
        last = down;

        if (started) DrawString(10, 10, "Started = true", COL_W);

        ScreenFlip();
    }
    DeleteFontToHandle(font);
    DxLib_End();
    
    return 0;
}