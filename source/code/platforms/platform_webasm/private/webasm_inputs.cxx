/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT


#include "webasm_inputs.hxx"
#include <emscripten/html5.h>
#include <emscripten.h>

namespace ice::platform::webasm
{

    using ice::input::KeyboardKey;
    using ice::input::KeyboardMod;

    auto webasm_map_keycode(char const* code, char const* key, bool& out_text_event) noexcept -> ice::input::KeyboardKey
    {
        out_text_event = true;

        // Matching all 'Key{}' codes, note the {} value is always uppercase
        if (char const key = code[3]; code[0] == 'K' && (key >= 'A' && key <= 'Z'))
        {
            // We are mapping in ASCII order so we can just calculate the value.
            return KeyboardKey(i16(KeyboardKey::KeyA) + (key - 'A'));
        }

        // Check number keys (top side) Digit{}
        if (code[0] == 'D' && *key >= '0' && *key <= '9')
        {
            // We are mapping in ASCII order so we can just calculate the value.
            return KeyboardKey(i16(KeyboardKey::Key0) + (*key - '0'));
        }

        // Check number keys (numpad) Numpad{}
        if (code[0] == 'N' && *key >= '0' && *key <= '9')
        {
            // We are mapping in ASCII order so we can just calculate the value.
            return KeyboardKey(i16(KeyboardKey::NumPad0) + (*key - '0'));
        }

        // Check number keys (F1-F12) Numpad{}
        if (code[0] == 'F' && code[1] >= '1' && code[1] <= '9')
        {
            ice::u32 offset = code[1] - '0';
            if (code[2] != '\0')
            {
                offset *= 10;
                offset += code[2] - '0';
            }

            // We are mapping in ASCII order so we can just calculate the value.
            return KeyboardKey(i16(KeyboardKey::NumPad0) + offset);
        }

        // Check all ASCII key values first before continuing
        switch(*key)
        {
        case '&': return KeyboardKey::Ampersand;
        case '*': return KeyboardKey::Asteriks;
        case '@': return KeyboardKey::At;
        case '`': return KeyboardKey::BackQuote;
        case '^': return KeyboardKey::Caret;
        case ':': return KeyboardKey::Colon;
        case ';': return KeyboardKey::SemiColon;
        case '=': return KeyboardKey::Equals;
        case '!': return KeyboardKey::Exclaim;
        case '#': return KeyboardKey::Hash;
        case '%': return KeyboardKey::Percent;
        case '.': return KeyboardKey::Period;
        case ',': return KeyboardKey::Comma;
        case '\'': return KeyboardKey::Quote;
        case '"': return KeyboardKey::QuoteDouble;
        case '?': return KeyboardKey::Question;
        case '$': return KeyboardKey::Dollar;
        case ' ': return KeyboardKey::Space;
        case '_': return KeyboardKey::Underscore;
        case '>': return KeyboardKey::Greater;
        case '<': return KeyboardKey::Less;
        case '+': return KeyboardKey::Plus;
        case '-': return KeyboardKey::Minus;
        case '/': return KeyboardKey::Slash;
        case '\\': return KeyboardKey::BackSlash;
        case '(': return KeyboardKey::LeftParen;
        case ')': return KeyboardKey::RightParen;
        case '{': return KeyboardKey::LeftBracket;
        case '}': return KeyboardKey::RightBracket;
        // case '[': return KeyboardKey::LeftBracket;
        // case ']': return KeyboardKey::RightBracket;
        }

        out_text_event = false;

        // End,
        switch(code[0])
        {
        case 'A': // Arrows, AltL, AltR
            out_text_event = false;
            if (code[5] == 'L') return KeyboardKey::Left;
            if (code[5] == 'R') return KeyboardKey::Right;
            if (code[5] == 'U') return KeyboardKey::Up;
            if (code[5] == 'D') return KeyboardKey::Down;
            if (code[3] == 'L') return KeyboardKey::KeyLeftAlt;
            if (code[3] == 'R') return KeyboardKey::KeyRightAlt;
            break;
        case 'B': // Backspace
            if (code[5] == 'p') return KeyboardKey::Backspace;
            break;
        case 'C': // ControlL, ControlR
            if (code[7] == 'L') return KeyboardKey::KeyLeftCtrl;
            if (code[7] == 'R') return KeyboardKey::KeyRightCtrl;
            break;
        case 'E': // End + Enter + Escape
            if (code[2] == 'd') return KeyboardKey::End;
            if (code[2] == 't') return KeyboardKey::Return; // Enter
            if (code[2] == 'c') return KeyboardKey::Escape;
            break;
        case 'H': // Home
            if (code[2] == 'm') return KeyboardKey::Home;
        case 'I': // Insert
            if (code[5] == 't') return KeyboardKey::Insert;
            break;
        case 'P': // PageUp, PageDown
            if (code[4] == 'U') return KeyboardKey::PageUp;
            if (code[4] == 'D') return KeyboardKey::PageDown;
            break;
        case 'S': // ShiftL, ShiftR
            if (code[5] == 'L') return KeyboardKey::KeyLeftShift;
            if (code[5] == 'R') return KeyboardKey::KeyRightShift;
            break;
        case 'T': // Tab
            if (code[2] == 'b') return KeyboardKey::Tab;
            break;
        }

        out_text_event = false;
        return KeyboardKey::Unknown;
    }

}
