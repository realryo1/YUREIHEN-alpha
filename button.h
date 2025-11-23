#pragma once

#include "DirectXMath.h"
using namespace DirectX;

class Button
{
public:
    Button(float x, float y, float width, float height);
    ~Button();

    void Draw(void);
    void Update(void);

    // ボタン状態
    bool IsClicked(void) const;
    bool IsHovered(void) const;
    void SetPosition(float x, float y);
    void SetSize(float width, float height);
    void SetColor(XMFLOAT4 color);
    void SetHoverColor(XMFLOAT4 color);

private:
    XMFLOAT2 position;      // ボタンの中心座標
    float width;
    float height;
    XMFLOAT4 color;         // RGBA
    XMFLOAT4 hoverColor;    // ホバー時の色
    bool isHovered;
    bool wasClicked;

    bool CheckMouseCollision(void) const;
};
