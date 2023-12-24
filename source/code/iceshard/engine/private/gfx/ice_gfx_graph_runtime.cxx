#include "ice_gfx_graph_runtime.hxx"
#include "ice_gfx_graph.hxx"

#include <ice/gfx/gfx_device.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/container/array.hxx>
#include <ice/sort.hxx>

namespace ice::gfx
{

    bool postprocess_snapshots(
        ice::Span<GfxGraphSnapshot> snapshots,
        ice::Array<GraphBarrier>& out_barriers
    ) noexcept
    {
        ice::u32 last_pass = 0;
        for (ice::u32 idx = 0; idx < ice::count(snapshots); ++idx)
        {
            GfxGraphSnapshot& current = snapshots[idx];
            if (current.event & (GfxSnapshotEvent::EventBeginPass | GfxSnapshotEvent::EventNextSubPass | GfxSnapshotEvent::EventEndPass))
            {
                last_pass = ice::u32(current.resource.value);
                continue;
            }

            if (current.resource == Const_ResourceFrameBuffer)
            {
                continue;
            }

            GfxGraphSnapshot prev{}, next{};
            for (GfxGraphSnapshot const prev_candidate : ice::span::subspan(snapshots, 0, idx))
            {
                if (current.resource == prev_candidate.resource)
                {
                    prev = prev_candidate;
                }
            }

            for (GfxGraphSnapshot const next_candidate : ice::span::subspan(snapshots, idx + 1))
            {
                if (current.resource == next_candidate.resource)
                {
                    next = next_candidate;
                }
            }

            if (prev.event == GfxSnapshotEvent::EventInvalid)
            {
                current.event = GfxSnapshotEvent(current.event | GfxSnapshotEvent::EventCreateRes);
            }
            if (next.event == GfxSnapshotEvent::EventInvalid)
            {
                current.event = GfxSnapshotEvent(current.event | GfxSnapshotEvent::EventDeleteRes);
            }

            ICE_ASSERT_CORE(prev.event != GfxSnapshotEvent::EventInvalid || next.event != GfxSnapshotEvent::EventInvalid);
            if (prev.event != GfxSnapshotEvent::EventInvalid)
            {
                current.info = ice::array::count(out_barriers);

                if (current.event & GfxSnapshotEvent::EventReadRes)
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

        ice::Array<GfxGraphSnapshot> snapshots{ alloc };
        ice::Array<GfxResource> resources{ alloc };
        ice::array::push_back(resources, base_definition.get_framebuffer());

        ice::u32 pass_idx = 0;
        ice::array::push_back(
            snapshots,
            GfxGraphSnapshot{
                .subpass = 0,
                .resource = { pass_idx++ },
                .event = GfxSnapshotEvent::EventBeginPass,
                .info = 0,
            }
        );

        IceshardGfxGraphStages stages{ alloc };
        for (GfxGraphPass const& pass : base_definition.passes())
        {
            ice::array::push_back(
                snapshots,
                GfxGraphSnapshot{
                    .subpass = pass_idx,
                    .resource = { pass_idx },
                    .event = GfxSnapshotEvent::EventNextSubPass,
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
                        GfxGraphSnapshot{
                            .subpass = pass_idx,
                            .resource = res,
                            .event = GfxSnapshotEvent::EventReadRes,
                            .info = ice::u32_max,
                        }
                    );
                }
                for (GfxResource res : stage.outputs)
                {
                    ice::array::push_back(
                        snapshots,
                        GfxGraphSnapshot{
                            .subpass = pass_idx,
                            .resource = res,
                            .event = GfxSnapshotEvent::EventWriteRes,
                            .info = ice::u32_max,
                        }
                    );
                }
                for (GfxResource res : stage.depth_stencil)
                {
                    ice::array::push_back(
                        snapshots,
                        GfxGraphSnapshot{
                            .subpass = pass_idx,
                            .resource = res,
                            .event = GfxSnapshotEvent(GfxSnapshotEvent::EventReadRes | GfxSnapshotEvent::EventWriteRes),
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
            GfxGraphSnapshot{
                .subpass = pass_idx,
                .resource = { pass_idx },
                .event = GfxSnapshotEvent::EventEndPass,
                .info = 2,
            }
        );


        ice::Array<GraphBarrier> barriers{ alloc };
        postprocess_snapshots(snapshots, barriers);

        ice::ucount max_images = 0;
        ice::ucount current_images = 0;
        ice::ucount removed_images = 0;
        for (GfxGraphSnapshot const snapshot : snapshots)
        {
            if (snapshot.event & GfxSnapshotEvent::MaskPass)
            {
                max_images = ice::max(max_images, current_images);
                current_images -= removed_images;

                removed_images = 0;
            }
            else
            {
                bool const res_created = (snapshot.event & GfxSnapshotEvent::EventCreateRes) != 0;
                if (res_created)
                {
                    ice::array::push_back(resources, snapshot.resource);
                }

                current_images += ice::u32(res_created);
                removed_images += ice::u32((snapshot.event & GfxSnapshotEvent::EventDeleteRes) != 0);
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

} // namespace ice::gfx
