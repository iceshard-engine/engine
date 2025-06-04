/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "linux_sdl2_utils.hxx"
#include <ice/input/input_mouse.hxx>
#include <ice/input/input_keyboard.hxx>

namespace ice::platform::linux::sdl2
{

    auto map_sdl_mouse_button(ice::u8 button_id) noexcept -> ice::input::MouseInput
    {
        using ice::input::MouseInput;

        switch (button_id)
        {
        case SDL_BUTTON_LEFT:
            return MouseInput::ButtonLeft;
        case SDL_BUTTON_RIGHT:
            return MouseInput::ButtonRight;
        case SDL_BUTTON_MIDDLE:
            return MouseInput::ButtonMiddle;
        case SDL_BUTTON_X1:
            return MouseInput::ButtonCustom0;
        case SDL_BUTTON_X2:
            return MouseInput::ButtonCustom1;
        default:
            break;
        }
        return MouseInput::Unknown;
    }

    auto map_sdl_key_scancode(SDL_Scancode scancode) noexcept -> ice::input::KeyboardKey
    {
        using ice::input::KeyboardKey;

        if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z)
        {
            return KeyboardKey(static_cast<ice::u32>(KeyboardKey::KeyA) + (scancode - SDL_SCANCODE_A));
        }

        if (scancode >= SDL_SCANCODE_F1 && scancode <= SDL_SCANCODE_F12)
        {
            return KeyboardKey(static_cast<ice::u32>(KeyboardKey::KeyF1) + (scancode - SDL_SCANCODE_F1));
        }

        if (scancode == SDL_SCANCODE_0)
        {
            return KeyboardKey(static_cast<ice::u32>(KeyboardKey::Key0));
        }
        else if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9)
        {
            return KeyboardKey(static_cast<ice::u32>(KeyboardKey::Key1) + (scancode - SDL_SCANCODE_1));
        }

        switch (scancode)
        {
        case SDL_SCANCODE_RETURN: return KeyboardKey::Return;
        case SDL_SCANCODE_ESCAPE: return KeyboardKey::Escape;
        case SDL_SCANCODE_BACKSPACE: return KeyboardKey::Backspace;
        case SDL_SCANCODE_TAB: return KeyboardKey::Tab;
        case SDL_SCANCODE_SPACE: return KeyboardKey::Space;
        case SDL_SCANCODE_APOSTROPHE: return KeyboardKey::Quote;
        case SDL_SCANCODE_COMMA: return KeyboardKey::Comma;
        case SDL_SCANCODE_MINUS: return KeyboardKey::Minus;
        case SDL_SCANCODE_PERIOD: return KeyboardKey::Period;
        case SDL_SCANCODE_SLASH: return KeyboardKey::Slash;

        case SDL_SCANCODE_SEMICOLON: return KeyboardKey::SemiColon;
        case SDL_SCANCODE_EQUALS: return KeyboardKey::Equals;
        case SDL_SCANCODE_LEFTBRACKET: return KeyboardKey::LeftBracket;
        case SDL_SCANCODE_BACKSLASH: return KeyboardKey::BackSlash;
        case SDL_SCANCODE_RIGHTBRACKET: return KeyboardKey::RightBracket;
        case SDL_SCANCODE_GRAVE: return KeyboardKey::BackQuote;

        case SDL_SCANCODE_DELETE: return KeyboardKey::Delete;
        //case SDL_SCANCODE_CAPSLOCK: return KeyboardKey::CapsLock;

        case SDL_SCANCODE_UP: return KeyboardKey::Up;
        case SDL_SCANCODE_DOWN: return KeyboardKey::Down;
        case SDL_SCANCODE_LEFT: return KeyboardKey::Left;
        case SDL_SCANCODE_RIGHT: return KeyboardKey::Right;
        default:
            break;
        }

        //case SDL_SCANCODE_EXCLAIM: return KeyboardKey::Exclaim;
        //case SDL_SCANCODE_QUOTEDBL: return KeyboardKey::QuoteDouble;
        //case SDL_SCANCODE_HASH: return KeyboardKey::Hash;
        //case SDL_SCANCODE_PERCENT: return KeyboardKey::Percent;
        //case SDL_SCANCODE_AMPERSAND: return KeyboardKey::Ampersand;
        //case SDL_SCANCODE_LEFTPAREN: return KeyboardKey::LeftParen;
        //case SDL_SCANCODE_RIGHTPAREN: return KeyboardKey::RightParen;
        //case SDL_SCANCODE_ASTERISK: return KeyboardKey::Asteriks;
        //case SDL_SCANCODE_PLUS: return KeyboardKey::Plus;
        //case SDL_SCANCODE_COLON: return KeyboardKey::Colon;
        //case SDL_SCANCODE_LESS: return KeyboardKey::Less;
        //case SDL_SCANCODE_GREATER: return KeyboardKey::Greater;
        //case SDL_SCANCODE_QUESTION: return KeyboardKey::Question;
        //case SDL_SCANCODE_AT: return KeyboardKey::At;
        //case SDL_SCANCODE_CARET: return KeyboardKey::Caret;
        //case SDL_SCANCODE_UNDERSCORE: return KeyboardKey::Underscore;

        return KeyboardKey::Unknown;
    }

    auto map_sdl_mod_scancode(SDL_Scancode scancode) noexcept -> ice::input::KeyboardMod
    {
        using ice::input::KeyboardMod;

        if (scancode == SDL_Scancode::SDL_SCANCODE_LCTRL)
        {
            return KeyboardMod::CtrlLeft;
        }
        if (scancode == SDL_Scancode::SDL_SCANCODE_LSHIFT)
        {
            return KeyboardMod::ShiftLeft;
        }
        if (scancode == SDL_Scancode::SDL_SCANCODE_LALT)
        {
            return KeyboardMod::AltLeft;
        }
        if (scancode == SDL_Scancode::SDL_SCANCODE_LGUI)
        {
            return KeyboardMod::GuiLeft;
        }

        if (scancode == SDL_Scancode::SDL_SCANCODE_RCTRL)
        {
            return KeyboardMod::CtrlRight;
        }
        if (scancode == SDL_Scancode::SDL_SCANCODE_RSHIFT)
        {
            return KeyboardMod::ShiftRight;
        }
        if (scancode == SDL_Scancode::SDL_SCANCODE_RALT)
        {
            return KeyboardMod::AltRight;
        }
        if (scancode == SDL_Scancode::SDL_SCANCODE_RGUI)
        {
            return KeyboardMod::GuiRight;
        }

        if (scancode == SDL_Scancode::SDL_SCANCODE_MODE)
        {
            return KeyboardMod::Mode;
        }
        if (scancode == SDL_Scancode::SDL_SCANCODE_NUMLOCKCLEAR)
        {
            return KeyboardMod::NumLock;
        }
        if (scancode == SDL_Scancode::SDL_SCANCODE_CAPSLOCK)
        {
            return KeyboardMod::CapsLock;
        }

        return KeyboardMod::None;
    }


    void mouse_input_events(
        ice::input::DeviceEventQueue& input_queue,
        SDL_Event const& sdl_event
    ) noexcept
    {
        using namespace ice::input;

        if (sdl_event.type == SDL_EventType::SDL_MOUSEMOTION)
        {
            ice::i32 pos[2];

            if (SDL_GetRelativeMouseMode())
            {
                pos[0] = sdl_event.motion.xrel;
                pos[1] = sdl_event.motion.yrel;
            }
            else
            {
                pos[0] = sdl_event.motion.x;
                pos[1] = sdl_event.motion.y;
            }

            if (pos[0] != 0 || pos[1] != 0)
            {
                input_queue.push(
                    make_device_handle(DeviceType::Mouse, DeviceIndex(sdl_event.motion.which)),
                    DeviceMessage::MousePositionX,
                    pos[0]
                );
                input_queue.push(
                    make_device_handle(DeviceType::Mouse, DeviceIndex(sdl_event.motion.which)),
                    DeviceMessage::MousePositionY,
                    pos[1]
                );
            }
        }
        else if (sdl_event.type == SDL_MOUSEBUTTONDOWN)
        {
            input_queue.push(
                make_device_handle(DeviceType::Mouse, DeviceIndex(sdl_event.motion.which)),
                DeviceMessage::MouseButtonDown,
                map_sdl_mouse_button(sdl_event.button.button)
            );
        }
        else if (sdl_event.type == SDL_MOUSEBUTTONUP)
        {
            input_queue.push(
                make_device_handle(DeviceType::Mouse, DeviceIndex(sdl_event.motion.which)),
                DeviceMessage::MouseButtonUp,
                map_sdl_mouse_button(sdl_event.button.button)
            );
        }
        else if (sdl_event.type == SDL_MOUSEWHEEL)
        {
            input_queue.push(
                make_device_handle(DeviceType::Mouse, DeviceIndex(sdl_event.motion.which)),
                DeviceMessage::MouseWheel,
                sdl_event.wheel.y
            );
        }
    }

    void keyboard_input_events(
        ice::input::DeviceEventQueue& input_queue,
        SDL_Event const& sdl_event
    ) noexcept
    {
        using namespace ice::input;
        [[maybe_unused]]
        DeviceHandle const device = make_device_handle(DeviceType::Keyboard, DeviceIndex{ 0 });

        KeyboardKey const key = map_sdl_key_scancode(sdl_event.key.keysym.scancode);
        if (key != KeyboardKey::Unknown)
        {
            if (sdl_event.key.type == SDL_KEYDOWN)
            {
                input_queue.push(
                    device,
                    DeviceMessage::KeyboardButtonDown,
                    key
                );
            }
            else if (sdl_event.key.type == SDL_KEYUP)
            {
                input_queue.push(
                    device,
                    DeviceMessage::KeyboardButtonUp,
                    key
                );
            }
        }
        else
        {
            KeyboardMod const mod = map_sdl_mod_scancode(sdl_event.key.keysym.scancode);
            if (mod != KeyboardMod::None)
            {
                if (sdl_event.key.type == SDL_KEYDOWN)
                {
                    input_queue.push(
                        device,
                        DeviceMessage::KeyboardModifierDown,
                        mod
                    );
                }
                else if (sdl_event.key.type == SDL_KEYUP)
                {
                    input_queue.push(
                        device,
                        DeviceMessage::KeyboardModifierUp,
                        mod
                    );
                }
            }
        }
    }

} // namespace ice::platform::linux::sdl2
