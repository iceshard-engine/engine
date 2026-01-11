/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "imgui_gfx_stage.hxx"
#include <ice/gfx/gfx_runner.hxx>
#include <ice/gfx/gfx_context.hxx>
#include <ice/gfx/gfx_stage_registry.hxx>
#include <ice/gfx/gfx_utils.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/render/render_pipeline.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_image.hxx>
#include <ice/render/render_buffer.hxx>
#include <ice/render/render_device.hxx>
#include <ice/render/render_profiler.hxx>
#include <ice/math/projection.hxx>
#include <ice/config.hxx>

namespace ice::devui
{

    ImGuiGfxStage::ImGuiGfxStage(ice::Allocator& alloc, ice::AssetStorage& assets) noexcept
        : draw_commands{ alloc }
        , _assets{ assets }
        , _index_buffers{ alloc }
        , _vertex_buffers{ alloc }
    {
    }

    auto ImGuiGfxStage::initialize(
        ice::gfx::GfxContext& gfx,
        ice::gfx::GfxFrameStages& stages,
        ice::render::Renderpass renderpass,
        ice::u32 subpass
    ) noexcept -> ice::Task<>
    {
        using namespace ice::gfx;
        using namespace ice::render;

        ice::TaskScheduler scheduler = stages.scheduler;

        RenderDevice& device = gfx.device();

        ice::Expected r_vert = co_await ice::gfx::load_shader_program("shaders/debug/imgui-vert", _assets);
        ice::Expected r_frag = co_await ice::gfx::load_shader_program("shaders/debug/imgui-frag", _assets);
        ICE_ASSERT_CORE(r_vert && r_frag);

        co_await scheduler;

        SamplerInfo sampler_info
        {
            .min_filter = SamplerFilter::Nearest,
            .mag_filter = SamplerFilter::Nearest,
            .address_mode =
            {
                .u = SamplerAddressMode::ClampToEdge,
                .v = SamplerAddressMode::ClampToEdge,
                .w = SamplerAddressMode::ClampToEdge,
            },
            .mip_map_mode = SamplerMipMapMode::Nearest,
        };

        _sampler = device.create_sampler(sampler_info);

        ResourceSetLayoutBindingDetails const resource_details[]{
            ResourceSetLayoutBindingDetails{},
            ResourceSetLayoutBindingDetails{.image = {.type = ImageType::Image2D}}
        };

        ResourceSetLayoutBinding const resource_bindings[]
        {
            ResourceSetLayoutBinding
            {
                .binding_index = 1,
                .resource_count = 1,
                .resource_type = ResourceType::Sampler,
                .shader_stage_flags = ShaderStageFlags::FragmentStage,
                .binding_details = resource_details + 0
            },
            ResourceSetLayoutBinding
            {
                .binding_index = 2,
                .resource_count = 1,
                .resource_type = ResourceType::SampledImage,
                .shader_stage_flags = ShaderStageFlags::FragmentStage,
                .binding_details = resource_details + 1
            },
            ResourceSetLayoutBinding
            {
                .binding_index = 3,
                .resource_count = 1,
                .resource_type = ResourceType::UniformBuffer,
                .shader_stage_flags = ShaderStageFlags::VertexStage,
                .binding_details = resource_details + 0
            },
        };

        _uniform_buffer = device.create_buffer(BufferType::Uniform, 64);
        _resource_layout[0] = device.create_resourceset_layout({ resource_bindings + 2, 1 });
        _resource_layout[1] = device.create_resourceset_layout({ resource_bindings + 0, 2 });


        ResourceSetLayout resource_layouts[20]{ };
        for (auto& layout : resource_layouts)
        {
            layout = _resource_layout[1];
        }
        resource_layouts[0] = _resource_layout[0];

        device.create_resourcesets({ resource_layouts + 0, 1 }, { _resources + 0, 1 });
        device.create_resourcesets({ resource_layouts + 1, 19 }, { _resources + 1, 19 });

        ResourceUpdateInfo resource_update[]
        {
            ResourceUpdateInfo
            {
                .uniform_buffer = {.buffer = _uniform_buffer, .offset = 0, .size = 64},
            },
        };

        ResourceSetUpdateInfo update_infos[]
        {
            ResourceSetUpdateInfo
            {
                .resource_set = _resources[0],
                .resource_type = ResourceType::UniformBuffer,
                .binding_index = 3,
                .array_element = 0,
                .resources = { resource_update, 1 }
            },
        };

        device.update_resourceset(update_infos);

        PipelineLayoutInfo const layout_info
        {
            .resource_layouts = _resource_layout,
        };

        _pipeline_layout = device.create_pipeline_layout(layout_info);

        ShaderInputAttribute attribs[]
        {
            ShaderInputAttribute
            {
                .location = 0,
                .offset = 0,
                .type = ShaderAttribType::Vec2f
            },
            ShaderInputAttribute
            {
                .location = 1,
                .offset = 8,
                .type = ShaderAttribType::Vec2f
            },
            ShaderInputAttribute
            {
                .location = 2,
                .offset = 16,
                .type = ShaderAttribType::Vec4f_Unorm8
            },
        };

        ShaderInputBinding bindings[]
        {
            ShaderInputBinding
            {
                .binding = 0,
                .stride = sizeof(ImDrawVert),
                .instanced = false,
                .attributes = { attribs + 0, 3 }
            },
        };

        PipelineProgramInfo shaders[]{ r_vert, r_frag };
        PipelineInfo pipeline_info{
            .layout = _pipeline_layout,
            .renderpass = renderpass,
            .shaders = shaders,
            .vertex_bindings = bindings,
            .cull_mode = CullMode::Disabled,
            .front_face = FrontFace::CounterClockWise,
            .subpass_index = subpass,
            .depth_test = true
        };

        _pipeline = device.create_pipeline(pipeline_info);

        _index_buffer_host = _index_buffers._allocator->allocate<ice::u16>(1024 * 1024 * 32);
        _index_buffers.push_back(device.create_buffer(BufferType::Index, 1024 * 1024 * 64));
        _vertex_buffers.push_back(device.create_buffer(BufferType::Vertex, 1024 * 1024 * 64));

        co_return;
    }

    auto ImGuiGfxStage::cleanup(
        ice::gfx::GfxContext& gfx
    ) noexcept -> ice::Task<>
    {
        ice::render::RenderDevice& device = gfx.device();

        _index_buffers._allocator->deallocate(_index_buffer_host);
        for (ice::render::Buffer buffer : _index_buffers)
        {
            device.destroy_buffer(buffer);
        }
        for (ice::render::Buffer buffer : _vertex_buffers)
        {
            device.destroy_buffer(buffer);
        }
        _index_buffers.clear();
        _vertex_buffers.clear();

        device.destroy_buffer(_uniform_buffer);
        device.destroy_pipeline(_pipeline);
        device.destroy_pipeline_layout(_pipeline_layout);
        device.destroy_sampler(_sampler);
        device.destroy_resourcesets(_resources);
        device.destroy_resourceset_layout(_resource_layout[0]);
        device.destroy_resourceset_layout(_resource_layout[1]);
        co_return;
    }

    void ImGuiGfxStage::update(
        ice::EngineFrame const& frame,
        ice::gfx::GfxContext& gfx
    ) noexcept
    {
        using namespace ice::render;

        ImDrawData* draw_data = ImGui::GetDrawData();
        if (draw_data == nullptr || draw_commands.is_empty())
        {
            return;
        }

        RenderDevice& device = gfx.device();

        {
            ImGuiIO& _io = ImGui::GetIO();

            // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
            int fb_width = static_cast<int>(_io.DisplaySize.x * _io.DisplayFramebufferScale.x);
            int fb_height = static_cast<int>(_io.DisplaySize.y * _io.DisplayFramebufferScale.y);
            if (fb_width == 0 || fb_height == 0)
            {
                return;
            }

            ice::mat4 const mvp = ice::math::orthographic(
                vec2f{ draw_data->DisplayPos.x, draw_data->DisplayPos.x + draw_data->DisplaySize.x },
                vec2f{ draw_data->DisplayPos.y, draw_data->DisplaySize.y },
                vec2f{ 0, 1 }
            );

            ice::render::BufferUpdateInfo const updates[] {
                ice::render::BufferUpdateInfo { .buffer = _uniform_buffer, .data = ice::data_view(mvp) },
            };
            device.update_buffers(updates);
        }

        ice::u32 buffer_update_count = 0;
        BufferUpdateInfo buffer_updates[10]{ };

        ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
        ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        ImTextureID last_texid = ImTextureID_Invalid; // ImGui::GetIO().tex
        ice::u32 next_resource_idx = 2;
        ResourceUpdateInfo resource_update[]
        {
            ResourceUpdateInfo
            {
                .sampler = _sampler
            },
            ResourceUpdateInfo
            {
                .image = Image::Invalid,
            },
        };

        ResourceSetUpdateInfo resource_set_update[]
        {
            ResourceSetUpdateInfo
            {
                .resource_type = ResourceType::Sampler,
                .binding_index = 1,
                .array_element = 0,
                .resources = { resource_update + 0, 1 }
            },
            ResourceSetUpdateInfo
            {
                .resource_type = ResourceType::SampledImage,
                .binding_index = 2,
                .array_element = 0,
                .resources = { resource_update + 1, 1 }
            },
        };

        // Upload vertex/index data into a single contiguous GPU buffer
        ice::u32 vtx_buffer_offset = 0;
        ice::u32 idx_buffer_offset = 0;

        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            ImDrawList const* cmd_list = draw_data->CmdLists[n];

            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
            {
                ImDrawCmd const* pcmd = &cmd_list->CmdBuffer[cmd_i];
                if (pcmd->GetTexID() != ImTextureID_Invalid && pcmd->GetTexID() != last_texid)
                {
                    last_texid = pcmd->GetTexID();
                    resource_update[1].image = static_cast<Image>(last_texid);
                    resource_set_update[0].resource_set = _resources[next_resource_idx];
                    resource_set_update[1].resource_set = _resources[next_resource_idx];

                    device.update_resourceset(resource_set_update);
                    next_resource_idx += 1;
                }

                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;
            }

            BufferUpdateInfo& vtx_info = buffer_updates[buffer_update_count];
            vtx_info.buffer = _vertex_buffers[0];
            vtx_info.offset = vtx_buffer_offset * sizeof(ImDrawVert);
            vtx_info.data = ice::Data{ cmd_list->VtxBuffer.Data, ice::size_of<ImDrawVert> *cmd_list->VtxBuffer.Size, ice::align_of<ImDrawVert> };
            vtx_buffer_offset += cmd_list->VtxBuffer.Size;

            // BufferUpdateInfo& idx_info = buffer_updates[buffer_update_count + 1];
            // idx_info.buffer = _index_buffers[0];
            // idx_info.offset = idx_buffer_offset * sizeof(ImDrawIdx);
            // idx_info.data = ice::Data{ cmd_list->IdxBuffer.Data, ice::size_of<ImDrawIdx> *cmd_list->IdxBuffer.Size, ice::align_of<ImDrawIdx> };
            ice::memcpy(_index_buffer_host + idx_buffer_offset, cmd_list->IdxBuffer.Data, sizeof(ImDrawIdx) * cmd_list->IdxBuffer.Size);
            idx_buffer_offset += cmd_list->IdxBuffer.Size;

            buffer_update_count += 1;
            if (buffer_update_count == 10)
            {
                device.update_buffers({ buffer_updates, buffer_update_count });
                buffer_update_count = 0;
            }
        }

        if (idx_buffer_offset % 4 != 0)
        {
            idx_buffer_offset += (4 - (idx_buffer_offset % 4));
        }

        BufferUpdateInfo& idx_info = buffer_updates[buffer_update_count];
        idx_info.buffer = _index_buffers[0];
        idx_info.offset = 0;
        idx_info.data = { _index_buffer_host, { idx_buffer_offset * ice::u32(sizeof(ice::u16)) }, ice::ualign::b_4 };
        buffer_update_count += 1;

        ICE_ASSERT(
            vtx_buffer_offset * sizeof(ImDrawVert) < (1024 * 1024 * 4),
            "ImGui vertex buffer to big"
        );
        ICE_ASSERT(
            idx_buffer_offset * sizeof(ImDrawIdx) < (1024 * 1024 * 4),
            "ImGui index buffer to big"
        );

        if (buffer_update_count > 0)
        {
            device.update_buffers({ buffer_updates, buffer_update_count });
        }
    }

    void ImGuiGfxStage::draw(
        ice::EngineFrame const& frame,
        ice::render::CommandBuffer cmds,
        ice::render::RenderCommands& api
    ) const noexcept
    {
        IPT_ZONE_SCOPED_NAMED("DevUI - Gfx - ImGui command recording.");

        using namespace ice::render;

        ImGuiIO& _io = ImGui::GetIO();

        IPR_ZONE( api, cmds, "ImGUI" );

        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        int fb_width = static_cast<int>(_io.DisplaySize.x * _io.DisplayFramebufferScale.x);
        int fb_height = static_cast<int>(_io.DisplaySize.y * _io.DisplayFramebufferScale.y);
        if (fb_width == 0 || fb_height == 0)
        {
            return;
        }

        float scale[2];
        scale[0] = 2.0f / fb_width;
        scale[1] = 2.0f / fb_height;
        float translate[2];
        translate[0] = -1.0f; // -1.0f - width * scale[0];
        translate[1] = -1.0f; //-1.0f - height * scale[1];

        ResourceSet last_resource = _resources[1];

        api.bind_pipeline(cmds, _pipeline);
        api.bind_resource_set(cmds, _pipeline_layout, _resources[0], 0);
        api.bind_resource_set(cmds, _pipeline_layout, _resources[1], 1);
        api.bind_vertex_buffer(cmds, _vertex_buffers[0], 0);
        api.bind_index_buffer(cmds, _index_buffers[0]);

        ice::vec4u viewport_rect{ 0, 0, (ice::u32)_io.DisplaySize.x, (ice::u32)_io.DisplaySize.y };
        api.set_viewport(cmds, viewport_rect);
        api.set_scissor(cmds, viewport_rect);

        for (DrawCommand const& cmd : draw_commands)
        {
            ImVec4 clip_rect = cmd.clip_rect;
            if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
            {
                // Negative offsets are illegal for vkCmdSetScissor
                if (clip_rect.x < 0.0f)
                    clip_rect.x = 0.0f;
                if (clip_rect.y < 0.0f)
                    clip_rect.y = 0.0f;

                // Apply scissor/clipping rectangle
                viewport_rect.x = (int32_t)(clip_rect.x);
                viewport_rect.y = (int32_t)(clip_rect.y);
                viewport_rect.z = (uint32_t)(clip_rect.z - clip_rect.x);
                viewport_rect.w = (uint32_t)(clip_rect.w - clip_rect.y);
                api.set_scissor(cmds, viewport_rect);

                ResourceSet new_set = _resources[cmd.resource_set_idx];
                if (new_set != last_resource)
                {
                    last_resource = new_set;
                    api.bind_resource_set(cmds, _pipeline_layout, new_set, 1);
                }

                // Draw
                api.draw_indexed(cmds, cmd.index_count, 1, cmd.index_offset /*+ idx_buffer_offset*/, cmd.vertex_offset /*+ vtx_buffer_offset*/, 0);
            }
        }
    }

} // namespace ice::devui
