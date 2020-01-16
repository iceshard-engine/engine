#include "../event_handlers.hxx"
#include <core/message/operations.hxx>
#include <input_system/message/keyboard.hxx>

namespace input::sdl2
{

    namespace detail
    {

        auto key_from_keycode(SDL_Keycode keycode) noexcept -> KeyboardKey
        {
            if (keycode >= SDLK_a && keycode <= SDLK_z)
            {
                return KeyboardKey{ static_cast<uint32_t>(KeyboardKey::KeyA) + (keycode - SDLK_a) };
            }

            if (keycode >= SDLK_F1 && keycode <= SDLK_F12)
            {
                return KeyboardKey{ static_cast<uint32_t>(KeyboardKey::KeyF1) + (keycode - SDLK_F1) };
            }

            if (keycode >= SDLK_0 && keycode <= SDLK_9)
            {
                return KeyboardKey{ static_cast<uint32_t>(KeyboardKey::Key0) + (keycode - SDLK_0) };
            }

            switch (keycode)
            {
                // Special codes (I)
            case SDLK_RETURN: return KeyboardKey::Return;
            case SDLK_ESCAPE: return KeyboardKey::Escape;
            case SDLK_BACKSPACE: return KeyboardKey::Backspace;
            case SDLK_TAB: return KeyboardKey::Tab;
            case SDLK_SPACE: return KeyboardKey::Space;
            case SDLK_EXCLAIM: return KeyboardKey::Exclaim;
            case SDLK_QUOTEDBL: return KeyboardKey::QuoteDouble;
            case SDLK_HASH: return KeyboardKey::Hash;
            case SDLK_PERCENT: return KeyboardKey::Percent;
            case SDLK_AMPERSAND: return KeyboardKey::Ampersand;
            case SDLK_QUOTE: return KeyboardKey::Quote;
            case SDLK_LEFTPAREN: return KeyboardKey::LeftParen;
            case SDLK_RIGHTPAREN: return KeyboardKey::RightParen;
            case SDLK_ASTERISK: return KeyboardKey::Asteriks;
            case SDLK_PLUS: return KeyboardKey::Plus;
            case SDLK_COMMA: return KeyboardKey::Comma;
            case SDLK_MINUS: return KeyboardKey::Minus;
            case SDLK_PERIOD: return KeyboardKey::Period;
            case SDLK_SLASH: return KeyboardKey::Slash;
                // Special codes (II)
            case SDLK_COLON: return KeyboardKey::Colon;
            case SDLK_SEMICOLON: return KeyboardKey::SemiColon;
            case SDLK_LESS: return KeyboardKey::Less;
            case SDLK_EQUALS: return KeyboardKey::Equals;
            case SDLK_GREATER: return KeyboardKey::Greater;
            case SDLK_QUESTION: return KeyboardKey::Question;
            case SDLK_AT: return KeyboardKey::At;
            case SDLK_LEFTBRACKET: return KeyboardKey::LeftBracket;
            case SDLK_BACKSLASH: return KeyboardKey::BackSlash;
            case SDLK_RIGHTBRACKET: return KeyboardKey::RightBracket;
            case SDLK_CARET: return KeyboardKey::Caret;
            case SDLK_UNDERSCORE: return KeyboardKey::Underscore;
            case SDLK_BACKQUOTE: return KeyboardKey::BackQuote;
                // Special codes (III)
            case SDLK_DELETE: return KeyboardKey::Delete;
            case SDLK_CAPSLOCK: return KeyboardKey::CapsLock;
            default:
                break;
            }

            return KeyboardKey::Unknown;
        }

        void key_event_handler(core::MessageBuffer& buffer, SDL_Event const& sdl_event) noexcept
        {
            if (sdl_event.key.type == SDL_KEYDOWN)
            {
                core::message::push(buffer,
                    input::message::KeyboardKeyDown{
                        .key = detail::key_from_keycode(sdl_event.key.keysym.scancode)
                    }
                );
            }
            else if (sdl_event.key.type == SDL_KEYUP)
            {
                core::message::push(buffer,
                    input::message::KeyboardKeyUp{
                        .key = detail::key_from_keycode(sdl_event.key.keysym.sym)
                    }
                );
            }
        }

    } // namespace detail

    //! \brief Handler functions for keyboard events.
    void register_keyboard_event_handlers(core::pod::Hash<EventHandlerSignature*>& handler_map) noexcept
    {
        core::pod::hash::set(handler_map, SDL_EventType::SDL_KEYDOWN, &detail::key_event_handler);
        core::pod::hash::set(handler_map, SDL_EventType::SDL_KEYUP, &detail::key_event_handler);
    }

} // namespace input::sdl2
