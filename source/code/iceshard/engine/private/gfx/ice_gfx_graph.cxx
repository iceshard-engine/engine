/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "ice_gfx_graph.hxx"
#include "ice_gfx_graph_runtime.hxx"
#include "ice_gfx_graph_snapshot.hxx"

#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_queue.hxx>

#include <ice/render/render_swapchain.hxx>
#include <ice/render/render_framebuffer.hxx>
#include <ice/render/render_device.hxx>
#include <ice/render/render_image.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_pipeline.hxx>
#include <ice/render/render_driver.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/render/render_command_buffer.hxx>
#include <ice/render/render_fence.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/sort.hxx>
#include <ice/profiler.hxx>
#include <chrono>

namespace ice::gfx
{

    IceshardGfxGraph::IceshardGfxGraph(ice::Allocator& alloc) noexcept
        : _passes{ alloc }
        , _resources{ alloc }
        , _resources_ids{ 1, 1, 1 }
    {
    }

    auto IceshardGfxGraph::get_framebuffer() const noexcept -> GfxResource
    {
        return Const_ResourceFrameBuffer;
    }

    auto IceshardGfxGraph::get_resource(ice::StringID_Arg name, GfxResourceType type) noexcept -> GfxResource
    {
        ICE_ASSERT_CORE(type != GfxResourceType::Invalid);

        ice::uptr const typeval = ice::u8(type);
        ice::u32 const id = _resources_ids[0]++;
        return { (typeval << 32) + id };
    }

    bool IceshardGfxGraph::add_pass(GfxGraphPass const& pass) noexcept
    {
        if (ice::hashmap::has(_passes, ice::hash(pass.name)) == false)
        {
            ice::hashmap::set(_passes, ice::hash(pass.name), pass);
        }
        return true;
    }

    auto IceshardGfxGraph::passes() const noexcept -> ice::Span<GfxGraphPass const>
    {
        return ice::hashmap::values(_passes);
    }

    void create_framebuffers(
        ice::Allocator& alloc,
        ice::render::RenderDevice& render_device,
        ice::render::RenderSwapchain const& swapchain,
        ice::render::Renderpass render_pass,
        ice::Span<GfxResource> resources,
        ice::Array<ice::render::Image>& out_images,
        ice::Array<ice::render::Framebuffer>& out_framebuffers
    ) noexcept
    {
        using namespace ice::render;

        ice::vec2u extent = swapchain.extent();

        ice::ucount const count_images = ice::count(resources);

        ice::array::clear(out_images);
        ice::array::reserve(out_images, count_images);
        ice::array::push_back(out_images, Image::Invalid); // First image beeing framebuffer

        for (GfxResource res : ice::span::subspan(resources, 1))
        {
            ImageInfo image_info {
                .type = ImageType::Image2D,
                .width = extent.x,
                .height = extent.y,
            };
            if (gfx_resource_type(res) == GfxResourceType::RenderTarget)
            {
                image_info.format = swapchain.image_format();
                image_info.usage = ImageUsageFlags::ColorAttachment
                    | ImageUsageFlags::InputAttachment
                    | ImageUsageFlags::Sampled;
            }
            else
            {
                image_info.format = ImageFormat::SFLOAT_D32_UINT_S8;
                image_info.usage = ImageUsageFlags::DepthStencilAttachment;
            }

            ice::array::push_back(
                out_images,
                render_device.create_image(image_info, {})
            );
        }

        ice::u32 fb_idx = 0;
        for (Framebuffer& out_framebuffer : out_framebuffers)
        {
            out_images[0] = swapchain.image(fb_idx++);
            out_framebuffer = render_device.create_framebuffer(extent, render_pass, out_images);
        }

        out_images[0] = Image::Invalid;
    }

    IceshardGfxGraphRuntime::IceshardGfxGraphRuntime(
        ice::Allocator& alloc,
        ice::gfx::GfxDevice& device,
        ice::render::RenderSwapchain const& swapchain,
        ice::render::Renderpass renderpass,
        ice::Array<GfxGraphSnapshot> GfxGraphSnapshots,
        ice::Array<GfxResource> resources,
        ice::gfx::IceshardGfxGraphStages stages
    ) noexcept
        : GfxGraphRuntime{}
        , _allocator{ alloc }
        , _device{ device }
        , _swapchain{ swapchain }
        , _snapshots{ ice::move(GfxGraphSnapshots) }
        , _resources{ ice::move(resources) }
        , _framebuffer_images{ _allocator }
        , _framebuffers{ _allocator }
        , _renderpass{ renderpass }
        , _clears{ _allocator }
        , _stages{ ice::move(stages) }
    {
        ice::array::resize(_framebuffers, _swapchain.image_count());

        create_framebuffers(
            _allocator,
            _device.device(),
            _swapchain,
            _renderpass,
            _resources,
            _framebuffer_images,
            _framebuffers
        );

        ice::array::resize(_clears, ice::count(_resources));
        for (vec4f& ccolor : _clears)
        {
            ccolor = vec4f{ 0.2f };
        }
    }

    IceshardGfxGraphRuntime::~IceshardGfxGraphRuntime() noexcept
    {
        render::RenderDevice& device = _device.device();

        for (ice::render::Framebuffer framebuffer : _framebuffers)
        {
            device.destroy_framebuffer(framebuffer);
        }
        for (ice::render::Image image : ice::array::slice(_framebuffer_images, 1))
        {
            device.destroy_image(image);
        }

        device.destroy_renderpass(_renderpass);
    }

    auto IceshardGfxGraphRuntime::renderpass() const noexcept -> ice::render::Renderpass
    {
        return _renderpass;
    }

    bool IceshardGfxGraphRuntime::execute(
        ice::EngineFrame const& frame,
        ice::gfx::GfxStageRegistry const& stage_registry,
        ice::render::RenderFence& fence
    ) noexcept
    {
        IPT_ZONE_SCOPED;

        ice::ucount const fb_idx = _device.next_frame();
        render::RenderDevice& device = _device.device();

        ice::render::CommandBuffer command_buffer;
        ice::gfx::GfxQueue* queue;
        _device.queue_group(fb_idx).get_queue(ice::render::QueueFlags::Graphics, queue);

        queue->reset();
        queue->request_command_buffers(render::CommandBufferType::Primary, { &command_buffer, 1 });

        if (execute_pass(frame, stage_registry, _framebuffers[fb_idx], device.get_commands(), command_buffer))
        {
            IPT_ZONE_SCOPED_NAMED("gfx_draw_commands");
            queue->submit_command_buffers({ &command_buffer, 1 }, &fence);
            fence.wait(10'000'000);
            _device.present(_swapchain.current_image_index());
            return true;
        }
        return false;
    }

    bool IceshardGfxGraphRuntime::execute_pass(
        ice::EngineFrame const& frame,
        GfxStageRegistry const& stage_registry,
        ice::render::Framebuffer framebuffer,
        ice::render::RenderCommands& api,
        ice::render::CommandBuffer cmds
    ) noexcept
    {
        IPT_ZONE_SCOPED;
        api.begin(cmds);
        bool first_skipped = false;

        ice::u32 pass_idx = 0;
        ice::u32 stage_idx = 0;
        for (GfxGraphSnapshot const& GfxGraphSnapshot : _snapshots)
        {
            if ((GfxGraphSnapshot.event & GfxSnapshotEvent::EventBeginPass) == GfxSnapshotEvent::EventBeginPass)
            {
                api.begin_renderpass(cmds, _renderpass, framebuffer, _clears, _swapchain.extent());
            }
            else if (GfxGraphSnapshot.event & GfxSnapshotEvent::EventNextSubPass && ice::exchange(first_skipped, true))
            {
                for (ice::StringID_Arg stage : ice::array::slice(_stages._stages, stage_idx, _stages._counts[pass_idx]))
                {
                    stage_registry.execute_stages(frame, stage, cmds, api);
                }

                api.next_subpass(cmds, render::SubPassContents::Inline);
                stage_idx += _stages._counts[pass_idx];
                pass_idx += 1;
            }
            else if (GfxGraphSnapshot.event & GfxSnapshotEvent::EventEndPass)
            {
                IPT_ZONE_SCOPED_NAMED("graph_execute_stages");
                for (ice::StringID_Arg stage : ice::array::slice(_stages._stages, stage_idx, _stages._counts[pass_idx]))
                {
                    stage_registry.execute_stages(frame, stage, cmds, api);
                }

                api.end_renderpass(cmds);
            }
            else
            {
            }
        }
        api.end(cmds);
        return true;
    }

    auto find_snapshot_event(
        ice::Span<GfxGraphSnapshot const> snapshots,
        GfxResource current
    ) noexcept
    {
        GfxSnapshotEvent result = GfxSnapshotEvent::EventCreateRes;

        bool pass_boundary = false;
        for (ice::u32 idx = ice::count(snapshots) - 1; idx >= 0; --idx)
        {
            GfxGraphSnapshot const prev = snapshots[idx];

            if (prev.event == GfxSnapshotEvent::EventNextSubPass)
            {
                pass_boundary = true;
                continue;
            }

            if (prev.resource == current)
            {
                if (pass_boundary)
                {
                    if ((prev.event & GfxSnapshotEvent::EventWriteRes) != 0)
                    {
                    }
                    else
                    {
                    }
                }
                else
                {
                }
            }
        }
        return result;
    }

    auto create_graph(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<GfxGraph>
    {
        return ice::make_unique<IceshardGfxGraph>(alloc, alloc);
    }

} // namespace ice::gfx
