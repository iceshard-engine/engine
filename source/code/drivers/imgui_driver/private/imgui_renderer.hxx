#pragma once
#include <asset_system/asset_system.hxx>
#include <render_system/render_system.hxx>
#include <imgui/imgui.h>

namespace debugui::imgui
{

    class ImGuiRenderer final
    {
    public:
        ImGuiRenderer(core::allocator& alloc, ImGuiIO& io, asset::AssetSystem& asset_system, render::RenderSystem& render_system) noexcept;
        ~ImGuiRenderer() noexcept;

        void Draw(ImDrawData* draw_data) noexcept;

    private:
        core::allocator& _allocator;
        asset::AssetSystem& _asset_system;
        render::RenderSystem& _render_system;
        ImGuiIO& _io;

        // Render resources used by ImGUI.
        render::api::Texture _font_texture = render::api::Texture::Invalid;
        render::api::RenderPipeline _pipeline = render::api::RenderPipeline::Invalid;
        render::api::VertexBuffer _indice_buffer = render::api::VertexBuffer::Invalid;
        core::pod::Array<render::api::VertexBuffer> _vertice_buffers;
    };

} // namespace debugui::imgui
