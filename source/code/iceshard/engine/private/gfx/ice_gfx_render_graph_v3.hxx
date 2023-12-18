/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/ice_gfx_render_graph_v3.hxx>
#include <ice/gfx/gfx_device.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/mem_allocator_proxy.hxx>

namespace ice::gfx::v3
{

    struct Snapshot
    {
        enum Event : ice::u8
        {
            EventInvalid = 0b0,
            EventReadRes = 0b0000'0001,
            EventWriteRes = 0b0000'0010,
            EventCreateRes = 0b0000'0100,
            EventDeleteRes = 0b0000'1000,

            EventBeginPass = 0b0011'0000,
            EventNextSubPass = 0b0001'0000,
            EventEndPass = 0b0100'0000,

            MaskResRW = EventWriteRes | EventReadRes,
            MaskRes = EventWriteRes | EventReadRes | EventCreateRes | EventDeleteRes,
            MaskPass = EventBeginPass | EventNextSubPass | EventEndPass
        };

        ice::u32 subpass;
        GfxResource resource;
        Event event; // create, write, read, transfer
        ice::u32 info;

        static bool compare(Snapshot const& left, Snapshot const& right) noexcept
        {
            return (left.subpass < right.subpass)
                && ((left.event & MaskResRW) < (right.event & MaskResRW)
                    || (left.resource.value < right.resource.value));
        }
    };

    class IceshardGfxGraph : public GfxGraph
    {
    public:
        IceshardGfxGraph(ice::Allocator& alloc) noexcept;
        ~IceshardGfxGraph() noexcept override = default;

        auto get_framebuffer() const noexcept -> GfxResource override;
        auto get_resource(ice::StringID_Arg name, GfxResourceType type) noexcept -> GfxResource override;
        bool add_pass(GfxGraphPass const& ) noexcept override;

        auto passes() const noexcept -> ice::Span<GfxGraphPass const> override;

    private:
        ice::HashMap<GfxGraphPass> _passes;
        ice::HashMap<GfxResource> _resources;
        ice::u32 _resources_ids[ice::u8(ice::gfx::v3::GfxResourceType::DepthStencil) + 1];
    };

    enum class GraphStageType : ice::u8
    {
        Allocate,
        Release,
        Barrier,
        Draw,
    };

    struct GraphBarrier
    {
        ice::u32 pass_idx;
        ice::render::ImageLayout source_layout;
        ice::render::ImageLayout destination_layout;
        ice::render::AccessFlags source_access;
        ice::render::AccessFlags destination_access;
    };

    struct GraphStage
    {
        GraphStageType type;
        union
        {
            ice::u32 barrier_idx;
            ice::StringID stage_name;
            GfxResource resource;
        };
    };

    struct IceshardGfxStages
    {
        IceshardGfxStages(ice::Allocator& alloc) noexcept
            : _counts{ alloc }
            , _stages{ alloc }
        { }

        ice::Array<ice::u8> _counts;
        ice::Array<ice::StringID> _stages;
    };

    class IceshardGfxGraphRuntime final : public GfxGraphRuntime
    {
    public:
        IceshardGfxGraphRuntime(
            ice::Allocator& alloc,
            ice::gfx::GfxDevice& device,
            ice::render::RenderSwapchain const& swapchain,
            ice::render::Renderpass renderpass,
            ice::Array<Snapshot> snapshots,
            ice::Array<GfxResource> resources,
            IceshardGfxStages stages
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

        ice::Array<Snapshot> _snapshots;
        ice::Array<GfxResource> _resources;
        ice::Array<ice::render::Image> _framebuffer_images;
        ice::Array<ice::render::Framebuffer> _framebuffers;
        ice::render::Renderpass _renderpass;
        ice::Array<vec4f> _clears;
        IceshardGfxStages _stages;
    };

} // namespace ice::gfx::v3
