#pragma once
#include <render_system/render_system.hxx>
#include <imgui/imgui.h>

namespace debugui::imgui
{

    class DebugUIRenderer final
    {
    public:
        DebugUIRenderer(core::allocator& alloc, render::RenderSystem& render_system) noexcept;
        ~DebugUIRenderer() noexcept;

        void Draw(ImDrawData* draw_data) noexcept;

    private:
        core::allocator& _allocator;
        render::RenderSystem& _render_system;
    };

} // namespace debugui::imgui
