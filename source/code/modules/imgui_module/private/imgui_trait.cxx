/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "imgui_trait.hxx"

#include <ice/engine.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_devui.hxx>
#include <ice/engine_shards.hxx>
#include <ice/world/world_manager.hxx>

#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_context.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>
#include <ice/gfx/gfx_render_graph_runtime.hxx>
#include <ice/gfx/gfx_object_storage.hxx>

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
            ice::render::ResourceSet resource_set;
            ice::u32 index_count;
            ice::u32 index_offset;
            ice::u32 vertex_offset;
            ImVec4 clip_rect;
        };

        auto load_imgui_shader(ice::AssetStorage& assets, ice::String name) noexcept -> ice::Task<ice::Data>
        {
            ice::Asset asset = assets.bind(ice::render::AssetType_Shader, name);
            co_return co_await asset[AssetState::Baked];
        }

    } // namespace detail

    ImGuiTrait::ImGuiTrait(ice::Allocator& alloc) noexcept
        : _imgui_timer{ .clock = nullptr }
        , _index_buffers{ alloc }
        , _vertex_buffers{ alloc }
    {
        ice::array::reserve(_index_buffers, 10);
        ice::array::reserve(_vertex_buffers, 10);

        _imgui_context = ImGui::CreateContext();
    }

    ImGuiTrait::~ImGuiTrait() noexcept
    {
        ImGui::DestroyContext();
    }

    auto ImGuiTrait::activate(ice::EngineWorldUpdate const& update) noexcept -> ice::Task<>
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
        io.KeyMap[ImGuiKey_KeyPadEnter] = 0;
        io.KeyMap[ImGuiKey_A] = (uint32_t)ice::input::KeyboardKey::KeyA;
        io.KeyMap[ImGuiKey_C] = (uint32_t)ice::input::KeyboardKey::KeyC;
        io.KeyMap[ImGuiKey_V] = (uint32_t)ice::input::KeyboardKey::KeyV;
        io.KeyMap[ImGuiKey_X] = (uint32_t)ice::input::KeyboardKey::KeyX;
        io.KeyMap[ImGuiKey_Y] = (uint32_t)ice::input::KeyboardKey::KeyY;
        io.KeyMap[ImGuiKey_Z] = (uint32_t)ice::input::KeyboardKey::KeyZ;

        _shader_data[0] = co_await detail::load_imgui_shader(update.assets, "shaders/debug/imgui-vert");
        _shader_data[1] = co_await detail::load_imgui_shader(update.assets, "shaders/debug/imgui-frag");
        co_return;
    }

    auto ImGuiTrait::deactivate(ice::EngineWorldUpdate const& world_update) noexcept -> ice::Task<>
    {
        co_return;
    }

    void ImGuiTrait::gather_tasks(ice::TraitTaskLauncher& task_launcher) noexcept
    {
        task_launcher.bind<&ImGuiTrait::update>();
        task_launcher.bind<&ImGuiTrait::gfx_start>(ice::gfx::v2::ShardID_GfxStartup);
        task_launcher.bind<&ImGuiTrait::gfx_shutdown>(ice::gfx::v2::ShardID_GfxShutdown);
        task_launcher.bind<&ImGuiTrait::gfx_update>(ice::gfx::v2::ShardID_GfxFrameUpdate);
        task_launcher.bind<&ImGuiTrait::on_window_resized>(ice::platform::ShardID_WindowResized);
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
        if (ice::shards::inspect_last(update.frame.shards(), ice::platform::Shard_InputText.id, input_text))
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

    auto ImGuiTrait::gfx_start(ice::gfx::v2::GfxStateChange const& params) noexcept -> ice::Task<>
    {
        using namespace ice::gfx;
        using namespace ice::render;

        GfxDevice& gfx = params.device;
        RenderDevice& device = params.device.device();

        ice::vec2u const extent = gfx.swapchain().extent();
        co_await on_window_resized(ice::vec2i(extent.x, extent.y));

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
        _shaders[0] = device.create_shader(ShaderInfo{ .shader_data = _shader_data[0] });
        _shaders[1] = device.create_shader(ShaderInfo{ .shader_data = _shader_data[1] });

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

        ResourceSetLayoutBinding const resource_bindings[]
        {
            ResourceSetLayoutBinding
            {
                .binding_index = 1,
                .resource_count = 1,
                .resource_type = ResourceType::Sampler,
                .shader_stage_flags = ShaderStageFlags::FragmentStage
            },
            ResourceSetLayoutBinding
            {
                .binding_index = 2,
                .resource_count = 1,
                .resource_type = ResourceType::SampledImage,
                .shader_stage_flags = ShaderStageFlags::FragmentStage
            },
        };

        _resource_layout = device.create_resourceset_layout({ resource_bindings + 0, 2 });

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
            .renderpass = params.renderpass,
            .shaders = _shaders,
            .shaders_stages = _shader_stages,
            .shader_bindings = bindings,
            .cull_mode = CullMode::Disabled,
            .front_face = FrontFace::CounterClockWise,
            .subpass_index = 1,
            .depth_test = true
        };

        _pipeline = device.create_pipeline(pipeline_info);

        ice::array::push_back(
            _index_buffers,
            device.create_buffer(BufferType::Index, 1024 * 1024 * 64)
        );
        ice::array::push_back(
            _vertex_buffers,
            device.create_buffer(BufferType::Vertex, 1024 * 1024 * 64)
        );

        //auto update_texture_task = [](GfxDevice& gfx_device, GfxFrame& gfx_frame, Image image, ImageInfo image_info) noexcept -> ice::Task<>
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
            ice::render::CommandBuffer const cmds = co_await params.frame_transfer;

            api.update_texture(
                cmds,
                _font_texture,
                data_buffer,
                { font_info.width, font_info.height }
            );

            co_await params.frame_end;

            device.destroy_buffer(data_buffer);
        }//;

        //gfx_frame.add_task(update_texture_task(gfx_device, gfx_frame, _font_texture, font_info));

        _initialized = true;

        co_return;
    }

    auto ImGuiTrait::gfx_shutdown(ice::gfx::v2::GfxStateChange const& params) noexcept -> ice::Task<>
    {
        _initialized = false;

        ice::render::RenderDevice& device = params.device.device();
        device.destroy_shader(_shaders[0]);
        device.destroy_shader(_shaders[1]);

        for (ice::render::Buffer buffer : _index_buffers)
        {
            device.destroy_buffer(buffer);
        }
        for (ice::render::Buffer buffer : _vertex_buffers)
        {
            device.destroy_buffer(buffer);
        }
        device.destroy_image(_font_texture);
        device.destroy_pipeline(_pipeline);
        device.destroy_pipeline_layout(_pipeline_layout);
        device.destroy_sampler(_sampler);
        device.destroy_shader(_shaders[1]);
        device.destroy_shader(_shaders[0]);
        device.destroy_resourcesets(_resources);
        device.destroy_resourceset_layout(_resource_layout);
        co_return;
    }

    auto ImGuiTrait::gfx_update(ice::gfx::v2::GfxFrameUpdate const& update) noexcept -> ice::Task<>
    {
        IPT_ZONE_SCOPED;

        if (_initialized == false)
        {
            co_return;
        }

        using namespace ice::render;
        using namespace ice::gfx::v2;

        ImDrawData* draw_data = ImGui::GetDrawData();
        if (draw_data == nullptr)
        {
            co_return;
        }

        RenderDevice& device = update.device.device();

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

            BufferUpdateInfo& idx_info = buffer_updates[buffer_update_count + 1];
            idx_info.buffer = _index_buffers[0];
            idx_info.offset = idx_buffer_offset * sizeof(ImDrawIdx);
            idx_info.data = ice::Data{ cmd_list->IdxBuffer.Data, ice::size_of<ImDrawIdx> *cmd_list->IdxBuffer.Size, ice::align_of<ImDrawIdx> };
            idx_buffer_offset += cmd_list->IdxBuffer.Size;

            buffer_update_count += 2;
            if (buffer_update_count == 10)
            {
                device.update_buffers({ buffer_updates, buffer_update_count });
                buffer_update_count = 0;
            }
        }

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

        update.stages.add_stage("copy"_sid, this);
    }

    auto ImGuiTrait::on_window_resized(ice::vec2i new_size) noexcept -> ice::Task<>
    {
        auto& io = ImGui::GetIO();
        _display_size = ice::vec2u(new_size.x, new_size.y);
        io.DisplaySize.x = (ice::f32)_display_size.x;
        io.DisplaySize.y = (ice::f32)_display_size.y;
        co_return;
    }

    void ImGuiTrait::draw(
        ice::EngineFrame const& frame,
        ice::render::CommandBuffer cmds,
        ice::render::RenderCommands& api
    ) const noexcept
    {
        IPT_ZONE_SCOPED_NAMED("DevUI - Gfx - ImGui command recording.");

        using namespace ice::render;

        ImGuiIO& _io = ImGui::GetIO();

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

        api.push_constant(cmds, _pipeline_layout, ShaderStageFlags::VertexStage, { scale, sizeof(scale) }, 0);
        api.push_constant(cmds, _pipeline_layout, ShaderStageFlags::VertexStage, { translate, sizeof(translate) }, sizeof(scale));
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

                ResourceSet new_set = _resources[0];
                if (cmd.resource_set != ResourceSet::Invalid)
                {
                    new_set = cmd.resource_set;
                }

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
                    cmd.resource_set = ResourceSet::Invalid;

                    if (pcmd->TextureId != nullptr && pcmd->TextureId != last_texid)
                    {
                        last_texid = pcmd->TextureId;
                        cmd.resource_set = _resources[next_resource_idx];
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
