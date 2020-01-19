#include "imgui_inputs.hxx"
#include <input_system/keyboard.hxx>

namespace debugui::imgui
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

} // namespace debugui::imgui
