#pragma once

namespace mooned::io
{

enum class MouseButton : int
{
    BT_LEFT,
    BT_RIGHT,
    BT_MIDDLE
};

bool check_button(MouseButton button);

void mouse_position(int& x, int& y);

void set_relative_mouse_mode(bool relative);

bool relative_mouse_mode();

void show_cursor(bool show);

}
