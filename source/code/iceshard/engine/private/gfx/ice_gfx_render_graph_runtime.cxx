/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "ice_gfx_render_graph_runtime.hxx"
#include "ice_gfx_object_storage.hxx"
#include <ice/assert.hxx>
#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_queue.hxx>
#include <ice/gfx/gfx_render_graph.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/render/render_command_buffer.hxx>
#include <ice/render/render_device.hxx>
#include <ice/render/render_fence.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/mem_allocator_host.hxx>

namespace ice::gfx::v2
{

    //struct StageEntry
    //{
    //    ice::gfx::v2::GfxStageDefinition definition;
    //    ice::gfx::v2::GfxStage* stage;
    //};
    namespace builtin
    {

    }

    struct Stage_CreateFramebuffer final : ice::gfx::v2::GfxStage
    {
        void execute(
            ice::gfx::v2::GfxStageParams const& params,
            ice::gfx::v2::GfxObjectStorage& storage
        ) noexcept override
        {
            using namespace ice::render;
            RenderDevice& render_device = params.device.device();
            RenderSwapchain const& swapchain = params.device.swapchain();

            RenderAttachment attachments[]{
                RenderAttachment{
                    .format = swapchain.image_format(),
                    .final_layout = ImageLayout::ShaderReadOnly,
                    .type = AttachmentType::TextureImage,
                    .operations = {
                        AttachmentOperation::Load_Clear,
                        AttachmentOperation::Store_DontCare,
                    },
                },
                RenderAttachment{
                    .format = swapchain.image_format(),
                    .final_layout = ImageLayout::Present,
                    .type = AttachmentType::SwapchainImage,
                    .operations = {
                        AttachmentOperation::Load_Clear,
                        AttachmentOperation::Store_Store
                    },
                },
                RenderAttachment{
                    .format = ImageFormat::SFLOAT_D32_UINT_S8,
                    .final_layout = ImageLayout::DepthStencil,
                    .type = AttachmentType::DepthStencil,
                    .operations = {
                        AttachmentOperation::Load_Clear
                    }
                },
            };

            AttachmentReference references[]{
                AttachmentReference{
                    .attachment_index = 0,
                    .layout = ImageLayout::Color
                },
                AttachmentReference{
                    .attachment_index = 0,
                    .layout = ImageLayout::ShaderReadOnly
                },
                AttachmentReference{
                    .attachment_index = 1,
                    .layout = ImageLayout::Color
                },
                AttachmentReference{
                    .attachment_index = 2,
                    .layout = ImageLayout::DepthStencil
                },
            };

            RenderSubPass subpasses[]{
                RenderSubPass{
                    .color_attachments = { references + 0, 1 },
                },
                RenderSubPass{
                    .color_attachments = { references + 0, 1 },
                    .depth_stencil_attachment = references[3],
                },
                RenderSubPass{
                    .input_attachments = { references + 1, 1 },
                    .color_attachments = { references + 2, 1 },
                },
            };

            SubpassDependency dependencies[]{
                SubpassDependency{
                    .source_subpass = 0,
                    .source_stage = PipelineStage::ColorAttachmentOutput,
                    .source_access = AccessFlags::ColorAttachmentWrite,
                    .destination_subpass = 1,
                    .destination_stage = PipelineStage::ColorAttachmentOutput,
                    .destination_access = AccessFlags::ColorAttachmentWrite,
                },
                SubpassDependency{
                    .source_subpass = 1,
                    .source_stage = PipelineStage::ColorAttachmentOutput,
                    .source_access = AccessFlags::ColorAttachmentWrite,
                    .destination_subpass = 2,
                    .destination_stage = PipelineStage::FramentShader,
                    .destination_access = AccessFlags::InputAttachmentRead,
                }
            };

            RenderpassInfo renderpass_info{
                .attachments = attachments,
                .subpasses = subpasses,
                .dependencies = dependencies,
            };

            Renderpass default_renderpass = render_device.create_renderpass(renderpass_info);
            storage.set("ice.gfx.renderpass.default"_sid, default_renderpass);

            ice::vec2u extent = swapchain.extent();

            ImageInfo image_info;
            image_info.type = ImageType::Image2D;
            image_info.format = ImageFormat::SFLOAT_D32_UINT_S8;
            image_info.usage = ImageUsageFlags::DepthStencilAttachment;
            image_info.width = extent.x;
            image_info.height = extent.y;

            Image default_depthstencil_attachment = render_device.create_image(image_info, { });
            storage.set("ice.gfx.image.depthstencil-attachment"_sid, default_depthstencil_attachment);

            image_info.type = ImageType::Image2D;
            image_info.format = swapchain.image_format();
            image_info.usage = ImageUsageFlags::ColorAttachment | ImageUsageFlags::InputAttachment | ImageUsageFlags::Sampled;
            image_info.width = extent.x;
            image_info.height = extent.y;

            Image default_color_attachment = render_device.create_image(image_info, { });
            storage.set("ice.gfx.image.color-attachment"_sid, default_color_attachment);

            Image framebuffer_images[]{
                default_color_attachment,
                Image::Invalid,
                default_depthstencil_attachment
            };

            //SamplerInfo sampler_info{
            //    .min_filter = SamplerFilter::Linear,
            //    .mag_filter = SamplerFilter::Linear,
            //    .address_mode = {
            //        .u = SamplerAddressMode::Repeat,
            //        .v = SamplerAddressMode::Repeat,
            //        .w = SamplerAddressMode::Repeat,
            //    },
            //    .mip_map_mode = SamplerMipMapMode::Linear,
            //};

            //Sampler basic_sampler = render_device.create_sampler(sampler_info);

            Framebuffer default_framebuffers[2];

            framebuffer_images[1] = swapchain.image(0);
            default_framebuffers[0] = render_device.create_framebuffer(
                extent,
                default_renderpass,
                framebuffer_images
            );
            framebuffer_images[1] = swapchain.image(1);
            default_framebuffers[1] = render_device.create_framebuffer(
                extent,
                default_renderpass,
                framebuffer_images
            );

            storage.set("ice.gfx.framebuffer.0"_sid, default_framebuffers[0]);
            storage.set("ice.gfx.framebuffer.1"_sid, default_framebuffers[1]);
        }
    };

    struct Stage_Clear final : ice::gfx::v2::GfxStage
    {
        ice::vec4f const color;

        Stage_Clear(ice::gfx::v2::GfxStageDescription const& desc) noexcept
            //: color{ desc.data.constant.clear.color }
        { }

        void execute(
            ice::gfx::v2::GfxStageParams const& params,
            ice::gfx::v2::GfxObjectStorage& storage
        ) noexcept override
        {
            params.render_commands.begin(params.command_buffer);

            ice::vec4f const colors[]{ color, color, color, };
            params.render_commands.begin_renderpass(
                params.command_buffer,
                params.state.renderpass,
                params.state.framebuffer,
                colors,
                params.state.framebuffer_size
            );
            params.render_commands.next_subpass(params.command_buffer, render::SubPassContents::Inline);
            params.render_commands.next_subpass(params.command_buffer, render::SubPassContents::Inline);
        }
    };

    struct Stage_Present final : ice::gfx::v2::GfxStage
    {
        void execute(
            ice::gfx::v2::GfxStageParams const& params,
            ice::gfx::v2::GfxObjectStorage&
        ) noexcept override
        {
            params.render_commands.end_renderpass(params.command_buffer);
            params.render_commands.end(params.command_buffer);
        }
    };

    SimpleGfxRenderGraphRuntime::SimpleGfxRenderGraphRuntime(
        ice::Allocator& alloc,
        ice::gfx::v2::GfxRenderGraph const& render_graph,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
        : _allocator{ alloc }
        , _render_graph{ render_graph }
        , _gfx_device{ gfx_device }
        , _storage{ ice::make_unique<ice::gfx::v2::SimpleGfxObjectStorage>(alloc, alloc) }
        , _stages{ alloc }
        , _next_framebuffer_index{ 0 }
        , _swap_fence{ }
    {
        ice::hashmap::reserve(_stages, ice::ucount(ice::count(_render_graph.definitions()) * 1.3));

        for (GfxStageDefinition const& stage : _render_graph.definitions())
        {
            GfxStage* builtin_stage = nullptr;
            GfxStageDescription const& desc = stage.desc;

            switch (desc.flags)
            {
            case GfxStageFlags::Create_Framebuffer:
                builtin_stage = _allocator.create<Stage_CreateFramebuffer>();
                break;
            case GfxStageFlags::BuiltIn_Clear:
                builtin_stage = _allocator.create<Stage_Clear>(desc);
                break;
            case GfxStageFlags::BuiltIn_Present:
                builtin_stage = _allocator.create<Stage_Present>();
                break;
            default:
                break;
            }

            ICE_ASSERT(
                builtin_stage != nullptr || ice::has_none(desc.flags, GfxStageFlags::BuiltIn),
                "Invalid flags for stage {}, the given definition seems not to have a 'BuiltIn' representation.",
                stage.name
            );
            if (builtin_stage != nullptr)
            {
                bind_stage(stage.name, builtin_stage);
            }
        }
    }

    SimpleGfxRenderGraphRuntime::~SimpleGfxRenderGraphRuntime() noexcept
    {
        for (GfxStageDefinition const& stage : _render_graph.definitions())
        {
            if (ice::has_all(stage.desc.flags, GfxStageFlags::BuiltIn))
            {
                auto it = ice::multi_hashmap::find_first(_stages, ice::hash(stage.name));
                while (it != nullptr)
                {
                    _allocator.destroy(it.value());
                    it = ice::multi_hashmap::find_next(_stages, it);
                }
            }
        };

        if (_swap_fence != nullptr)
        {
            _storage->destroy_all(_gfx_device.device());
            _gfx_device.device().destroy_fence(_swap_fence);
        }
    }

    auto SimpleGfxRenderGraphRuntime::storage() noexcept -> ice::gfx::v2::GfxObjectStorage&
    {
        return *_storage;
    }

    auto SimpleGfxRenderGraphRuntime::find_stage(
        ice::StringID_Arg identifier,
        ice::gfx::v2::GfxStage* stage
    ) noexcept -> ice::HashMap<ice::gfx::v2::GfxStage*>::ConstIterator
    {
        auto it = ice::multi_hashmap::find_first(_stages, ice::hash(identifier));
        while (it != nullptr)
        {
            if (it.value() == stage)
            {
                break;
            }
            it = ice::multi_hashmap::find_next(_stages, it);
        }
        return it;
    }

    void SimpleGfxRenderGraphRuntime::bind_stage(ice::StringID_Arg identifier, ice::gfx::v2::GfxStage* stage) noexcept
    {
        bool const stage_already_bound = find_stage(identifier, stage) != nullptr;
        ICE_ASSERT(stage_already_bound == false, "Trying to bind the same stages more than once!");
        if (stage_already_bound == false)
        {
            ice::multi_hashmap::insert(_stages, ice::hash(identifier), stage);
        }
    }

    void SimpleGfxRenderGraphRuntime::import_buffer(ice::StringID_Arg name, ice::render::Buffer buffer) noexcept
    {
        _storage->try_set(name, buffer);
    }

    void SimpleGfxRenderGraphRuntime::import_image(ice::StringID_Arg name, ice::render::Image image) noexcept
    {
        _storage->try_set(name, image);
    }

    void SimpleGfxRenderGraphRuntime::setup() noexcept
    {
        if (_swap_fence != nullptr) return;

        ice::gfx::v2::GfxStageState const graph_state{ };
        ice::gfx::v2::GfxStageParams const graph_params
        {
            .frame = *((ice::EngineFrame*)0),
            .device = _gfx_device,
            .state = graph_state,
            .command_buffer = ice::render::CommandBuffer::Invalid,
            .render_commands = _gfx_device.device().get_commands(),
        };

        for (ice::StringID_Arg stage_name : _render_graph.setup_stages())
        {
            auto it = ice::multi_hashmap::find_first(_stages, ice::hash(stage_name));
            while (it != nullptr)
            {
                it.value()->execute(graph_params, *_storage);
                it = ice::multi_hashmap::find_next(_stages, it);
            }
        }

        _swap_fence = _gfx_device.device().create_fence();
    }

    bool SimpleGfxRenderGraphRuntime::execute(
        ice::gfx::v2::GfxRenderParams const& params,
        ice::render::RenderFence& fence
    ) noexcept
    {
        ice::ucount const fb_idx = _next_framebuffer_index ^= 1;

        constexpr ice::StringID framebuffer_names[]{ "ice.gfx.framebuffer.0"_sid, "ice.gfx.framebuffer.1"_sid };

        ice::render::RenderDevice& device = _gfx_device.device();
        ice::render::Framebuffer framebuffer { };
        if (_storage->try_get(framebuffer_names[fb_idx], framebuffer) == false)
        {
            return false;
        }

        ice::render::Renderpass renderpass;
        if (_storage->try_get("ice.gfx.renderpass.default"_sid, renderpass) == false)
        {
            return false;
        }

        ice::gfx::v2::GfxStageState const graph_state{
            .renderpass = renderpass,
            .framebuffer = framebuffer,
            .framebuffer_size = _gfx_device.swapchain().extent()
        };

        ice::render::CommandBuffer command_buffer;
        ice::gfx::GfxQueue* queue;
        _gfx_device.queue_group(fb_idx).get_queue(ice::render::QueueFlags::Graphics, queue);
        queue->reset();
        queue->request_command_buffers(render::CommandBufferType::Primary, { &command_buffer, 1 });

        ice::gfx::v2::GfxStageParams const graph_params
        {
            .frame = params.frame,
            .device = _gfx_device,
            .state = graph_state,
            .command_buffer = command_buffer,
            .render_commands = device.get_commands(),
        };

        //graph_state.renderpass = r.renderpass;
        //graph_state.framebuffer = r.framebuffers[0];

        for (ice::StringID_Arg stage_name : _render_graph.runtime_stages())
        {
            auto it = ice::multi_hashmap::find_first(_stages, ice::hash(stage_name));
            while (it != nullptr)
            {
                it.value()->execute(graph_params, *_storage);
                it = ice::multi_hashmap::find_next(_stages, it);
            }
        }

        queue->submit_command_buffers({ &command_buffer, 1 }, &fence);
        fence.wait(10'000'000);
        _gfx_device.present(fb_idx);
        return true;
    }

    auto create_default_rendergraph_runtime(
        ice::Allocator& alloc,
        ice::gfx::v2::GfxRenderGraph const& render_graph,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept -> ice::UniquePtr<ice::gfx::v2::GfxRenderGraphRuntime>
    {
        return render_graph.create_runtime(alloc, gfx_device);
        //return ice::make_unique<ice::gfx::v2::SimpleGfxRenderGraphRuntime>(alloc, alloc, render_graph, gfx_device);
    }

} // namespace ice::gfx::v2
