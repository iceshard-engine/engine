#include "imgui_inputs.hxx"
#include <core/message/operations.hxx>
#include <input_system/keyboard.hxx>
#include <input_system/message/keyboard.hxx>
#include <input_system/message/mouse.hxx>
#include <input_system/message/window.hxx>

namespace iceshard::debug::imgui
{

    ImGuiInputs::ImGuiInputs(ImGuiIO& io, input::InputSystem& input_system) noexcept
        : _io{ io }
        , _input_system{ input_system }
    {
        io.KeyMap[ImGuiKey_Tab] = (uint32_t)input::KeyboardKey::Tab;
        io.KeyMap[ImGuiKey_LeftArrow] = (uint32_t)input::KeyboardKey::Left;
        io.KeyMap[ImGuiKey_RightArrow] = (uint32_t)input::KeyboardKey::Right;
        io.KeyMap[ImGuiKey_UpArrow] = (uint32_t)input::KeyboardKey::Up;
        io.KeyMap[ImGuiKey_DownArrow] = (uint32_t)input::KeyboardKey::Down;
        io.KeyMap[ImGuiKey_PageUp] = (uint32_t)input::KeyboardKey::PageUp;
        io.KeyMap[ImGuiKey_PageDown] = (uint32_t)input::KeyboardKey::PageDown;
        io.KeyMap[ImGuiKey_Home] = (uint32_t)input::KeyboardKey::Home;
        io.KeyMap[ImGuiKey_End] = (uint32_t)input::KeyboardKey::End;
        io.KeyMap[ImGuiKey_Insert] = (uint32_t)input::KeyboardKey::Insert;
        io.KeyMap[ImGuiKey_Delete] = (uint32_t)input::KeyboardKey::Delete;
        io.KeyMap[ImGuiKey_Backspace] = (uint32_t)input::KeyboardKey::Backspace;
        io.KeyMap[ImGuiKey_Space] = (uint32_t)input::KeyboardKey::Space;
        io.KeyMap[ImGuiKey_Enter] = (uint32_t)input::KeyboardKey::Return;
        io.KeyMap[ImGuiKey_Escape] = (uint32_t)input::KeyboardKey::Escape;
        io.KeyMap[ImGuiKey_KeyPadEnter] = 0;
        io.KeyMap[ImGuiKey_A] = (uint32_t)input::KeyboardKey::KeyA;
        io.KeyMap[ImGuiKey_C] = (uint32_t)input::KeyboardKey::KeyC;
        io.KeyMap[ImGuiKey_V] = (uint32_t)input::KeyboardKey::KeyV;
        io.KeyMap[ImGuiKey_X] = (uint32_t)input::KeyboardKey::KeyX;
        io.KeyMap[ImGuiKey_Y] = (uint32_t)input::KeyboardKey::KeyY;
        io.KeyMap[ImGuiKey_Z] = (uint32_t)input::KeyboardKey::KeyZ;
    }

    ImGuiInputs::~ImGuiInputs() noexcept
    {
    }

    void ImGuiInputs::update(core::MessageBuffer const& messages) noexcept
    {
        core::message::for_each(messages, std::bind(&ImGuiInputs::on_message, this, std::placeholders::_1));
    }

    void ImGuiInputs::on_message(core::Message const& msg) noexcept
    {
        using input::message::MouseMotion;
        using input::message::MouseWheel;
        using input::message::MouseButtonUp;
        using input::message::MouseButtonDown;

        using input::KeyboardMod;
        using input::KeyboardKey;
        using input::message::KeyboardKeyUp;
        using input::message::KeyboardKeyDown;
        using input::message::KeyboardModChanged;
        using input::message::KeyboardTextInput;

        if (msg.header.type == MouseMotion::message_type)
        {
            auto const& data = *reinterpret_cast<MouseMotion const*>(msg.data._data);
            _io.MousePos = { (float)data.pos.x, (float)data.pos.y };
        }
        else if (msg.header.type == MouseButtonDown::message_type)
        {
            auto const& data = *reinterpret_cast<MouseButtonDown const*>(msg.data._data);
            _io.MousePos = { (float)data.pos.x, (float)data.pos.y };

            _io.MouseDown[0] = data.button == input::MouseButton::Left; // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
            _io.MouseDown[1] = data.button == input::MouseButton::Right;
            _io.MouseDown[2] = data.button == input::MouseButton::Middle;
        }
        else if (msg.header.type == MouseButtonUp::message_type)
        {
            auto const& data = *reinterpret_cast<MouseButtonUp const*>(msg.data._data);
            _io.MousePos = { (float)data.pos.x, (float)data.pos.y };

            if (_io.MouseDown[0])
            {
                _io.MouseDown[0] = data.button != input::MouseButton::Left; // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
            }
            if (_io.MouseDown[1])
            {
                _io.MouseDown[1] = data.button != input::MouseButton::Right; // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
            }
            if (_io.MouseDown[2])
            {
                _io.MouseDown[2] = data.button != input::MouseButton::Middle; // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
            }
        }
        else if (msg.header.type == MouseWheel::message_type)
        {
            auto const& data = *reinterpret_cast<MouseWheel const*>(msg.data._data);
            if (data.dy > 0)
            {
                _io.MouseWheel += 1.0f;
            }
            else if (data.dy < 0)
            {
                _io.MouseWheel -= 1.0f;
            }
            else if (data.dx > 0)
            {
                _io.MouseWheelH += 1.0f;
            }
            else if (data.dx < 0)
            {
                _io.MouseWheelH -= 1.0f;
            }
        }
        else if (msg.header.type == KeyboardKeyDown::message_type)
        {
            auto const& data = *reinterpret_cast<KeyboardKeyDown const*>(msg.data._data);
            _io.KeysDown[static_cast<uint32_t>(data.key)] = true;
        }
        else if (msg.header.type == KeyboardKeyUp::message_type)
        {
            auto const& data = *reinterpret_cast<KeyboardKeyUp const*>(msg.data._data);
            _io.KeysDown[static_cast<uint32_t>(data.key)] = false;
        }
        else if (msg.header.type == KeyboardModChanged::message_type)
        {
            auto const& data = *reinterpret_cast<KeyboardModChanged const*>(msg.data._data);
            if (has_flag(data.mod, KeyboardMod::ShiftAny))
            {
                _io.KeyShift = data.pressed;
            }
            if (has_flag(data.mod, KeyboardMod::CtrlAny))
            {
                _io.KeyCtrl = data.pressed;
            }
            if (has_flag(data.mod, KeyboardMod::AltAny))
            {
                _io.KeyAlt = data.pressed;
            }
            if (has_flag(data.mod, KeyboardMod::GuiAny))
            {
                _io.KeySuper = data.pressed;
            }
        }
        else if (msg.header.type == KeyboardTextInput::message_type)
        {
            auto const& data = *reinterpret_cast<KeyboardTextInput const*>(msg.data._data);
            _io.AddInputCharactersUTF8(data.text);
        }
        else if (msg.header.type == input::message::WindowSizeChanged::message_type)
        {
            auto const& data = *reinterpret_cast<input::message::WindowSizeChanged const*>(msg.data._data);
            _io.DisplaySize = { (float)data.width, (float)data.height };
        }
    }

} // namespace debugui::imgui
