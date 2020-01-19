#include "imgui_renderer.hxx"

namespace debugui::imgui
{

    DebugUIRenderer::DebugUIRenderer(core::allocator& alloc, render::RenderSystem& render_system) noexcept
        : _allocator{ alloc }
        , _render_system{ render_system }
    {
    }

    DebugUIRenderer::~DebugUIRenderer() noexcept
    {
    }

    void DebugUIRenderer::Draw(ImDrawData* draw_data) noexcept
    {
    }

} // namespace debugui::imgui
