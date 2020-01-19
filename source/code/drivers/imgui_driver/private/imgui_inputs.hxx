#pragma once
#include <input_system/system.hxx>
#include <imgui/imgui.h>

namespace debugui::imgui
{

    class ImGuiInputs final
    {
    public:
        ImGuiInputs(ImGuiIO& io, input::InputSystem& input_system) noexcept;
        ~ImGuiInputs() noexcept;

    private:
        ImGuiIO& _io;
        input::InputSystem& _input_system;
    };

} // namespace debugui::imgui
