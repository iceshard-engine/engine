/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_gfx_subpass.hxx"

#include <ice/render/render_device.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_buffer.hxx>

namespace ice::gfx
{

    void create_subpass_resources_primitives(
        ice::render::RenderDevice& render_device,
        ice::gfx::GfxResourceTracker& resource_tracker
    ) noexcept
    {
        using namespace ice::render;

        ResourceSetLayoutBinding resourceset_bindings[]{
            ResourceSetLayoutBinding{
                .binding_index = GfxSubpass_Primitives::ResConst_CameraUniformBinding,
                .resource_count = 1,
                .resource_type = ResourceType::UniformBuffer,
                .shader_stage_flags = ShaderStageFlags::VertexStage
            }
        };

        ResourceSetLayout resourceset_layout = render_device.create_resourceset_layout(
            resourceset_bindings
        );

        PipelineLayout pipeline_layout = render_device.create_pipeline_layout(
            PipelineLayoutInfo{
                .push_constants = { },
                .resource_layouts { &resourceset_layout, 1 }
            }
        );

        track_resource(
            resource_tracker,
            GfxSubpass_Primitives::ResName_ResourceLayout,
            resourceset_layout
        );

        track_resource(
            resource_tracker,
            GfxSubpass_Primitives::ResName_PipelineLayout,
            pipeline_layout
        );
    }

    void destroy_subpass_resources_primitives(
        ice::render::RenderDevice& render_device,
        ice::gfx::GfxResourceTracker& resource_tracker
    ) noexcept
    {
        using namespace ice::render;

        render_device.destroy_pipeline_layout(
            find_resource<PipelineLayout>(resource_tracker, GfxSubpass_Primitives::ResName_PipelineLayout)
        );
        render_device.destroy_resourceset_layout(
            find_resource<ResourceSetLayout>(resource_tracker, GfxSubpass_Primitives::ResName_ResourceLayout)
        );
    }

} // namespace ice::gfx
