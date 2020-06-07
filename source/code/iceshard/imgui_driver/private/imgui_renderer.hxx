#pragma once
#include <asset_system/asset_system.hxx>
#include <iceshard/service_provider.hxx>
#include <iceshard/component/component_system.hxx>
#include <iceshard/render/render_stage.hxx>
#include <imgui/imgui.h>

namespace iceshard::debug::imgui
{

    class ImGuiRenderer final : public RenderStageTaskFactory
    {
    public:
        ImGuiRenderer(
            core::allocator& alloc,
            ImGuiIO& io,
            asset::AssetSystem& asset_system,
            iceshard::Engine& engine
        ) noexcept;

        ~ImGuiRenderer() noexcept;

        void create_render_tasks(
            iceshard::Frame const& current,
            iceshard::renderer::api::CommandBuffer cmds,
            core::Vector<cppcoro::task<>>& task_list
        ) noexcept override;

        auto draw_task(
            iceshard::renderer::api::CommandBuffer cb,
            ImDrawData* draw_data
        ) noexcept -> cppcoro::task<>;

    private:
        core::allocator& _allocator;
        asset::AssetSystem& _asset_system;
        iceshard::renderer::RenderSystem& _render_system;
        ImGuiIO& _io;

        iceshard::renderer::api::Texture _font_texture = iceshard::renderer::api::Texture::Invalid;
        iceshard::renderer::api::Buffer _temp_texture_data = iceshard::renderer::api::Buffer::Invalid;
        iceshard::renderer::api::CommandBuffer _temp_buffer = iceshard::renderer::api::CommandBuffer::Invalid;

        // Render resources used by ImGUI.
        iceshard::renderer::api::Buffer _indice_buffer = iceshard::renderer::api::Buffer::Invalid;
        iceshard::renderer::Pipeline _pipeline = iceshard::renderer::Pipeline::Invalid;
        iceshard::renderer::ResourceSet _resource_set = iceshard::renderer::ResourceSet::Invalid;
        core::pod::Array<iceshard::renderer::api::Buffer> _vertice_buffers;
    };

} // namespace debugui::imgui
