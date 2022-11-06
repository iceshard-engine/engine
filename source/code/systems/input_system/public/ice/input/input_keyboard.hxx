/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>

namespace ice::input
{

    enum class InputID : ice::u16;
    enum class DeviceType : ice::u8;

    enum class KeyboardKey : ice::i16;
    enum class KeyboardMod : ice::i16;


    inline constexpr bool has_flag(
        ice::input::KeyboardMod value,
        ice::input::KeyboardMod flag
    ) noexcept;


    constexpr static ice::u16 key_identifier_base_value = 0;
    constexpr static ice::u16 mod_identifier_base_value = 256;


    enum class KeyboardKey : ice::i16
    {
        Unknown,

        Return,
        Escape,
        Backspace,
        Tab,
        Space,
        Exclaim,
        QuoteDouble,
        Hash,
        Percent,
        Ampersand,
        Quote,
        LeftParen,
        RightParen,
        Asteriks,
        Plus,
        Comma,
        Minus,
        Period,
        Slash,

        Key0,
        Key1,
        Key2,
        Key3,
        Key4,
        Key5,
        Key6,
        Key7,
        Key8,
        Key9,

        Colon,
        SemiColon,
        Less,
        Equals,
        Greater,
        Question,
        At,

        LeftBracket,
        BackSlash,
        RightBracket,
        Caret,
        Underscore,
        BackQuote,

        KeyA,
        KeyB,
        KeyC,
        KeyD,
        KeyE,
        KeyF,
        KeyG,
        KeyH,
        KeyI,
        KeyJ,
        KeyK,
        KeyL,
        KeyM,
        KeyN,
        KeyO,
        KeyP,
        KeyQ,
        KeyR,
        KeyS,
        KeyT,
        KeyU,
        KeyV,
        KeyW,
        KeyX,
        KeyY,
        KeyZ,

        Delete,
        CapsLock,

        KeyF1,
        KeyF2,
        KeyF3,
        KeyF4,
        KeyF5,
        KeyF6,
        KeyF7,
        KeyF8,
        KeyF9,
        KeyF10,
        KeyF11,
        KeyF12,

        PrintScreen,
        ScrollLock,
        Pause,
        Insert,
        Home,
        PageUp,
        End,
        PageDown,
        Right,
        Left,
        Down,
        Up,

        KeyLeftCtrl,
        KeyLeftShift,
        KeyLeftAlt,
        KeyLeftGui,
        KeyRightCtrl,
        KeyRightShift,
        KeyRightAlt,
        KeyRightGui,

        NumPadNumlockClear,
        NumPadDivide,
        NumPadMultiply,
        NumPadMinus,
        NumPadPlus,
        NumPadEnter,

        NumPad1,
        NumPad2,
        NumPad3,
        NumPad4,
        NumPad5,
        NumPad6,
        NumPad7,
        NumPad8,
        NumPad9,
        NumPad0,

        Reserved,
    };

    enum class KeyboardMod : ice::i16
    {
        None = 0x0000,

        ShiftLeft = 0x0001,
        ShiftRight = 0x0002,
        ShiftAny = ShiftLeft | ShiftRight,

        CtrlLeft = 0x0004,
        CtrlRight = 0x0008,
        CtrlAny = CtrlLeft | CtrlRight,

        AltLeft = 0x0010,
        AltRight = 0x0020,
        AltAny = AltLeft | AltRight,

        GuiLeft = 0x0040,
        GuiRight = 0x0080,
        GuiAny = GuiLeft | GuiRight,

        NumLock = 0x0100,
        CapsLock = 0x0200,
        Mode = 0x0400,

        Reserved = 0x0800,
    };


    inline constexpr bool has_flag(
        ice::input::KeyboardMod value,
        ice::input::KeyboardMod flag
    ) noexcept
    {
        return (value & flag) != KeyboardMod::None;
    }

} // ice::input
