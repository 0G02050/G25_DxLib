#include "PasswordTools.h"
#include <random>
#include <string.h>
#include <stdio.h>
#include <functional> // hash用

namespace SecureLogic {
    // 内部キー (QRコードの直接可読を防ぐための簡易XOR暗号化に使用)
    const std::string SECRET_KEY = "user0001";

    // 【追加】ヒントのプレースホルダーを保持する静的グローバル変数
    // static修飾子により、本ファイル内でのみ有効（内部結合）かつ永続的な生存期間を持つ
    static char g_HintPlaceholder[128] = "e.g. 'Gmail', 'AWS', 'Bank'...";

    std::string GenerateSessionID() {
        return "SESSION-001"; // 将来的にはランダム生成へ拡張可能
    }

    // 【追加】Getter関数の実装
    const char* GetHintPlaceholder() {
        return g_HintPlaceholder;
    }

    // 【追加】Setter関数の実装
    void SetHintPlaceholder(const char* newText) {
        // 安全なコピー：文字列長によるバッファオーバーフローを防止
        // g_HintPlaceholder: コピー先
        // sizeof(...): コピー先の最大容量
        // newText: コピー元
        strcpy_s(g_HintPlaceholder, sizeof(g_HintPlaceholder), newText);
    }

    // パスワード生成のコアロジック
    void GeneratePassword(const char* seedStr, const char* extraHint, int length, char* outBuffer, int outBufferSize) {
        // 1. マスターキーとヒント文字列の結合
        std::string rawSource = std::string(seedStr) + extraHint;

        // 2. ハッシュ値を計算しランダムシードとして使用 (同一入力から常に同一パスワードを生成するため)
        std::hash<std::string> hasher;
        size_t seedValue = hasher(rawSource);
        std::mt19937 gen((unsigned int)seedValue);

        // 3. 文字セットの定義 (大文字・小文字 + 数字 + 特殊記号)
        const std::string charset =
            "abcdefghijkmnpqrstuvwxyz"
            "ABCDEFGHJKLMNPQRSTUVWXYZ"
            "23456789!@#$%&*+?";

        std::uniform_int_distribution<> dist(0, (int)charset.size() - 1);

        // 4. バッファオーバーフロー対策（境界チェック）
        int actualLen = (length < outBufferSize - 1) ? length : outBufferSize - 1;

        // 5. パスワード生成処理
        for (int i = 0; i < actualLen; i++) {
            outBuffer[i] = charset[dist(gen)];
        }
        outBuffer[actualLen] = '\0'; // 終端文字（ヌル文字）の付加
    }

    // 暗号化ロジック (簡易XOR暗号化)
    std::string EncryptForQRCode(const char* rawPass) {
        std::string result = "COLDVAULT:";
        int keyLen = (int)SECRET_KEY.length();
        int passLen = (int)strlen(rawPass);
        char hexBuf[4];

        for (int i = 0; i < passLen; i++) {
            // 各文字に対してXOR演算を実行
            unsigned char encryptedChar = rawPass[i] ^ SECRET_KEY[i % keyLen];
            sprintf_s(hexBuf, "%02X", encryptedChar); // 16進数文字列へ変換
            result += hexBuf;
        }
        return result;
    }
}