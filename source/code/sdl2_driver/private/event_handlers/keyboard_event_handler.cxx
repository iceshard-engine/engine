#include "../event_handlers.hxx"
#include <core/message/operations.hxx>
#include <input_system/message/keyboard.hxx>

namespace input::sdl2
{

    namespace detail
    {

        auto key_from_scancode(SDL_Scancode scancode) noexcept -> KeyboardKey
        {
            if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z)
            {
                return KeyboardKey{ static_cast<uint32_t>(KeyboardKey::KeyA) + (scancode - SDL_SCANCODE_A) };
            }

            if (scancode >= SDL_SCANCODE_F1 && scancode <= SDL_SCANCODE_F12)
            {
                return KeyboardKey{ static_cast<uint32_t>(KeyboardKey::KeyF1) + (scancode - SDL_SCANCODE_F1) };
            }

            if (scancode == SDL_SCANCODE_0)
            {
                return KeyboardKey{ static_cast<uint32_t>(KeyboardKey::Key0) };
            }
            else if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9)
            {
                return KeyboardKey{ static_cast<uint32_t>(KeyboardKey::Key1) + (scancode - SDL_SCANCODE_1) };
            }

            switch (scancode)
            {
                // Special codes (I)
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
                // Special codes (II)
            case SDL_SCANCODE_SEMICOLON: return KeyboardKey::SemiColon;
            case SDL_SCANCODE_EQUALS: return KeyboardKey::Equals;
            case SDL_SCANCODE_LEFTBRACKET: return KeyboardKey::LeftBracket;
            case SDL_SCANCODE_BACKSLASH: return KeyboardKey::BackSlash;
            case SDL_SCANCODE_RIGHTBRACKET: return KeyboardKey::RightBracket;
            case SDL_SCANCODE_GRAVE: return KeyboardKey::BackQuote;
                // Special codes (III)
            case SDL_SCANCODE_DELETE: return KeyboardKey::Delete;
            case SDL_SCANCODE_CAPSLOCK: return KeyboardKey::CapsLock;
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

        auto mod_from_scancode(SDL_Scancode scancode) noexcept -> KeyboardMod
        {
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

        auto mod_event_handler(core::MessageBuffer& messages, SDL_Event const& sdl_event) noexcept
        {
            if (auto const mod = mod_from_scancode(sdl_event.key.keysym.scancode); mod != KeyboardMod::None)
            {
                core::message::push(messages,
                    input::message::KeyboardModChanged{
                        .mod = mod,
                        .pressed = sdl_event.key.type == SDL_KEYDOWN
                    }
                );
            }
        }

        void key_event_handler(core::MessageBuffer& buffer, SDL_Event const& sdl_event) noexcept
        {
            if (auto const key = detail::key_from_scancode(sdl_event.key.keysym.scancode); key != KeyboardKey::Unknown)
            {
                if (sdl_event.key.type == SDL_KEYDOWN)
                {
                    core::message::push(buffer,
                        input::message::KeyboardKeyDown{
                            .key = key
                        }
                    );
                }
                else if (sdl_event.key.type == SDL_KEYUP)
                {
                    core::message::push(buffer,
                        input::message::KeyboardKeyUp{
                            .key = key
                        }
                    );
                }
            }
            else
            {
                mod_event_handler(buffer, sdl_event);
            }
        }

        void text_input_event_handler(core::MessageBuffer& buffer, SDL_Event const& sdl_event) noexcept
        {
            input::message::KeyboardTextInput message{ };
            std::memcpy(message.text, sdl_event.text.text, 32);
            core::message::push(buffer, std::move(message));
        }

    } // namespace detail

    //! \brief Handler functions for keyboard events.
    void register_keyboard_event_handlers(core::pod::Hash<EventHandlerSignature*>& handler_map) noexcept
    {
        core::pod::hash::set(handler_map, SDL_EventType::SDL_KEYDOWN, &detail::key_event_handler);
        core::pod::hash::set(handler_map, SDL_EventType::SDL_KEYUP, &detail::key_event_handler);
        core::pod::hash::set(handler_map, SDL_EventType::SDL_TEXTINPUT, &detail::text_input_event_handler);
    }

} // namespace input::sdl2
