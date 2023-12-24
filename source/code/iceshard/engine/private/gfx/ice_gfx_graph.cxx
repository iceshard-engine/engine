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
    auto gfx_resource_type_val(GfxResource res) noexcept
    {
        return ice::u32((res.value & 0x0000'00ff'0000'0000) >> 32);
    }

    auto gfx_resource_type(GfxResource res) noexcept
    {
        return static_cast<GfxResourceType>(gfx_resource_type_val(res));
    }

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
        ice::Span<GfxGraphSnapshot const> GfxGraphSnapshots,
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
        ice::array::reserve(references, ice::count(GfxGraphSnapshots));

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
        for (GfxGraphSnapshot const GfxGraphSnapshot : GfxGraphSnapshots)
        {
            [[maybe_unused]]
            ice::u32 const type_idx = gfx_resource_type_val(GfxGraphSnapshot.resource);
            if (GfxGraphSnapshot.event & GfxSnapshotEvent::MaskPass)
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

                if (GfxGraphSnapshot.event & GfxSnapshotEvent::EventNextSubPass && subpass_idx > 1)
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
                GfxResourceType const type = gfx_resource_type(GfxGraphSnapshot.resource);

                ice::u32 idx = 0;
                if (ice::binary_search(resources, GfxGraphSnapshot.resource, [](GfxResource lhs, GfxResource rhs) noexcept { return (lhs.value & 0xffff) < (rhs.value & 0xffff); }, idx))
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
                    else if (GfxGraphSnapshot.event & GfxSnapshotEvent::EventWriteRes)
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
        ice::Array<GfxGraphSnapshot> GfxGraphSnapshots,
        ice::Array<GfxResource> resources,
        ice::gfx::IceshardGfxGraphStages stages
    ) noexcept
        : GfxGraphRuntime{}
        , _allocator{ alloc }
        , _device{ device }
        , _swapchain{ swapchain }
        //, _framebuffers{ framebuffers }
        //, _framebuffers_count{ ice::u8(_framebuffers.framebuffers[2] == render::Framebuffer::Invalid ? 2 : 3) }
        //, _stages{ alloc }
        //, _fence{ }
        , _snapshots{ ice::move(GfxGraphSnapshots) }
        , _resources{ ice::move(resources) }
        , _framebuffer_images{ _allocator }
        , _framebuffers{ _allocator }
        , _renderpass{ renderpass }
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

    auto find_GfxGraphSnapshot_event(ice::Span<GfxGraphSnapshot const> snapshots, GfxResource current) noexcept
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
