/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "ice_gfx_render_graph_v3.hxx"
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

namespace ice::gfx::v3
{
    auto gfx_resource_type_val(GfxResource res) noexcept
    {
        return ice::u32((res.value & 0x0000'00ff'0000'0000) >> 32);
    }

    auto gfx_resource_type(GfxResource res) noexcept
    {
        return static_cast<GfxResourceType>(gfx_resource_type_val(res));
    }

    static constexpr GfxResource Const_ResourceFrameBuffer{ 0x8000'0000 };

    IceshardGfxGraph::IceshardGfxGraph(ice::Allocator& alloc) noexcept
        : _passes{ alloc }
        , _resources{ alloc }
        , _resources_ids{ 1, 1, 1 }
    {
        //ice::hashmap::set(
        //    _passes,
        //    ice::hash("framebuffer"_sid),
        //    GfxGraphPass{ .name = "framebuffer"_sid, .stages = {} }
        //);
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

    auto create_renderpass(
        ice::Allocator& alloc,
        ice::render::RenderDevice& render_device,
        ice::render::RenderSwapchain const& swapchain,
        ice::Span<GfxResource> resources,
        ice::Span<Snapshot const> snapshots,
        ice::u32 max_image_count
    ) noexcept -> ice::render::Renderpass
    {
        using namespace ice::render;
        ICE_ASSERT_CORE(max_image_count <= 4);

        ice::Array<RenderAttachment> attachments{ alloc };
        ice::Array<AttachmentReference> references{ alloc };
        ice::Array<RenderSubPass> subpasses{ alloc };
        ice::Array<SubpassDependency> dependencies{ alloc };

        ice::array::reserve(attachments, ice::count(resources));
        ice::array::reserve(references, ice::count(snapshots));

        ice::array::push_back(
            attachments,
            RenderAttachment{
                .format = swapchain.image_format(),
                .final_layout = ImageLayout::Present,
                .type = AttachmentType::SwapchainImage,
                .operations = {
                    AttachmentOperation::Load_Clear,
                    AttachmentOperation::Store_Store
                },
            }
        );

        for (GfxResource const res : resources)
        {
            GfxResourceType const type = gfx_resource_type(res);
            if (type == GfxResourceType::Invalid)
            {

            }
            else if (type == GfxResourceType::RenderTarget)
            {
                ice::array::push_back(
                    attachments,
                    RenderAttachment{
                        .format = swapchain.image_format(),
                        .final_layout = ImageLayout::ShaderReadOnly,
                        .type = AttachmentType::TextureImage,
                        .operations = {
                            AttachmentOperation::Load_Clear,
                            AttachmentOperation::Store_DontCare,
                        },
                    }
                );
            }
            else // if (type == GfxResourceType::DepthStencil)
            {
                ice::array::push_back(
                    attachments,
                    RenderAttachment{
                        .format = ImageFormat::SFLOAT_D32_UINT_S8,
                        .final_layout = ImageLayout::DepthStencil,
                        .type = AttachmentType::DepthStencil,
                        .operations = {
                            AttachmentOperation::Load_Clear
                        }
                    }
                );
            }
        }

        ice::u32 counts[3]{};
        ice::u32 ref_subpass_idx = 0;
        ice::u32 subpass_idx = 0;
        for (Snapshot const snapshot : snapshots)
        {
            [[maybe_unused]]
            ice::u32 const type_idx = gfx_resource_type_val(snapshot.resource);
            if (snapshot.event & Snapshot::MaskPass)
            {
                if (subpass_idx > 1)
                {
                    ice::array::push_back(
                        subpasses,
                        RenderSubPass{
                            .input_attachments = ice::array::slice(references, ref_subpass_idx, counts[0]),
                            .color_attachments = ice::array::slice(references, ref_subpass_idx + counts[0], counts[1]),
                            .depth_stencil_attachment = counts[2] == 0 ? AttachmentReference{ } : references[ref_subpass_idx + counts[0] + counts[1]],
                        }
                    );

                    ref_subpass_idx += counts[0] + counts[1] + counts[2];
                    counts[0] = counts[1] = counts[2] = 0;
                }

                if (snapshot.event & Snapshot::EventNextSubPass && subpass_idx > 1)
                {
                    if (subpass_idx == 2)
                    {
                        ice::array::push_back(
                            dependencies,
                            SubpassDependency{
                                .source_subpass = subpass_idx - 2,
                                .source_stage = PipelineStage::ColorAttachmentOutput,
                                .source_access = AccessFlags::ColorAttachmentWrite,
                                .destination_subpass = subpass_idx - 1,
                                .destination_stage = PipelineStage::ColorAttachmentOutput,
                                .destination_access = AccessFlags::ColorAttachmentWrite,
                            }
                        );
                    }
                    else
                    {
                        ice::array::push_back(
                            dependencies,
                            SubpassDependency{
                                .source_subpass = subpass_idx - 2,
                                .source_stage = PipelineStage::ColorAttachmentOutput,
                                .source_access = AccessFlags::ColorAttachmentWrite,
                                .destination_subpass = subpass_idx - 1,
                                .destination_stage = PipelineStage::FramentShader,
                                .destination_access = AccessFlags::InputAttachmentRead
                            }
                        );
                    }
                }
                subpass_idx += 1;
            }
            else
            {
                GfxResourceType const type = gfx_resource_type(snapshot.resource);

                ice::u32 idx = 0;
                if (ice::binary_search(resources, snapshot.resource, [](GfxResource lhs, GfxResource rhs) noexcept { return (lhs.value & 0xffff) < (rhs.value & 0xffff); }, idx))
                {
                    if (type == GfxResourceType::DepthStencil)
                    {
                        counts[2] += 1;
                        ice::array::push_back(
                            references,
                            AttachmentReference{
                                .attachment_index = idx,
                                .layout = ImageLayout::DepthStencil
                            }
                        );
                    }
                    else if (snapshot.event & Snapshot::EventWriteRes)
                    {
                        counts[1] += 1;
                        ice::array::push_back(
                            references,
                            AttachmentReference{
                                .attachment_index = idx,
                                .layout = ImageLayout::Color
                            }
                        );
                    }
                    else
                    {
                        counts[0] += 1;
                        ice::array::push_back(
                            references,
                            AttachmentReference{
                                .attachment_index = idx,
                                .layout = ImageLayout::ShaderReadOnly
                            }
                        );
                    }
                }
                else
                {
                    ICE_ASSERT_CORE(false);
                }
            }
        }

        RenderpassInfo renderpass_info{
            .attachments = attachments,
            .subpasses = subpasses,
            .dependencies = dependencies,
        };

        return render_device.create_renderpass(renderpass_info);
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

    //void internal_begin_renderpass(
    //    ice::render::CommandBuffer cmds,
    //    ice::render::RenderCommands& api,
    //    IceshardGfxGraphRuntime::Framebuffers& ctx,
    //    ice::u32 framebuffer_idx
    //) noexcept
    //{
    //    ice::vec4f clear_values[4]
    //    {
    //        ice::vec4f{ 0.3f },
    //        ice::vec4f{ 0.3f },
    //        ice::vec4f{ 1.0f },
    //        ice::vec4f{ 1.0f },
    //    };

    //    api.begin_renderpass(
    //        cmds,
    //        ctx.pass,
    //        ctx.framebuffers[framebuffer_idx],
    //        clear_values,
    //        ctx.extent
    //    );
    //}

    IceshardGfxGraphRuntime::IceshardGfxGraphRuntime(
        ice::Allocator& alloc,
        ice::gfx::GfxDevice& device,
        ice::render::RenderSwapchain const& swapchain,
        ice::render::Renderpass renderpass,
        ice::Array<Snapshot> snapshots,
        ice::Array<GfxResource> resources,
        IceshardGfxStages stages
    ) noexcept
        : GfxGraphRuntime{}
        , _allocator{ alloc }
        , _device{ device }
        , _swapchain{ swapchain }
        , _renderpass{ renderpass }
        //, _framebuffers{ framebuffers }
        //, _framebuffers_count{ ice::u8(_framebuffers.framebuffers[2] == render::Framebuffer::Invalid ? 2 : 3) }
        //, _stages{ alloc }
        //, _fence{ }
        , _snapshots{ ice::move(snapshots) }
        , _resources{ ice::move(resources) }
        , _framebuffer_images{ _allocator }
        , _framebuffers{ _allocator }
        , _clears{ _allocator }
        , _stages{ ice::move(stages) }
    {
        //_fence = _device.device().create_fence();
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
        //_fence->wait(1'000'000);
        render::RenderDevice& device = _device.device();
        //device.destroy_fence(_fence);

        for (ice::render::Framebuffer framebuffer : _framebuffers)
        {
            device.destroy_framebuffer(framebuffer);
        }
        for (ice::render::Image image : ice::array::slice(_framebuffer_images, 1))
        {
            device.destroy_image(image);
        }

        device.destroy_renderpass(_renderpass);
        //device.destroy_framebuffer(_framebuffers.framebuffers[0]);
        //device.destroy_framebuffer(_framebuffers.framebuffers[1]);
        //if (_framebuffers_count > 2) device.destroy_framebuffer(_framebuffers.framebuffers[2]);
        //device.destroy_image(_framebuffers.attach_color);
        //device.destroy_image(_framebuffers.attach_depth);
        //device.destroy_renderpass(_framebuffers.pass);
    }

    auto IceshardGfxGraphRuntime::renderpass() const noexcept -> ice::render::Renderpass
    {
        return _renderpass;
    }

    bool IceshardGfxGraphRuntime::execute(
        ice::EngineFrame const& frame,
        GfxStageRegistry const& stage_registry,
        ice::render::RenderFence& fence
    ) noexcept
    {
        ice::ucount const fb_idx = _device.next_frame();
        render::RenderDevice& device = _device.device();

        ice::render::CommandBuffer command_buffer;
        ice::gfx::GfxQueue* queue;
        _device.queue_group(fb_idx).get_queue(ice::render::QueueFlags::Graphics, queue);

        queue->reset();
        queue->request_command_buffers(render::CommandBufferType::Primary, { &command_buffer, 1 });

        if (execute_pass(frame, stage_registry, _framebuffers[fb_idx], device.get_commands(), command_buffer))
        {
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
        api.begin(cmds);
        bool first_skipped = false;

        ice::u32 pass_idx = 0;
        ice::u32 stage_idx = 0;
        for (Snapshot const& snapshot : _snapshots)
        {
            if ((snapshot.event & Snapshot::EventBeginPass) == Snapshot::EventBeginPass)
            {
                api.begin_renderpass(cmds, _renderpass, framebuffer, _clears, _swapchain.extent());
            }
            else if (snapshot.event & Snapshot::EventNextSubPass && ice::exchange(first_skipped, true))
            {
                for (ice::StringID_Arg stage : ice::array::slice(_stages._stages, stage_idx, _stages._counts[pass_idx]))
                {
                    stage_registry.execute_stages(frame, stage, cmds, api);
                }

                api.next_subpass(cmds, render::SubPassContents::Inline);
                stage_idx += _stages._counts[pass_idx];
                pass_idx += 1;
            }
            else if (snapshot.event & Snapshot::EventEndPass)
            {
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

    //bool IceshardGfxGraphRuntime::execute(
    //    GfxStageRegistry const& stage_registry
    //) noexcept
    //{
    //    IPT_ZONE_SCOPED;
    //    IPT_FRAME_MARK_NAMED("Graphics");

    //    ice::ucount const fb_idx = _swapchain.current_image_index();

    //    constexpr ice::StringID framebuffer_names[]{ "ice.gfx.framebuffer.0"_sid, "ice.gfx.framebuffer.1"_sid };

    //    ice::render::RenderDevice& device = _device.device();

    //    ice::render::CommandBuffer command_buffer;
    //    ice::gfx::GfxQueue* queue;
    //    _device.queue_group(fb_idx).get_queue(ice::render::QueueFlags::Graphics, queue);

    //    queue->reset();
    //    queue->request_command_buffers(render::CommandBufferType::Primary, { &command_buffer, 1 });

    //    auto& api = device.get_commands();
    //    internal_begin_renderpass(command_buffer, api, _framebuffers, fb_idx);
    //    //for (ice::StringID_Arg stage_name : _render_graph.runtime_stages())
    //    //{
    //    //    auto it = ice::multi_hashmap::find_first(_stages, ice::hash(stage_name));
    //    //    while (it != nullptr)
    //    //    {
    //    //        it.value()->execute(graph_params, *_storage);
    //    //        it = ice::multi_hashmap::find_next(_stages, it);
    //    //    }
    //    //}

    //    for (GraphStage const& stage : _stages)
    //    {
    //        switch (stage.type)
    //        {
    //        case GraphStageType::Allocate: break;
    //        case GraphStageType::Release: break;
    //        case GraphStageType::Barrier: break;
    //        case GraphStageType::Draw:
    //            break;
    //        default:
    //            ICE_ASSERT_CORE(false);
    //            break;
    //        }
    //    }

    //    api.end_renderpass(command_buffer);

    //    queue->submit_command_buffers({ &command_buffer, 1 }, _fence);
    //    _fence->wait(10'000'000);
    //    _device.present(fb_idx);
    //    return true;
    //}

    auto find_snapshot_event(ice::Span<Snapshot const> snapshots, GfxResource current) noexcept
    {
        Snapshot::Event result = Snapshot::EventCreateRes;

        bool pass_boundary = false;
        for (ice::u32 idx = ice::count(snapshots) - 1; idx >= 0; --idx)
        {
            Snapshot const prev = snapshots[idx];

            if (prev.event == Snapshot::EventNextSubPass)
            {
                pass_boundary = true;
                continue;
            }

            if (prev.resource == current)
            {
                if (pass_boundary)
                {
                    if ((prev.event & Snapshot::EventWriteRes) != 0)
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

    bool postprocess_snapshots(
        ice::Span<Snapshot> snapshots,
        ice::Array<GraphBarrier>& out_barriers
    ) noexcept
    {
        ice::u32 last_pass = 0;
        for (ice::u32 idx = 0; idx < ice::count(snapshots); ++idx)
        {
            Snapshot& current = snapshots[idx];
            if (current.event & (Snapshot::EventBeginPass | Snapshot::EventNextSubPass | Snapshot::EventEndPass))
            {
                last_pass = ice::u32(current.resource.value);
                continue;
            }

            if (current.resource == Const_ResourceFrameBuffer)
            {
                continue;
            }

            Snapshot prev{}, next{};
            for (Snapshot const prev_candidate : ice::span::subspan(snapshots, 0, idx))
            {
                if (current.resource == prev_candidate.resource)
                {
                    prev = prev_candidate;
                }
            }

            for (Snapshot const next_candidate : ice::span::subspan(snapshots, idx + 1))
            {
                if (current.resource == next_candidate.resource)
                {
                    next = next_candidate;
                }
            }

            if (prev.event == Snapshot::EventInvalid)
            {
                current.event = Snapshot::Event(current.event | Snapshot::EventCreateRes);
            }
            if (next.event == Snapshot::EventInvalid)
            {
                current.event = Snapshot::Event(current.event | Snapshot::EventDeleteRes);
            }

            ICE_ASSERT_CORE(prev.event != Snapshot::EventInvalid || next.event != Snapshot::EventInvalid);
            if (prev.event != Snapshot::EventInvalid)
            {
                current.info = ice::array::count(out_barriers);

                if (current.event & Snapshot::EventReadRes)
                {
                    ice::array::push_back(
                        out_barriers,
                        GraphBarrier{
                            .pass_idx = last_pass,
                            .source_layout = render::ImageLayout::Color,
                            .destination_layout = render::ImageLayout::ShaderReadOnly,
                            .source_access = render::AccessFlags::ColorAttachmentWrite,
                            .destination_access = render::AccessFlags::ShaderRead,
                        }
                    );
                }
                else
                {
                    ice::array::push_back(
                        out_barriers,
                        GraphBarrier{
                            .pass_idx = last_pass,
                            .source_layout = render::ImageLayout::Undefined,
                            .destination_layout = render::ImageLayout::Color,
                            .source_access = render::AccessFlags::None,
                            .destination_access = render::AccessFlags::ColorAttachmentWrite,
                        }
                    );
                }
            }
        }
        return true;
    }

    auto create_graph(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<GfxGraph>
    {
        return ice::make_unique<IceshardGfxGraph>(alloc, alloc);
    }

    auto create_graph_runtime(
        ice::Allocator& alloc,
        GfxDevice& device,
        GfxGraph const& base_definition,
        GfxGraph const* dynamic_definition
    ) noexcept -> ice::UniquePtr<GfxGraphRuntime>
    {
        // TODO:
        ICE_ASSERT_CORE(dynamic_definition == nullptr);

        using namespace ice::render;

        ice::Array<Snapshot> snapshots{ alloc };
        ice::Array<GfxResource> resources{ alloc };
        ice::array::push_back(resources, base_definition.get_framebuffer());

        ice::u32 pass_idx = 0;
        ice::array::push_back(
            snapshots,
            Snapshot{
                .subpass = 0,
                .resource = { pass_idx++ },
                .event = Snapshot::EventBeginPass,
                .info = 0,
            }
        );

        IceshardGfxStages stages{ alloc };
        for (GfxGraphPass const& pass : base_definition.passes())
        {
            ice::array::push_back(
                snapshots,
                Snapshot{
                    .subpass = pass_idx,
                    .resource = { pass_idx },
                    .event = Snapshot::EventNextSubPass,
                    .info = 1,
                }
            );

            for (GfxGraphStage const& stage : pass.stages)
            {
                ice::array::push_back(stages._stages, stage.name);

                for (GfxResource res : stage.inputs)
                {
                    ice::array::push_back(
                        snapshots,
                        Snapshot{
                            .subpass = pass_idx,
                            .resource = res,
                            .event = Snapshot::EventReadRes,
                            .info = ice::u32_max,
                        }
                    );
                }
                for (GfxResource res : stage.outputs)
                {
                    ice::array::push_back(
                        snapshots,
                        Snapshot{
                            .subpass = pass_idx,
                            .resource = res,
                            .event = Snapshot::EventWriteRes,
                            .info = ice::u32_max,
                        }
                    );
                }
                for (GfxResource res : stage.depth_stencil)
                {
                    ice::array::push_back(
                        snapshots,
                        Snapshot{
                            .subpass = pass_idx,
                            .resource = res,
                            .event = Snapshot::Event(Snapshot::EventReadRes | Snapshot::EventWriteRes),
                            .info = ice::u32_max,
                        }
                    );
                }
            }

            ice::array::push_back(stages._counts, ice::u8(ice::count(pass.stages)));
            pass_idx += 1;
        }

        ice::array::push_back(
            snapshots,
            Snapshot{
                .subpass = pass_idx,
                .resource = { pass_idx },
                .event = Snapshot::EventEndPass,
                .info = 2,
            }
        );


        ice::Array<GraphBarrier> barriers{ alloc };
        postprocess_snapshots(snapshots, barriers);

        ice::ucount max_images = 0;
        ice::ucount current_images = 0;
        ice::ucount removed_images = 0;
        for (Snapshot const snapshot : snapshots)
        {
            if (snapshot.event & Snapshot::MaskPass)
            {
                max_images = ice::max(max_images, current_images);
                current_images -= removed_images;

                removed_images = 0;
            }
            else
            {
                bool const res_created = (snapshot.event & Snapshot::EventCreateRes) != 0;
                if (res_created)
                {
                    ice::array::push_back(resources, snapshot.resource);
                }

                current_images += ice::u32(res_created);
                removed_images += ice::u32((snapshot.event & Snapshot::EventDeleteRes) != 0);
            }
        }

        ice::sort(
            ice::array::slice(resources),
            [](GfxResource lhs, GfxResource rhs) noexcept { return (lhs.value & 0xffff) < (rhs.value & 0xffff); }
        );

        //IceshardGfxGraphRuntime::Framebuffers frame_buffers = create_framebuffers(
        //    alloc,
        //    device.device(),
        //    device.swapchain(),
        //    resources,
        //    snapshots,
        //    max_images
        //);
        //if (frame_buffers.framebuffers[1] == Framebuffer::Invalid)
        //{
        //    return {};
        //}

        Renderpass renderpass = create_renderpass(
            alloc,
            device.device(),
            device.swapchain(),
            resources,
            snapshots,
            max_images
        );

        if (renderpass == Renderpass::Invalid)
        {
            return {};
        }

        return ice::make_unique<IceshardGfxGraphRuntime>(
            alloc,
            alloc,
            device,
            device.swapchain(),
            renderpass,
            ice::move(snapshots),
            ice::move(resources),
            ice::move(stages)
        );
    }


} // namespace ice::gfx::v3
