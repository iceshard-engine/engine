/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "imgui_trait.hxx"

#include <ice/engine.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_devui.hxx>
#include <ice/engine_shards.hxx>
#include <ice/engine_frame.hxx>
#include <ice/world/world_updater.hxx>

#include <ice/gfx/gfx_shards.hxx>
#include <ice/gfx/gfx_object_storage.hxx>
#include <ice/gfx/gfx_stage_registry.hxx>

#include <ice/render/render_swapchain.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/render/render_pipeline.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_image.hxx>
#include <ice/render/render_buffer.hxx>
#include <ice/render/render_device.hxx>

#include <ice/input/input_keyboard.hxx>
#include <ice/input/input_mouse.hxx>
#include <ice/input/input_event.hxx>

#include <ice/asset.hxx>
#include <ice/task.hxx>
#include <ice/task_utils.hxx>
#include <ice/profiler.hxx>

namespace ice::devui
{

    namespace detail
    {

        struct DrawCommand
        {
            ice::u32 resource_set_idx;
            ice::u32 index_count;
            ice::u32 index_offset;
            ice::u32 vertex_offset;
            ImVec4 clip_rect;
        };

        auto load_imgui_shader(ice::AssetStorage& assets, ice::String name, ice::render::Shader& out_shader) noexcept -> ice::Task<>
        {
            ice::Asset asset = assets.bind(ice::render::AssetType_Shader, name);
            ice::Data shader_data = co_await asset[AssetState::Runtime];
            out_shader = *reinterpret_cast<ice::render::Shader const*>(shader_data.location);
            co_return;
        }

    } // namespace detail

    ImGuiGfxStage::ImGuiGfxStage(ice::Allocator& alloc, ice::AssetStorage& assets) noexcept
        : _assets{ assets }
        , _index_buffers{ alloc }
        , _vertex_buffers{ alloc }
    {
    }

    auto ImGuiGfxStage::initialize(
        ice::gfx::GfxContext& gfx,
        ice::gfx::GfxFrameStages& stages,
        ice::render::Renderpass renderpass
    ) noexcept -> ice::Task<>
    {
        using namespace ice::gfx;
        using namespace ice::render;

        auto stage_transfer = stages.frame_transfer;
        auto stage_end = stages.frame_end;

        RenderDevice& device = gfx.device();

        ImGuiIO& io = ImGui::GetIO();

        ice::u8* pixels;
        ice::i32 font_texture_width, font_texture_height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &font_texture_width, &font_texture_height);

        // ice::u32 upload_size = font_texture_width * font_texture_height * 4 * sizeof(char);
        ice::render::ImageInfo font_info;
        font_info.type = ImageType::Image2D;
        font_info.usage = ImageUsageFlags::Sampled | ImageUsageFlags::TransferDst;
        font_info.format = ImageFormat::UNORM_RGBA;
        font_info.width = font_texture_width;
        font_info.height = font_texture_height;
        font_info.data = pixels;

        _font_texture = device.create_image(font_info, { });

        _shader_stages[0] = ShaderStageFlags::VertexStage;
        _shader_stages[1] = ShaderStageFlags::FragmentStage;

        co_await detail::load_imgui_shader(_assets, "shaders/debug/imgui-vert", _shaders[0]);
        co_await detail::load_imgui_shader(_assets, "shaders/debug/imgui-frag", _shaders[1]);

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
        _resource_layout = device.create_resourceset_layout(resource_bindings);

        ResourceSetLayout resource_layouts[20]{ };
        for (auto& layout : resource_layouts)
        {
            layout = _resource_layout;
        }

        //_resource_layout[1] = device.create_resourceset_layout({ resource_bindings + 2, 2 });
        device.create_resourcesets(resource_layouts, _resources);

        ResourceUpdateInfo resource_update[]
        {
            ResourceUpdateInfo
            {
                .sampler = _sampler
            },
            ResourceUpdateInfo
            {
                .image = _font_texture,
            },
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
                .resource_type = ResourceType::Sampler,
                .binding_index = 1,
                .array_element = 0,
                .resources = { resource_update + 0, 1 }
            },
            ResourceSetUpdateInfo
            {
                .resource_set = _resources[0],
                .resource_type = ResourceType::SampledImage,
                .binding_index = 2,
                .array_element = 0,
                .resources = { resource_update + 1, 1 }
            },
            ResourceSetUpdateInfo
            {
                .resource_set = _resources[0],
                .resource_type = ResourceType::UniformBuffer,
                .binding_index = 3,
                .array_element = 0,
                .resources = { resource_update + 2, 1 }
            },
        };

        device.update_resourceset(update_infos);

        PipelinePushConstant const push_constants[]
        {
            PipelinePushConstant
            {
                .shader_stage_flags = ShaderStageFlags::VertexStage,
                .offset = 0,
                .size = sizeof(ice::vec2f) * 2,
            }
        };
        PipelineLayoutInfo const layout_info
        {
            .push_constants = push_constants,
            .resource_layouts = { &_resource_layout, 1 },
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

        PipelineInfo pipeline_info{
            .layout = _pipeline_layout,
            .renderpass = renderpass,
            .shaders = _shaders,
            .shaders_stages = _shader_stages,
            .shader_bindings = bindings,
            .cull_mode = CullMode::Disabled,
            .front_face = FrontFace::CounterClockWise,
            .subpass_index = 1,
            .depth_test = true
        };

        _pipeline = device.create_pipeline(pipeline_info);

        _index_buffer_host = _index_buffers._allocator->allocate<ice::u16>(1024 * 1024 * 32);
        ice::array::push_back(
            _index_buffers,
            device.create_buffer(BufferType::Index, 1024 * 1024 * 64)
        );
        ice::array::push_back(
            _vertex_buffers,
            device.create_buffer(BufferType::Vertex, 1024 * 1024 * 64)
        );

#if 1
        //auto update_texture_task = [](GfxContext& gfx_ctx, GfxFrame& gfx_frame, Image image, ImageInfo image_info) noexcept -> ice::Task<>
        {
            ice::u32 const image_data_size = font_info.width * font_info.height * 4;

            // Currently we start the task on the graphics thread, so we can dont race for access to the render device.
            //  It is planned to create an awaiter for graphics thread access so we can schedule this at any point from any thread.
            ice::render::Buffer const data_buffer = device.create_buffer(
                ice::render::BufferType::Transfer,
                image_data_size
            );

            ice::render::BufferUpdateInfo updates[]
            {
                ice::render::BufferUpdateInfo
                {
                    .buffer = data_buffer,
                    .data =
                    {
                        .location = font_info.data,
                        .size = { image_data_size },
                        .alignment = ice::ualign::b_4
                    }
                }
            };

            device.update_buffers(updates);

            ice::render::RenderCommands& api = device.get_commands();
            ice::render::CommandBuffer const cmds = co_await stage_transfer;

            api.update_texture(
                cmds,
                _font_texture,
                data_buffer,
                { font_info.width, font_info.height }
            );

            co_await stage_end;

            device.destroy_buffer(data_buffer);
        }
#endif

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
        ice::array::clear(_index_buffers);
        ice::array::clear(_vertex_buffers);

        device.destroy_buffer(_uniform_buffer);
        device.destroy_image(_font_texture);
        device.destroy_pipeline(_pipeline);
        device.destroy_pipeline_layout(_pipeline_layout);
        device.destroy_sampler(_sampler);
        // device.destroy_shader(_shaders[1]);
        // device.destroy_shader(_shaders[0]);
        device.destroy_resourcesets(_resources);
        device.destroy_resourceset_layout(_resource_layout);
        co_return;
    }

    void ImGuiGfxStage::update(ice::gfx::GfxContext& gfx) noexcept
    {
        using namespace ice::render;

        ImDrawData* draw_data = ImGui::GetDrawData();
        if (draw_data == nullptr)
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

            float L = draw_data->DisplayPos.x;
            float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
            float T = draw_data->DisplayPos.y;
            float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
            float mvp[4][4] =
            {
                { 2.0f/(R-L),   0.0f,           0.0f,       0.0f },
                { 0.0f,         2.0f/(T-B),     0.0f,       0.0f },
                { 0.0f,         0.0f,           0.5f,       0.0f },
                { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
            };

            ice::render::BufferUpdateInfo updates[]
            {
                ice::render::BufferUpdateInfo
                {
                    .buffer = _uniform_buffer,
                    .data =
                    {
                        .location = mvp,
                        .size = {sizeof(mvp)},
                        .alignment = ice::ualign::b_4
                    }
                },
            };

            device.update_buffers(updates);
        }

        ice::u32 buffer_update_count = 0;
        BufferUpdateInfo buffer_updates[10]{ };

        ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
        ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        ImTextureID last_texid = nullptr; // ImGui::GetIO().tex
        ice::u32 next_resource_idx = 1;
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

                if (pcmd->TextureId != nullptr && pcmd->TextureId != last_texid)
                {
                    last_texid = pcmd->TextureId;
                    resource_update[1].image = static_cast<Image>(reinterpret_cast<ice::uptr>(last_texid));
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

        void const* a;
        frame.data().frame().get("a"_sid, a);

        ice::u32 c = (ice::u32)(ice::uptr)a;

        void const* span_loc;
        frame.data().frame().get("ice.devui.imgui_draw_commands"_sid, span_loc);
        detail::DrawCommand const* cmds_span = reinterpret_cast<detail::DrawCommand const*>(span_loc);// frame.storage().named_object<ice::Span<detail::DrawCommand>>("ice.devui.imgui_draw_commands_span"_sid);
        if (cmds_span == nullptr)
        {
            return;
        }

        float scale[2];
        scale[0] = 2.0f / fb_width;
        scale[1] = 2.0f / fb_height;
        float translate[2];
        translate[0] = -1.0f; // -1.0f - width * scale[0];
        translate[1] = -1.0f; //-1.0f - height * scale[1];

        ResourceSet last_resource = _resources[0];

        api.push_constant(cmds, _pipeline_layout, ShaderStageFlags::VertexStage, { scale, { sizeof(scale) } }, 0);
        api.push_constant(cmds, _pipeline_layout, ShaderStageFlags::VertexStage, { translate, { sizeof(translate) } }, sizeof(scale));
        api.bind_pipeline(cmds, _pipeline);
        api.bind_resource_set(cmds, _pipeline_layout, _resources[0], 0);
        api.bind_vertex_buffer(cmds, _vertex_buffers[0], 0);
        api.bind_index_buffer(cmds, _index_buffers[0]);

        ice::vec4u viewport_rect{ 0, 0, (ice::u32)_io.DisplaySize.x, (ice::u32)_io.DisplaySize.y };
        api.set_viewport(cmds, viewport_rect);
        api.set_scissor(cmds, viewport_rect);

        for (detail::DrawCommand const& cmd : ice::Span<detail::DrawCommand const>{ cmds_span, c })
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
                    api.bind_resource_set(cmds, _pipeline_layout, new_set, 0);
                }

                // Draw
                api.draw_indexed(cmds, cmd.index_count, 1, cmd.index_offset /*+ idx_buffer_offset*/, cmd.vertex_offset /*+ vtx_buffer_offset*/, 0);
            }
        }

        delete[] cmds_span;
    }

    ImGuiTrait::ImGuiTrait(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _imgui_gfx_stage{ }
        , _imgui_timer{ .clock = nullptr }
    {
        _imgui_context = ImGui::CreateContext();
    }

    ImGuiTrait::~ImGuiTrait() noexcept
    {
        ImGui::DestroyContext();
    }

    void ImGuiTrait::gather_tasks(ice::TraitTaskRegistry& task_registry) noexcept
    {
        task_registry.bind<&ImGuiTrait::update>();
        task_registry.bind<&ImGuiTrait::gfx_start>(ice::gfx::ShardID_GfxStartup);
        task_registry.bind<&ImGuiTrait::gfx_shutdown>(ice::gfx::ShardID_GfxShutdown);
        task_registry.bind<&ImGuiTrait::gfx_update>(ice::gfx::ShardID_GfxFrameUpdate);
        task_registry.bind<&ImGuiTrait::on_window_resized>(ice::platform::ShardID_WindowResized);
    }

    auto ImGuiTrait::activate(ice::WorldStateParams const& update) noexcept -> ice::Task<>
    {
        _imgui_timer = ice::timer::create_timer(update.clock, 1.f / 60.f);

        auto& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        io.KeyMap[ImGuiKey_Tab] = (uint32_t)ice::input::KeyboardKey::Tab;
        io.KeyMap[ImGuiKey_LeftArrow] = (uint32_t)ice::input::KeyboardKey::Left;
        io.KeyMap[ImGuiKey_RightArrow] = (uint32_t)ice::input::KeyboardKey::Right;
        io.KeyMap[ImGuiKey_UpArrow] = (uint32_t)ice::input::KeyboardKey::Up;
        io.KeyMap[ImGuiKey_DownArrow] = (uint32_t)ice::input::KeyboardKey::Down;
        io.KeyMap[ImGuiKey_PageUp] = (uint32_t)ice::input::KeyboardKey::PageUp;
        io.KeyMap[ImGuiKey_PageDown] = (uint32_t)ice::input::KeyboardKey::PageDown;
        io.KeyMap[ImGuiKey_Home] = (uint32_t)ice::input::KeyboardKey::Home;
        io.KeyMap[ImGuiKey_End] = (uint32_t)ice::input::KeyboardKey::End;
        io.KeyMap[ImGuiKey_Insert] = (uint32_t)ice::input::KeyboardKey::Insert;
        io.KeyMap[ImGuiKey_Delete] = (uint32_t)ice::input::KeyboardKey::Delete;
        io.KeyMap[ImGuiKey_Backspace] = (uint32_t)ice::input::KeyboardKey::Backspace;
        io.KeyMap[ImGuiKey_Space] = (uint32_t)ice::input::KeyboardKey::Space;
        io.KeyMap[ImGuiKey_Enter] = (uint32_t)ice::input::KeyboardKey::Return;
        io.KeyMap[ImGuiKey_Escape] = (uint32_t)ice::input::KeyboardKey::Escape;
        io.KeyMap[ImGuiKey_KeypadEnter] = 0;
        io.KeyMap[ImGuiKey_A] = (uint32_t)ice::input::KeyboardKey::KeyA;
        io.KeyMap[ImGuiKey_C] = (uint32_t)ice::input::KeyboardKey::KeyC;
        io.KeyMap[ImGuiKey_V] = (uint32_t)ice::input::KeyboardKey::KeyV;
        io.KeyMap[ImGuiKey_X] = (uint32_t)ice::input::KeyboardKey::KeyX;
        io.KeyMap[ImGuiKey_Y] = (uint32_t)ice::input::KeyboardKey::KeyY;
        io.KeyMap[ImGuiKey_Z] = (uint32_t)ice::input::KeyboardKey::KeyZ;
        co_return;
    }

    auto ImGuiTrait::deactivate(ice::WorldStateParams const& world_update) noexcept -> ice::Task<>
    {
        co_return;
    }

    auto ImGuiTrait::update(ice::EngineFrameUpdate const& update) noexcept -> ice::Task<>
    {
        IPT_ZONE_SCOPED_NAMED("DevUI - ImGui input update");

        using ice::input::InputEvent;
        using ice::input::MouseInput;

        auto& io = ImGui::GetIO();

        ice::vec2i window_size{ };
        if (ice::shards::inspect_last(update.frame.shards(), ice::platform::ShardID_WindowResized, window_size))
        {
            _display_size = { (ice::u32)window_size.x, (ice::u32)window_size.y };
            io.DisplaySize.x = (ice::f32)_display_size.x;
            io.DisplaySize.y = (ice::f32)_display_size.y;
        }

        char const* input_text;
        if (ice::shards::inspect_last(update.frame.shards(), ice::platform::ShardID_InputText, input_text))
        {
            io.AddInputCharactersUTF8(input_text);
        }

        ice::shards::inspect_each<ice::input::InputEvent>(
            update.frame.shards(),
            ice::ShardID_InputEvent,
            [&](ice::input::InputEvent input) noexcept
            {
                if (ice::input::input_identifier_device(input.identifier) == ice::input::DeviceType::Mouse)
                {
                    MouseInput const input_source = MouseInput(ice::input::input_identifier_value(input.identifier));

                    switch (input_source)
                    {
                    case MouseInput::ButtonLeft:
                        io.MouseDown[0] = input.value.button.state.pressed;
                        break;
                    case MouseInput::ButtonRight:
                        io.MouseDown[1] = input.value.button.state.pressed;
                        break;
                    case MouseInput::ButtonMiddle:
                        io.MouseDown[2] = input.value.button.state.pressed;
                        break;
                    case MouseInput::PositionX:
                        io.MousePos.x = (ice::f32)input.value.axis.value_i32;
                        break;
                    case MouseInput::PositionY:
                        io.MousePos.y = (ice::f32)input.value.axis.value_i32;
                        break;
                    case MouseInput::Wheel:
                        io.MouseWheel = (ice::f32)input.value.axis.value_i32;
                        break;
                    }
                }
                if (ice::input::input_identifier_device(input.identifier) == ice::input::DeviceType::Keyboard)
                {
                    using namespace ice::input;
                    ice::u32 const input_source = ice::input::input_identifier_value(input.identifier);

                    io.KeysDown[input_source] = input.value.button.state.pressed;

                    InputID constexpr left_ctrl_key_mod = ice::input::input_identifier(DeviceType::Keyboard, KeyboardMod::CtrlLeft, mod_identifier_base_value);
                    InputID constexpr right_ctrl_key_mod = ice::input::input_identifier(DeviceType::Keyboard, KeyboardMod::CtrlRight, mod_identifier_base_value);
                    InputID constexpr left_shift_key_mod = ice::input::input_identifier(DeviceType::Keyboard, KeyboardMod::ShiftLeft, mod_identifier_base_value);
                    InputID constexpr right_shift_key_mod = ice::input::input_identifier(DeviceType::Keyboard, KeyboardMod::ShiftRight, mod_identifier_base_value);

                    if (input.identifier == left_ctrl_key_mod || input.identifier == right_ctrl_key_mod)
                    {
                        io.KeyCtrl = input.value.button.state.pressed;
                    }
                    if (input.identifier == left_shift_key_mod || input.identifier == right_shift_key_mod)
                    {
                        io.KeyShift = input.value.button.state.pressed;
                    }
                }

            }
        );
        co_return;
    }

    auto ImGuiTrait::gfx_start(ice::gfx::GfxStateChange const& params) noexcept -> ice::Task<>
    {
        _imgui_gfx_stage = ice::make_unique<ImGuiGfxStage>(_allocator, _allocator, params.assets);
        params.stages.register_stage("copy"_sid, _imgui_gfx_stage.get());

        ice::vec2u const size = params.context.swapchain().extent();
        co_await on_window_resized(ice::vec2i{ size });

        _initialized = true;
        co_return;
    }

    auto ImGuiTrait::gfx_shutdown(ice::gfx::GfxStateChange const& params) noexcept -> ice::Task<>
    {
        _initialized = false;
        co_return;
    }

    auto ImGuiTrait::gfx_update(ice::gfx::GfxFrameUpdate const& update) noexcept -> ice::Task<>
    {
        IPT_ZONE_SCOPED;

        if (_initialized == false)
        {
            co_return;
        }
    }

    auto ImGuiTrait::on_window_resized(ice::vec2i new_size) noexcept -> ice::Task<>
    {
        auto& io = ImGui::GetIO();
        _display_size = ice::vec2u(new_size.x, new_size.y);
        io.DisplaySize.x = (ice::f32)_display_size.x;
        io.DisplaySize.y = (ice::f32)_display_size.y;
        co_return;
    }

    bool ImGuiTrait::start_frame() noexcept
    {
        // TODO: Make a proper 'init' guard
        if (_imgui_timer.clock == nullptr)
        {
            return false;
        }

        if (ice::timer::update(_imgui_timer))
        {
            _next_frame = _initialized;
        }

        if (_next_frame)
        {
            ImGui::NewFrame();
        }
        return _next_frame;
    }

    void ImGuiTrait::end_frame(ice::EngineFrame& frame) noexcept
    {
        // TODO: Make a proper 'init' guard
        if (_imgui_timer.clock == nullptr)
        {
            return;
        }

        if (_next_frame)
        {
            _next_frame = false;
            ImGui::EndFrame();
            ImGui::Render();
        }

        ImDrawData* draw_data = ImGui::GetDrawData();
        if (draw_data != nullptr)
        {
            ice::u32 cmd_count = 0;
            for (ice::i32 i = 0; i < draw_data->CmdListsCount; i++)
            {
                ImDrawList const* cmd_list = draw_data->CmdLists[i];
                cmd_count += cmd_list->CmdBuffer.Size;
            }

            //ice::Memory data = frame.allocator().allocate(ice::meminfo_of<detail::DrawCommand> * cmd_count);
            frame.data().frame().set("a"_sid, (void*)(ice::uptr)cmd_count);
            frame.data().frame().set("ice.devui.imgui_draw_commands"_sid,  new detail::DrawCommand[cmd_count]);

            //ice::Span<detail::DrawCommand> cmds_span = frame.storage().create_named_span<detail::DrawCommand>("ice.devui.imgui_draw_commands"_sid, cmd_count);
            //frame.storage().create_named_object<ice::Span<detail::DrawCommand>>("ice.devui.imgui_draw_commands_span"_sid, cmds_span);

            build_internal_command_list(frame);
        }
    }

    auto ImGuiTrait::imgui_context() const noexcept -> ImGuiContext*
    {
        return _imgui_context;
    }

    void ImGuiTrait::build_internal_command_list(ice::EngineFrame& frame) noexcept
    {
        ImDrawData* draw_data = ImGui::GetDrawData();
        if (draw_data == nullptr)
        {
            return;
        }

        void* span_loc = nullptr;
        frame.data().frame().get("ice.devui.imgui_draw_commands"_sid, span_loc);

        //ice::Span<detail::DrawCommand> cmds_span = //frame.data().frame().named_object<ice::Span<detail::DrawCommand>>("ice.devui.imgui_draw_commands_span"_sid);
        detail::DrawCommand* cmds = reinterpret_cast<detail::DrawCommand*>(span_loc);
        if (cmds != nullptr)
        {
            using ice::render::Image;
            using ice::render::ResourceSet;

            IPT_ZONE_SCOPED_NAMED("DevUI - Gfx - Build command list.");

            ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
            ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

            ImTextureID last_texid = nullptr; // ImGui::GetIO().tex
            ice::u32 next_resource_idx = 1;

            // Upload vertex/index data into a single contiguous GPU buffer
            ice::u32 vtx_buffer_offset = 0;
            ice::u32 idx_buffer_offset = 0;

            ice::u32 command_idx = 0;
            for (int n = 0; n < draw_data->CmdListsCount; n++)
            {
                ImDrawList const* cmd_list = draw_data->CmdLists[n];

                for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
                {
                    ImDrawCmd const* pcmd = &cmd_list->CmdBuffer[cmd_i];
                    detail::DrawCommand& cmd = cmds[command_idx];
                    cmd.resource_set_idx = 0;

                    if (pcmd->TextureId != nullptr && pcmd->TextureId != last_texid)
                    {
                        last_texid = pcmd->TextureId;
                        cmd.resource_set_idx = next_resource_idx;
                        next_resource_idx += 1;
                    }

                    ImVec4 clip_rect;
                    clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                    clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                    clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                    clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                    cmd.index_count = pcmd->ElemCount;
                    cmd.index_offset = pcmd->IdxOffset + idx_buffer_offset;
                    cmd.vertex_offset = pcmd->VtxOffset + vtx_buffer_offset;
                    cmd.clip_rect = clip_rect;

                    command_idx += 1;
                }

                vtx_buffer_offset += cmd_list->VtxBuffer.Size;

                idx_buffer_offset += cmd_list->IdxBuffer.Size;
            }

            ICE_ASSERT(
                vtx_buffer_offset * sizeof(ImDrawVert) < (1024 * 1024 * 4),
                "ImGui vertex buffer to big"
            );
            ICE_ASSERT(
                idx_buffer_offset * sizeof(ImDrawIdx) < (1024 * 1024 * 4),
                "ImGui index buffer to big"
            );
        }
    }

} // namespace ice::devui
