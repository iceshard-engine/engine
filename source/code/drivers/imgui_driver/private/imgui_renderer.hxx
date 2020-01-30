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
        iceshard::renderer::api::Texture _font_texture = iceshard::renderer::api::Texture::Invalid;
        iceshard::renderer::api::Buffer _indice_buffer = iceshard::renderer::api::Buffer::Invalid;
        iceshard::renderer::Pipeline _pipeline = iceshard::renderer::Pipeline::Invalid;
        iceshard::renderer::ResourceSet _resource_set = iceshard::renderer::ResourceSet::Invalid;
        core::pod::Array<iceshard::renderer::api::Buffer> _vertice_buffers;
    };

} // namespace debugui::imgui
