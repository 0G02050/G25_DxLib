#pragma once

void GeneratePassword(const char* seedStr, const char* extraHint, int length, char* outBuffer, int outBufferSize);
void DrawFakeQRCode(int x, int y, int size, const char* data);