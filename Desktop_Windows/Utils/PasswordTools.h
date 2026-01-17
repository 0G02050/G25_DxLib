#pragma once
#include <string>
#include <vector>

namespace SecureLogic {
    // 1. 生成会话 ID (示例功能)
    std::string GenerateSessionID();
    // 获取提示词 (读)
    const char* GetHintPlaceholder();
    // 【新增】修改提示词 (写)
    void SetHintPlaceholder(const char* newText);

    // 2. 生成密码 (核心算法)
    // seedStr: 主密钥
    // extraHint: 提示词组合
    // length: 密码长度
    // outBuffer: 接收结果的数组
    // outBufferSize: 数组最大长度
    void GeneratePassword(const char* seedStr, const char* extraHint, int length, char* outBuffer, int outBufferSize);

    // 3. 加密密码 (用于生成二维码)
    std::string EncryptForQRCode(const char* rawPass);
}