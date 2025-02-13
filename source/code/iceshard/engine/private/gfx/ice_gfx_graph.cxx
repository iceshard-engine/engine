/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "ice_gfx_graph.hxx"
#include "ice_gfx_graph_runtime.hxx"
#include "ice_gfx_graph_snapshot.hxx"

#include <ice/gfx/gfx_context.hxx>
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
#include <ice/task_container.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/sort.hxx>
#include <ice/profiler.hxx>
#include <chrono>

namespace ice::gfx
{

    namespace detail
    {

        auto copy_gfx_pass_stage(ice::gfx::GfxGraphStage const& stage, ice::Array<ice::gfx::GfxResource>& out_resources) noexcept
        {
            ice::gfx::GfxGraphStage stage_copy{
                .name = stage.name
            };

            ice::u32 offset = ice::array::count(out_resources);
            for (ice::gfx::GfxResource res : stage.depth_stencil)
            {
                ice::array::push_back(out_resources, res);
            }
            stage_copy.depth_stencil = ice::array::slice(out_resources, offset, ice::count(stage.depth_stencil));
            offset += ice::count(stage.depth_stencil);

            for (ice::gfx::GfxResource res : stage.inputs)
            {
                ice::array::push_back(out_resources, res);
            }
            stage_copy.inputs = ice::array::slice(out_resources, offset, ice::count(stage.inputs));
            offset += ice::count(stage.inputs);

            for (ice::gfx::GfxResource res : stage.outputs)
            {
                ice::array::push_back(out_resources, res);
            }
            stage_copy.outputs = ice::array::slice(out_resources, offset, ice::count(stage.outputs));
            offset += ice::count(stage.inputs);

            return stage_copy;
        }

    } // namespace detail

    IceshardGfxGraph::IceshardGfxGraph(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _objects{ alloc }
        , _resources{ alloc }
        , _passes{ alloc }
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

        ice::u64 const typeval = ice::u8(type);
        ice::u32 const id = _resources_ids[0]++;
        return { (typeval << 32) + id };
    }

    bool IceshardGfxGraph::add_pass(GfxGraphPass const& pass) noexcept
    {
        if (ice::hashmap::has(_objects, ice::hash(pass.name)) == false)
        {
            ice::ucount res_count = 0;
            for (ice::gfx::GfxGraphStage const& stage : pass.stages)
            {
                res_count += ice::count(stage.depth_stencil) + ice::count(stage.inputs) + ice::count(stage.outputs);
            }

            IceshardGfxGraphPassObjects gfxpass{ _allocator, pass.name };
            ice::array::reserve(gfxpass.resources, res_count);
            ice::array::reserve(gfxpass.stages, ice::count(pass.stages));

            for (ice::gfx::GfxGraphStage const& stage : pass.stages)
            {
                ice::array::push_back(gfxpass.stages, detail::copy_gfx_pass_stage(stage, gfxpass.resources));
            }

            ice::array::push_back(_passes, GfxGraphPass{ .name = pass.name, .stages = gfxpass.stages });
            ice::hashmap::set(_objects, ice::hash(pass.name), ice::move(gfxpass));
        }
        return true;
    }

    auto IceshardGfxGraph::passes() const noexcept -> ice::Span<GfxGraphPass const>
    {
        return _passes;
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
        ice::gfx::GfxContext& context,
        ice::render::RenderSwapchain const& swapchain,
        ice::render::Renderpass renderpass,
        ice::Array<GfxGraphSnapshot> graph_snapshots,
        ice::Array<GfxResource> resources,
        ice::gfx::IceshardGfxGraphStages stages
    ) noexcept
        : GfxGraphRuntime{}
        , _allocator{ alloc, "gfx-graph-runtime" }
        , _context{ context }
        , _swapchain{ swapchain }
        , _snapshots{ ice::move(graph_snapshots) }
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
            _context.device(),
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
        render::RenderDevice& device = _context.device();

        for (IceshardGfxGraphStages::Entry* entry : _stages._stages)
        {
            _allocator.destroy(entry);
        }

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

    auto initialize_stage(ice::Task<> init_task, std::atomic_uint32_t& ready_count) noexcept -> ice::Task<>
    {
        co_await init_task;
        ready_count.fetch_sub(1, std::memory_order_relaxed);
    }

    bool IceshardGfxGraphRuntime::prepare(
        ice::gfx::GfxFrameStages& stages,
        ice::gfx::GfxStageRegistry const& stage_registry,
        ice::TaskContainer& out_tasks
    ) noexcept
    {
        IPT_ZONE_SCOPED;
        using Entry = ice::gfx::IceshardGfxGraphStages::Entry;

        // ice::u32 const prev_revision = _stages._revision++;
        ice::u32 new_stages = 0, removed_stages = 0;
        ice::u32 const new_revision = ++_stages._revision;

        ice::Array<ice::gfx::GfxStage*> temp_stages{ _allocator };
        if (stage_registry.query_stages(_stages._stage_names, temp_stages))
        {
            for (ice::u32 idx = 0; idx < ice::count(_stages._stage_names); ++idx)
            {
                ice::u64 const stage_hash = ice::hash(_stages._stage_names[idx]);
                GfxStage* stage_ptr = temp_stages[idx];

                // TODO: Add possible optionality

                // Check for the entry to be compared with
                Entry* entry = ice::hashmap::get(_stages._stages, stage_hash, nullptr);
                if (entry != nullptr && entry->stage != stage_ptr)
                {
                    removed_stages += 1;

                    // Push the cleanup task
                    if (entry->initialized)
                    {
                        out_tasks.create_tasks(1, "gfx.graph-runtime.state-cleanup"_shardid)[0] = entry->stage->cleanup(_context);
                        entry->initialized = false;
                    }

                    // Only remove if a new stage is not there to replace the current one
                    if (stage_ptr == nullptr)
                    {
                        // Remove the entry from the map.
                        ice::hashmap::remove(_stages._stages, stage_hash);
                    }

                    // Delete the entry object
                    _allocator.destroy(entry);
                }

                // Only remove if a new stage is not there to replace the current one
                if (stage_ptr != nullptr)
                {
                    // Only create a new entry if the old didn't exist
                    if (entry == nullptr)
                    {
                        new_stages += 1;
                        ice::hashmap::set(_stages._stages, stage_hash, _allocator.create<Entry>(stage_ptr, new_revision, false));
                    }
                    else if (entry->stage != stage_ptr)
                    {
                        new_stages += 1;
                        entry->stage = stage_ptr;
                    }
                }
            }

            if (new_stages == 0)
            {
                return removed_stages > 0;
            }

            // Reset the ready flag
            _stages._ready.fetch_add(new_stages);

            ice::u32 init_task_idx = 0;
            ice::Span<ice::Task<>> init_tasks = out_tasks.create_tasks(new_stages, "gfx.graph-runtime.state-init"_shardid);

            // Collect init tasks
            ice::u32 pass_idx = 0;
            ice::u32 pass_idx_stages = _stages._counts[0];

            auto it = ice::begin(_stages._stages);
            for (ice::StringID stagename : _stages._stage_names)
            {
                if (it.key() == ice::hash(stagename))
                {
                    Entry* entry = it.value();

                    // Find entries with old revisions
                    if (entry->initialized == false)
                    {
                        entry->initialized = true;

                        init_tasks[init_task_idx++] = initialize_stage(
                            entry->stage->initialize(_context, stages, _renderpass, pass_idx),
                            _stages._ready
                        );
                    }

                    ++it;
                }

                pass_idx_stages -= 1;
                if (pass_idx_stages == 0)
                {
                    pass_idx += 1;
                    pass_idx_stages = _stages._counts[pass_idx];
                }
            }
        }

        return (new_stages + removed_stages) > 0;
    }

    bool IceshardGfxGraphRuntime::execute(
        ice::EngineFrame const& frame,
        ice::render::RenderFence& fence
    ) noexcept
    {
        IPT_ZONE_SCOPED;

        if (_stages._ready.load(std::memory_order_relaxed) > 0)
        {
            return false;
        }

        ice::ucount const fb_idx = _context.next_frame();

        // TODO: Can we do this differently?
        if (fb_idx == ice::ucount_max)
        {
            return false;
        }

        render::RenderDevice& device = _context.device();

        ice::render::CommandBuffer command_buffer;
        ice::gfx::GfxQueue* queue;
        _context.queue_group(fb_idx).get_queue(ice::render::QueueFlags::Graphics, queue);

        queue->reset();
        queue->request_command_buffers(render::CommandBufferType::Primary, { &command_buffer, 1 });

        bool has_work = false;
        {
            IPT_ZONE_SCOPED_NAMED("gfx_execute_graph");
            has_work = execute_pass(frame, _framebuffers[fb_idx], device.get_commands(), command_buffer);
        }

        if (has_work)
        {
            IPT_ZONE_SCOPED_NAMED("gfx_gpu_work");

            // TODO: Fix and use semaphores to synchronize queue workd done and presentation.
            queue->submit_command_buffers({ &command_buffer, 1 }, &fence);
            fence.wait(100'000'000);
        }
        return has_work;
    }

    void IceshardGfxGraphRuntime::present() noexcept
    {
        IPT_ZONE_SCOPED_NAMED("gfx_present");
        _context.present(_swapchain.current_image_index());
    }

    bool IceshardGfxGraphRuntime::execute_pass(
        ice::EngineFrame const& frame,
        ice::render::Framebuffer framebuffer,
        ice::render::RenderCommands& api,
        ice::render::CommandBuffer cmds
    ) noexcept
    {
        {
            IPT_ZONE_SCOPED_NAMED("gfx_begin");
            api.begin(cmds);

            // Collect profiling zones
            IPR_COLLECT_ZONES(api, cmds);
        }
        bool first_skipped = false;

        ice::u32 pass_idx = 0;
        ice::u32 stage_idx = 0;
        for (GfxGraphSnapshot const& GfxGraphSnapshot : _snapshots)
        {
            if ((GfxGraphSnapshot.event & GfxSnapshotEvent::EventBeginPass) == GfxSnapshotEvent::EventBeginPass)
            {
                IPT_ZONE_SCOPED_NAMED("gfx_begin_renderpass");
                api.begin_renderpass(cmds, _renderpass, framebuffer, _clears, _swapchain.extent());
            }
            else if (GfxGraphSnapshot.event & GfxSnapshotEvent::EventNextSubPass && ice::exchange(first_skipped, true))
            {
                for (ice::StringID_Arg stage : ice::array::slice(_stages._stage_names, stage_idx, _stages._counts[pass_idx]))
                {
                    // TODO: Separate update and draw?
                    _stages.apply_stages(stage, &GfxStage::update, frame, _context);
                    _stages.apply_stages(stage, &GfxStage::draw, frame, cmds, api);
                }

                api.next_subpass(cmds, render::SubPassContents::Inline);
                stage_idx += _stages._counts[pass_idx];
                pass_idx += 1;
            }
            else if (GfxGraphSnapshot.event & GfxSnapshotEvent::EventEndPass)
            {
                {
                    IPT_ZONE_SCOPED_NAMED("graph_execute_stages");
                    for (ice::StringID_Arg stage : ice::array::slice(_stages._stage_names, stage_idx, _stages._counts[pass_idx]))
                    {
                        // TODO: Separate update and draw?
                        _stages.apply_stages(stage, &GfxStage::update, frame, _context);
                        _stages.apply_stages(stage, &GfxStage::draw, frame, cmds, api);
                    }
                }

                IPT_ZONE_SCOPED_NAMED("gfx_end_renderpass");
                api.end_renderpass(cmds);
            }
            else
            {
            }
        }

        {
            IPT_ZONE_SCOPED_NAMED("gfx_end");
            api.end(cmds);
        }
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
