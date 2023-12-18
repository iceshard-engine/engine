/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container/hashmap.hxx>
#include <ice/gfx/gfx_render_graph_runtime.hxx>
#include <ice/gfx/gfx_object_storage.hxx>
#include <ice/gfx/gfx_device.hxx>

namespace ice::gfx::v2
{

    class SimpleGfxObjectStorage;
    class SimpleGfxRenderGraphRuntime final : public ice::gfx::v2::GfxRenderGraphRuntime
    {
    public:
        SimpleGfxRenderGraphRuntime(
            ice::Allocator& alloc,
            ice::gfx::v2::GfxRenderGraph const& render_graph,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept;

        ~SimpleGfxRenderGraphRuntime() noexcept override;

        auto storage() noexcept -> ice::gfx::v2::GfxObjectStorage& override;

        auto find_stage(
            ice::StringID_Arg identifier,
            ice::gfx::v2::GfxStage* stage
        ) noexcept -> ice::HashMap<ice::gfx::v2::GfxStage*>::ConstIterator;

        void bind_stage(
            ice::StringID_Arg identifier,
            ice::gfx::v2::GfxStage* stage
        ) noexcept override;

        void import_buffer(ice::StringID_Arg name, ice::render::Buffer buffer) noexcept override;
        void import_image(ice::StringID_Arg name, ice::render::Image buffer) noexcept override;

        void setup() noexcept override;
        bool execute(
            ice::gfx::v2::GfxRenderParams const& params,
            ice::render::RenderFence& fence
        ) noexcept override;

    private:
        ice::Allocator& _allocator;
        ice::gfx::v2::GfxRenderGraph const& _render_graph;
        ice::gfx::GfxDevice& _gfx_device;

        ice::UniquePtr<ice::gfx::v2::SimpleGfxObjectStorage> _storage;
        ice::HashMap<ice::gfx::v2::GfxStage*> _stages;

        ice::u8 _next_framebuffer_index;
        ice::render::RenderFence* _swap_fence;
    };

} // namespace ice::gfx::v2
