#pragma once
#include <ice/input/input_keyboard.hxx>
#include <ice/input/input_mouse.hxx>

namespace ice::platform::webasm
{

    using ice::input::KeyboardKey;
    using ice::input::KeyboardMod;

    auto webasm_map_keycode(char const* code, char const* key, bool& out_text_event) noexcept -> ice::input::KeyboardKey;

} // namespace ice::platform::webasm
