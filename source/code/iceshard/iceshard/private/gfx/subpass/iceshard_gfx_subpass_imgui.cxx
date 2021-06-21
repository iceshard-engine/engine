#include "iceshard_gfx_subpass.hxx"

#include <ice/render/render_device.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_buffer.hxx>

namespace ice::gfx
{

    void create_subpass_resources_imgui(
        ice::render::RenderDevice& render_device,
        ice::gfx::GfxResourceTracker& resource_tracker
    ) noexcept
    {
        using namespace ice::render;

        ResourceSetLayoutBinding resourceset_bindings[]{
            ResourceSetLayoutBinding{
                .binding_index = GfxSubpass_ImGui::ResConst_FontTextureBinding,
                .resource_count = 1,
                .resource_type = ResourceType::SampledImage,
                .shader_stage_flags = ShaderStageFlags::FragmentStage
            },
            ResourceSetLayoutBinding{
                .binding_index = GfxSubpass_ImGui::ResConst_FontSamplerBinding,
                .resource_count = 1,
                .resource_type = ResourceType::Sampler,
                .shader_stage_flags = ShaderStageFlags::FragmentStage
            }
        };

        ResourceSetLayout resourceset_layout = render_device.create_resourceset_layout(
            resourceset_bindings
        );

        PipelinePushConstant push_constants[]{
            PipelinePushConstant{ .shader_stage_flags = ShaderStageFlags::VertexStage, .offset = 0, .size = 16 },
            //PipelinePushConstant{ .shader_stage_flags = ShaderStageFlags::VertexStage, .offset = 8, .size = 8 },
        };

        PipelineLayout pipeline_layout = render_device.create_pipeline_layout(
            PipelineLayoutInfo{
                .push_constants = push_constants,
                .resource_layouts { &resourceset_layout, 1 }
            }
        );

        track_resource(
            resource_tracker,
            GfxSubpass_ImGui::ResName_ResourceLayout,
            resourceset_layout
        );

        track_resource(
            resource_tracker,
            GfxSubpass_ImGui::ResName_PipelineLayout,
            pipeline_layout
        );
    }

    void destroy_subpass_resources_imgui(
        ice::render::RenderDevice& render_device,
        ice::gfx::GfxResourceTracker& resource_tracker
    ) noexcept
    {
        using namespace ice::render;

        render_device.destroy_pipeline_layout(
            find_resource<PipelineLayout>(resource_tracker, GfxSubpass_ImGui::ResName_PipelineLayout)
        );
        render_device.destroy_resourceset_layout(
            find_resource<ResourceSetLayout>(resource_tracker, GfxSubpass_ImGui::ResName_ResourceLayout)
        );
    }

} // namespace ice::gfx
