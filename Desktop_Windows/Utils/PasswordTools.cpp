#include "PasswordTools.h"
#include <random>
#include <string.h>
#include <stdio.h>
#include <functional> // 用于 hash

namespace SecureLogic {
    // 内部密钥 (用于简单异或加密，防止二维码被肉眼直接识别)
    const std::string SECRET_KEY = "user0001";

    // 【新增】定义一个静态全局变量，用来存提示词
    // static 意味着它只在这个文件内部“活着”，但生命周期是永久的
    static char g_HintPlaceholder[128] = "e.g. 'Gmail', 'AWS', 'Bank'...";

    std::string GenerateSessionID() {
        return "SESSION-001"; // 这里可以扩展为随机生成
    }

    // 【新增】实现获取函数
    const char* GetHintPlaceholder() {
        return g_HintPlaceholder;
    }

    // 【新增】实现修改函数
    void SetHintPlaceholder(const char* newText) {
        // 安全复制：防止新文字太长把内存撑爆
        // g_HintPlaceholder: 目标
        // sizeof(...): 目标最大容量
        // newText: 来源
        strcpy_s(g_HintPlaceholder, sizeof(g_HintPlaceholder), newText);
    }

    // 核心密码生成逻辑
    void GeneratePassword(const char* seedStr, const char* extraHint, int length, char* outBuffer, int outBufferSize) {
        // 1. 拼接主密钥和提示词
        std::string rawSource = std::string(seedStr) + extraHint;

        // 2. 计算哈希值作为随机种子 (保证同样的输入永远生成同样的密码)
        std::hash<std::string> hasher;
        size_t seedValue = hasher(rawSource);
        std::mt19937 gen((unsigned int)seedValue);

        // 3. 定义字符集 (大小写 + 数字 + 特殊符号)
        const std::string charset =
            "abcdefghijkmnpqrstuvwxyz"
            "ABCDEFGHJKLMNPQRSTUVWXYZ"
            "23456789!@#$%&*+?";

        std::uniform_int_distribution<> dist(0, (int)charset.size() - 1);

        // 4. 确保不越界
        int actualLen = (length < outBufferSize - 1) ? length : outBufferSize - 1;

        // 5. 生成密码
        for (int i = 0; i < actualLen; i++) {
            outBuffer[i] = charset[dist(gen)];
        }
        outBuffer[actualLen] = '\0'; // 结尾补零
    }

    // 加密逻辑 (简单的异或加密)
    std::string EncryptForQRCode(const char* rawPass) {
        std::string result = "COLDVAULT:";
        int keyLen = (int)SECRET_KEY.length();
        int passLen = (int)strlen(rawPass);
        char hexBuf[4];

        for (int i = 0; i < passLen; i++) {
            // 对每一位字符进行异或运算
            unsigned char encryptedChar = rawPass[i] ^ SECRET_KEY[i % keyLen];
            sprintf_s(hexBuf, "%02X", encryptedChar); // 转成 16 进制字符串
            result += hexBuf;
        }
        return result;
    }
}