/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_graph_runtime.hxx>
#include <ice/mem_allocator_proxy.hxx>
#include <ice/container/hashmap.hxx>

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
            : _ready{ 0 }
            , _revision{ 0 }
            , _counts{ alloc }
            , _stage_names{ alloc }
            , _stages{ alloc }
        { }

        IceshardGfxGraphStages(IceshardGfxGraphStages&& other) noexcept
            : _ready{ 0 }
            , _revision{ 0 }
            , _counts{ ice::move(other._counts) }
            , _stage_names{ ice::move(other._stage_names) }
            , _stages{ ice::move(other._stages) }
        {
            ICE_ASSERT_CORE(other._ready == 0);
            ICE_ASSERT_CORE(other._revision == 0);
        }

        std::atomic_uint32_t _ready;
        ice::u32 _revision;
        ice::Array<ice::u8> _counts;
        ice::Array<ice::StringID> _stage_names;

        struct Entry
        {
            ice::gfx::GfxStage* stage;
            ice::u32 revision = 0;
            bool initialized = false;
        };

        ice::HashMap<Entry*> _stages;

        template<typename Method, typename... Args>
        void apply_stages(ice::StringID_Arg key, Method fn, Args&&... args) noexcept
        {
            Entry* const entry = ice::hashmap::get(_stages, ice::hash(key), nullptr);
            if (entry != nullptr)
            {
                ((entry->stage)->*fn)(ice::forward<Args>(args)...);
            }
        }
    };

    class IceshardGfxGraphRuntime final : public ice::gfx::GfxGraphRuntime
    {
    public:
        IceshardGfxGraphRuntime(
            ice::Allocator& alloc,
            ice::gfx::GfxContext& device,
            ice::render::RenderSwapchain const& swapchain,
            ice::render::Renderpass renderpass,
            ice::Array<ice::gfx::GfxGraphSnapshot> snapshots,
            ice::Array<ice::gfx::GfxResource> resources,
            ice::gfx::IceshardGfxGraphStages stages
        ) noexcept;
        ~IceshardGfxGraphRuntime() noexcept override;

        bool prepare(
            ice::gfx::GfxFrameStages& stages,
            ice::gfx::GfxStageRegistry const& stage_registry,
            ice::TaskContainer& out_tasks
        ) noexcept override;

        bool execute(
            ice::EngineFrame const& frame,
            ice::render::RenderFence& fence
        ) noexcept override;

    private:
        bool execute_pass(
            ice::EngineFrame const& frame,
            ice::render::Framebuffer framebuffer,
            ice::render::RenderCommands& api,
            ice::render::CommandBuffer cmds
        ) noexcept;

    private:
        ice::ProxyAllocator _allocator;
        GfxContext& _context;
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
