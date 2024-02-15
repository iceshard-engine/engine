/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_graph_runtime.hxx>
#include <ice/mem_allocator_proxy.hxx>
#include <ice/container_types.hxx>

#include "ice_gfx_graph_snapshot.hxx"

namespace ice::gfx
{

    struct GraphBarrier
    {
        ice::u32 pass_idx;
        ice::render::ImageLayout source_layout;
        ice::render::ImageLayout destination_layout;
        ice::render::AccessFlags source_access;
        ice::render::AccessFlags destination_access;
    };

    struct IceshardGfxGraphStages
    {
        IceshardGfxGraphStages(ice::Allocator& alloc) noexcept
            : _counts{ alloc }
            , _stages{ alloc }
        { }

        ice::Array<ice::u8> _counts;
        ice::Array<ice::StringID> _stages;
    };

    class IceshardGfxGraphRuntime final : public ice::gfx::GfxGraphRuntime
    {
    public:
        IceshardGfxGraphRuntime(
            ice::Allocator& alloc,
            ice::gfx::GfxDevice& device,
            ice::render::RenderSwapchain const& swapchain,
            ice::render::Renderpass renderpass,
            ice::Array<ice::gfx::GfxGraphSnapshot> snapshots,
            ice::Array<ice::gfx::GfxResource> resources,
            ice::gfx::IceshardGfxGraphStages stages
        ) noexcept;
        ~IceshardGfxGraphRuntime() noexcept override;

        auto renderpass() const noexcept -> ice::render::Renderpass override;

        bool execute(
            ice::EngineFrame const& frame,
            GfxStageRegistry const& stage_registry,
            ice::render::RenderFence& fence
        ) noexcept override;

    private:
        bool execute_pass(
            ice::EngineFrame const& frame,
            GfxStageRegistry const& stage_registry,
            ice::render::Framebuffer framebuffer,
            ice::render::RenderCommands& api,
            ice::render::CommandBuffer cmds
        ) noexcept;

    private:
        ice::ProxyAllocator _allocator;
        GfxDevice& _device;
        ice::render::RenderSwapchain const& _swapchain;

        ice::Array<ice::gfx::GfxGraphSnapshot> _snapshots;
        ice::Array<ice::gfx::GfxResource> _resources;
        ice::Array<ice::render::Image> _framebuffer_images;
        ice::Array<ice::render::Framebuffer> _framebuffers;
        ice::render::Renderpass _renderpass;
        ice::Array<vec4f> _clears;
        ice::gfx::IceshardGfxGraphStages _stages;
    };

} // namespace ice
