#include "imgui_inputs.hxx"
#include <core/message/operations.hxx>
#include <iceshard/input/input_mouse.hxx>
#include <iceshard/input/input_keyboard.hxx>

#include <input_system/message/keyboard.hxx>
#include <input_system/message/window.hxx>

namespace iceshard::debug::imgui
{

    ImGuiInputs::ImGuiInputs(ImGuiIO& io, ::input::InputSystem& input_system) noexcept
        : _io{ io }
        , _input_system{ input_system }
    {
        io.KeyMap[ImGuiKey_Tab] = (uint32_t)iceshard::input::KeyboardKey::Tab;
        io.KeyMap[ImGuiKey_LeftArrow] = (uint32_t)iceshard::input::KeyboardKey::Left;
        io.KeyMap[ImGuiKey_RightArrow] = (uint32_t)iceshard::input::KeyboardKey::Right;
        io.KeyMap[ImGuiKey_UpArrow] = (uint32_t)iceshard::input::KeyboardKey::Up;
        io.KeyMap[ImGuiKey_DownArrow] = (uint32_t)iceshard::input::KeyboardKey::Down;
        io.KeyMap[ImGuiKey_PageUp] = (uint32_t)iceshard::input::KeyboardKey::PageUp;
        io.KeyMap[ImGuiKey_PageDown] = (uint32_t)iceshard::input::KeyboardKey::PageDown;
        io.KeyMap[ImGuiKey_Home] = (uint32_t)iceshard::input::KeyboardKey::Home;
        io.KeyMap[ImGuiKey_End] = (uint32_t)iceshard::input::KeyboardKey::End;
        io.KeyMap[ImGuiKey_Insert] = (uint32_t)iceshard::input::KeyboardKey::Insert;
        io.KeyMap[ImGuiKey_Delete] = (uint32_t)iceshard::input::KeyboardKey::Delete;
        io.KeyMap[ImGuiKey_Backspace] = (uint32_t)iceshard::input::KeyboardKey::Backspace;
        io.KeyMap[ImGuiKey_Space] = (uint32_t)iceshard::input::KeyboardKey::Space;
        io.KeyMap[ImGuiKey_Enter] = (uint32_t)iceshard::input::KeyboardKey::Return;
        io.KeyMap[ImGuiKey_Escape] = (uint32_t)iceshard::input::KeyboardKey::Escape;
        io.KeyMap[ImGuiKey_KeyPadEnter] = 0;
        io.KeyMap[ImGuiKey_A] = (uint32_t)iceshard::input::KeyboardKey::KeyA;
        io.KeyMap[ImGuiKey_C] = (uint32_t)iceshard::input::KeyboardKey::KeyC;
        io.KeyMap[ImGuiKey_V] = (uint32_t)iceshard::input::KeyboardKey::KeyV;
        io.KeyMap[ImGuiKey_X] = (uint32_t)iceshard::input::KeyboardKey::KeyX;
        io.KeyMap[ImGuiKey_Y] = (uint32_t)iceshard::input::KeyboardKey::KeyY;
        io.KeyMap[ImGuiKey_Z] = (uint32_t)iceshard::input::KeyboardKey::KeyZ;
    }

    ImGuiInputs::~ImGuiInputs() noexcept
    {
    }

    void ImGuiInputs::update(iceshard::input::DeviceInputQueue const& inputs) noexcept
    {
        using namespace iceshard::input;

        inputs.for_each([this](DeviceInputMessage msg, void const* data) noexcept
            {
                if (is_device_type(msg.device, DeviceType::Keyboard))
                {
                    on_keyboard_event(msg, data);
                }
                else if (is_device_type(msg.device, DeviceType::Mouse))
                {
                    switch (msg.input_type)
                    {
                    case DeviceInputType::MousePosition:
                    {
                        auto pos = iceshard::input::read<int32_t>(msg, data);
                        _io.MousePos = { (float)pos[0], (float)pos[1] };
                        break;
                    }
                    case DeviceInputType::MouseButtonDown:
                    {
                        auto const button = iceshard::input::read_one<iceshard::input::MouseInput>(msg, data);
                        _io.MouseDown[0] = button == MouseInput::ButtonLeft;
                        _io.MouseDown[1] = button == MouseInput::ButtonRight;
                        _io.MouseDown[2] = button == MouseInput::ButtonMiddle;
                        break;
                    }
                    case DeviceInputType::MouseButtonUp:
                    {
                        auto const button = iceshard::input::read_one<iceshard::input::MouseInput>(msg, data);
                        if (_io.MouseDown[0])
                        {
                            _io.MouseDown[0] = button != iceshard::input::MouseInput::ButtonLeft;
                        }
                        if (_io.MouseDown[1])
                        {
                            _io.MouseDown[1] = button != iceshard::input::MouseInput::ButtonRight;
                        }
                        if (_io.MouseDown[2])
                        {
                            _io.MouseDown[2] = button != iceshard::input::MouseInput::ButtonMiddle;
                        }
                        break;
                    }
                    case DeviceInputType::MouseWheel:
                    {
                        auto pos = iceshard::input::read_one<int32_t>(msg, data);
                        if (pos > 0)
                        {
                            _io.MouseWheel += 1.0f;
                        }
                        if (pos < 0)
                        {
                            _io.MouseWheel -= 1.0f;
                        }
                        break;
                    }
                    default:
                        break;
                    }
                }

            });
    }

    void ImGuiInputs::update(core::MessageBuffer const& messages) noexcept
    {
        core::message::for_each(messages, std::bind(&ImGuiInputs::on_message, this, std::placeholders::_1));
    }

    void ImGuiInputs::on_keyboard_event(iceshard::input::DeviceInputMessage msg, void const* data) noexcept
    {
        using namespace iceshard::input;

        if (msg.input_type == DeviceInputType::KeyboardButtonDown)
        {
            auto const key = read_one<KeyboardKey>(msg, data);
            _io.KeysDown[static_cast<int32_t>(key)] = true;
        }
        else if (msg.input_type == DeviceInputType::KeyboardButtonUp)
        {
            auto const key = read_one<KeyboardKey>(msg, data);
            _io.KeysDown[static_cast<int32_t>(key)] = false;
        }
        else
        {
            bool const pressed = msg.input_type == DeviceInputType::KeyboardModifierDown;
            auto const mod = read_one<KeyboardMod>(msg, data);

            if (has_flag(mod, KeyboardMod::ShiftAny))
            {
                _io.KeyShift = pressed;
            }
            if (has_flag(mod, KeyboardMod::CtrlAny))
            {
                _io.KeyCtrl = pressed;
            }
            if (has_flag(mod, KeyboardMod::AltAny))
            {
                _io.KeyAlt = pressed;
            }
            if (has_flag(mod, KeyboardMod::GuiAny))
            {
                _io.KeySuper = pressed;
            }
        }
    }

    void ImGuiInputs::on_message(core::Message const& msg) noexcept
    {
        using ::input::message::TextInput;

        if (msg.header.type == TextInput::message_type)
        {
            auto const& data = *reinterpret_cast<TextInput const*>(msg.data._data);
            _io.AddInputCharactersUTF8(data.text);
        }
        else if (msg.header.type == ::input::message::WindowSizeChanged::message_type)
        {
            auto const& data = *reinterpret_cast<::input::message::WindowSizeChanged const*>(msg.data._data);
            _io.DisplaySize = { (float)data.width, (float)data.height };
        }
    }

} // namespace debugui::imgui
