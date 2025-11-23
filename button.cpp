#include "button.h"
#include "sprite.h"
#include "mouse.h"

Button::Button(float x, float y, float width, float height)
    : position(XMFLOAT2(x, y)), width(width), height(height),
      color(XMFLOAT4(0.2f, 0.5f, 0.8f, 1.0f)),
      hoverColor(XMFLOAT4(0.3f, 0.6f, 1.0f, 1.0f)),
      isHovered(false), wasClicked(false)
{
}

Button::~Button()
{
}

void Button::Update(void)
{
    isHovered = CheckMouseCollision();
    
    // クリック判定
    Mouse_State mouseState;
    Mouse_GetState(&mouseState);
    wasClicked = (isHovered && mouseState.leftButton);
}

void Button::Draw(void)
{
    // DrawSprite関数を使用して描画
    XMFLOAT4 currentColor = isHovered ? hoverColor : color;
    DrawSprite(position, XMFLOAT2(width, height), currentColor);
}

bool Button::CheckMouseCollision(void) const
{
    Mouse_State mouseState;
    Mouse_GetState(&mouseState);

    // スクリーン座標でのマウス位置
    float mouseX = (float)mouseState.x;
    float mouseY = (float)mouseState.y;

    // ボタンの範囲チェック
    return (mouseX >= position.x - width / 2 && mouseX <= position.x + width / 2 &&
            mouseY >= position.y - height / 2 && mouseY <= position.y + height / 2);
}

bool Button::IsClicked(void) const
{
    return wasClicked;
}

bool Button::IsHovered(void) const
{
    return isHovered;
}

void Button::SetPosition(float x, float y)
{
    position = XMFLOAT2(x, y);
}

void Button::SetSize(float w, float h)
{
    width = w;
    height = h;
}

void Button::SetColor(XMFLOAT4 col)
{
    color = col;
}

void Button::SetHoverColor(XMFLOAT4 col)
{
    hoverColor = col;
}
