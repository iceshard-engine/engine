#include "imgui_trait.hxx"

#include <ice/engine.hxx>
#include <ice/engine_runner.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_task.hxx>

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

        auto load_imgui_shader(ice::AssetSystem& assets, ice::StringID name) noexcept -> ice::Data
        {
            Data result;
            Asset const shader_asset = assets.request(ice::AssetType::Shader, name);
            if (shader_asset != Asset::Invalid)
            {
                Data temp;
                if (ice::asset_data(shader_asset, temp) == AssetStatus::Loaded)
                {
                    result = *reinterpret_cast<ice::Data const*>(temp.location);
                }
            }

            return result;
        }

    } // namespace detail

    ImGuiTrait::ImGuiTrait(ice::Allocator& alloc) noexcept
        : _vertex_buffers{ alloc }
        , _index_buffers{ alloc }
        , _imgui_timer{ .clock = nullptr }
    {
        ice::pod::array::reserve(_index_buffers, 10);
        ice::pod::array::reserve(_vertex_buffers, 10);

        _imgui_context = ImGui::CreateContext();
    }

    ImGuiTrait::~ImGuiTrait() noexcept
    {
        ImGui::DestroyContext();
    }

    auto ImGuiTrait::gfx_stage_info() const noexcept -> ice::gfx::GfxStageInfo
    {
        static ice::gfx::GfxStageInfo const stage_info{
            .name = "ice.devui.imgui-render"_sid,
            .dependencies = { },
            .type = ice::gfx::GfxStageType::DrawStage
        };
        return stage_info;
    }

    auto ImGuiTrait::gfx_stage_slot() const noexcept -> ice::gfx::GfxStageSlot
    {
        static ice::gfx::GfxStageSlot const stage_slot {
            .name = "ice.devui.imgui-render"_sid,
            .stage = this,
        };
        return stage_slot;
    }

    void ImGuiTrait::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        _imgui_timer = ice::timer::create_timer(runner.clock(), 1.f / 60.f);
        _display_size = runner.graphics_device().swapchain().extent();

        auto& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.DisplaySize.x = _display_size.x;
        io.DisplaySize.y = _display_size.y;

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

        runner.execute_task(
            task_create_render_objects(runner, engine.asset_system(), runner.graphics_device()),
            EngineContext::EngineRunner
        );
    }

    void ImGuiTrait::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        runner.execute_task(
            task_destroy_render_objects(runner.graphics_device()),
            EngineContext::GraphicsFrame
        );
    }

    void ImGuiTrait::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        IPT_ZONE_SCOPED_NAMED("DevUI - ImGui input update");

        using ice::input::InputEvent;
        using ice::input::MouseInput;

        auto& io = ImGui::GetIO();
        for (ice::platform::Event const& event : runner.platform_events())
        {
            if (event.type == ice::platform::EventType::InputText)
            {
                io.AddInputCharactersUTF8(event.data.input.text.data());
            }
        }

        for (InputEvent const& event : frame.input_events())
        {
            if (ice::input::input_identifier_device(event.identifier) == ice::input::DeviceType::Mouse)
            {
                MouseInput const input_source = MouseInput(ice::input::input_identifier_value(event.identifier));

                switch (input_source)
                {
                case MouseInput::ButtonLeft:
                    io.MouseDown[0] = event.value.button.state.pressed;
                    break;
                case MouseInput::ButtonRight:
                    io.MouseDown[1] = event.value.button.state.pressed;
                    break;
                case MouseInput::ButtonMiddle:
                    io.MouseDown[2] = event.value.button.state.pressed;
                    break;
                case MouseInput::PositionX:
                    io.MousePos.x = event.value.axis.value_i32;
                    break;
                case MouseInput::PositionY:
                    io.MousePos.y = event.value.axis.value_i32;
                    break;
                case MouseInput::Wheel:
                    io.MouseWheel = event.value.axis.value_i32;
                    break;
                }
            }
            if (ice::input::input_identifier_device(event.identifier) == ice::input::DeviceType::Keyboard)
            {
                using namespace ice::input;
                ice::u32 const input_source = ice::input::input_identifier_value(event.identifier);

                io.KeysDown[input_source] = event.value.button.state.pressed;

                InputID constexpr left_ctrl_key_mod = ice::input::input_identifier(DeviceType::Keyboard, KeyboardMod::CtrlLeft, mod_identifier_base_value);
                InputID constexpr right_ctrl_key_mod = ice::input::input_identifier(DeviceType::Keyboard, KeyboardMod::CtrlRight, mod_identifier_base_value);
                InputID constexpr left_shift_key_mod = ice::input::input_identifier(DeviceType::Keyboard, KeyboardMod::ShiftLeft, mod_identifier_base_value);
                InputID constexpr right_shift_key_mod = ice::input::input_identifier(DeviceType::Keyboard, KeyboardMod::ShiftRight, mod_identifier_base_value);

                if (event.identifier == left_ctrl_key_mod || event.identifier == right_ctrl_key_mod)
                {
                    io.KeyCtrl = event.value.button.state.pressed;
                }
                if (event.identifier == left_shift_key_mod || event.identifier == right_shift_key_mod)
                {
                    io.KeyShift = event.value.button.state.pressed;
                }
            }

        }


        runner.graphics_frame().set_stage_slot(gfx_stage_slot());


        runner.execute_task(task_update_buffers(frame, runner.graphics_device()), EngineContext::GraphicsFrame);
    }

    void ImGuiTrait::record_commands(
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

        //ImDrawData* draw_data = ImGui::GetDrawData();
        //if (draw_data == nullptr)
        //{
        //    return;
        //}

        ice::Span<detail::DrawCommand> const* cmds_span = frame.named_object<ice::Span<detail::DrawCommand>>("ice.devui.imgui_draw_commands_span"_sid);
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

        //ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
        //ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        //uint32_t vtx_buffer_offset = 0;
        //uint32_t idx_buffer_offset = 0;
        //for (int32_t i = 0; i < draw_data->CmdListsCount; i++)
        for (detail::DrawCommand const& cmd : *cmds_span)
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

            //ImDrawList const* cmd_list = draw_data->CmdLists[i];
            //for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
            //{
            //    ImDrawCmd const* pcmd = &cmd_list->CmdBuffer[cmd_i];
            //    if (pcmd->UserCallback)
            //    {
            //        pcmd->UserCallback(cmd_list, pcmd);
            //    }
            //    else
            //    {
            //        ImVec4 clip_rect;
            //        clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
            //        clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
            //        clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
            //        clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;



            //        if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
            //        {
            //            // Negative offsets are illegal for vkCmdSetScissor
            //            if (clip_rect.x < 0.0f)
            //                clip_rect.x = 0.0f;
            //            if (clip_rect.y < 0.0f)
            //                clip_rect.y = 0.0f;

            //            // Apply scissor/clipping rectangle
            //            viewport_rect.x = (int32_t)(clip_rect.x);
            //            viewport_rect.y = (int32_t)(clip_rect.y);
            //            viewport_rect.z = (uint32_t)(clip_rect.z - clip_rect.x);
            //            viewport_rect.w = (uint32_t)(clip_rect.w - clip_rect.y);
            //            api.set_scissor(cmds, viewport_rect);

            //            // Draw
            //            api.draw_indexed(cmds, pcmd->ElemCount, 1, pcmd->IdxOffset + idx_buffer_offset, pcmd->VtxOffset + vtx_buffer_offset, 0);
            //        }
            //    }
            //}

            //vtx_buffer_offset += cmd_list->VtxBuffer.Size;
            //idx_buffer_offset += cmd_list->IdxBuffer.Size;
        }

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

    void ImGuiTrait::end_frame(
        ice::EngineFrame& frame
    ) noexcept
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

            ice::Span<detail::DrawCommand> cmds_span = frame.create_named_span<detail::DrawCommand>("ice.devui.imgui_draw_commands"_sid, cmd_count);
            frame.create_named_object<ice::Span<detail::DrawCommand>>("ice.devui.imgui_draw_commands_span"_sid, cmds_span);
        }
    }

    auto ImGuiTrait::imgui_context() const noexcept -> ImGuiContext*
    {
        return _imgui_context;
    }

    auto ImGuiTrait::task_create_render_objects(
        ice::EngineRunner& runner,
        ice::AssetSystem& asset_system,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept -> ice::Task<>
    {
        using namespace ice::gfx;
        using namespace ice::render;

        Data vtx_shader_data = ice::devui::detail::load_imgui_shader(asset_system, "/shaders/debug/imgui-vert"_sid);
        Data pix_shader_data = ice::devui::detail::load_imgui_shader(asset_system, "/shaders/debug/imgui-frag"_sid);

        ice::render::Renderpass renderpass = Renderpass::Invalid;
        while (renderpass == Renderpass::Invalid)
        {
            ice::EngineFrame& frame = co_await runner.schedule_next_frame();

            co_await frame.schedule_frame_end();

            ice::render::Renderpass* candidate_renderpass = frame.named_object<ice::render::Renderpass>("ice.gfx.renderpass"_sid);
            if (candidate_renderpass != nullptr)
            {
                renderpass = *candidate_renderpass;
            }
        }

        ImGuiIO& io = ImGui::GetIO();

        ice::u8* pixels;
        ice::i32 font_texture_width, font_texture_height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &font_texture_width, &font_texture_height);

        co_await runner.graphics_frame().frame_start();

        RenderDevice& device = gfx_device.device();

        ice::u32 upload_size = font_texture_width * font_texture_height * 4 * sizeof(char);
        ice::render::ImageInfo font_info;
        font_info.type = ImageType::Image2D;
        font_info.usage = ImageUsageFlags::Sampled | ImageUsageFlags::TransferDst;
        font_info.format = ImageFormat::UNORM_RGBA;
        font_info.width = font_texture_width;
        font_info.height = font_texture_height;
        font_info.data = pixels;

        _font_texture = device.create_image(font_info, { });

        auto update_texture_task = [](GfxDevice& device, GfxFrame& gfx_frame, Image image, ImageInfo image_info) noexcept -> ice::Task<>
        {
            ice::gfx::GfxTaskLoadImage const task_info{
                .image = image,
                .image_info = image_info
            };

            co_await ice::gfx::load_image_data_task(device, gfx_frame, task_info);
        };


        _shader_stages[0] = ShaderStageFlags::VertexStage;
        _shader_stages[1] = ShaderStageFlags::FragmentStage;
        _shaders[0] = device.create_shader(ShaderInfo{ .shader_data = vtx_shader_data });
        _shaders[1] = device.create_shader(ShaderInfo{ .shader_data = pix_shader_data });

        SamplerInfo sampler_info{
            .min_filter = SamplerFilter::Nearest,
            .mag_filter = SamplerFilter::Nearest,
            .address_mode = {
                .u = SamplerAddressMode::ClampToEdge,
                .v = SamplerAddressMode::ClampToEdge,
                .w = SamplerAddressMode::ClampToEdge,
            },
            .mip_map_mode = SamplerMipMapMode::Nearest,
        };

        _sampler = device.create_sampler(sampler_info);

        ResourceSetLayoutBinding const resource_bindings[]{
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

        ResourceUpdateInfo resource_update[]{
            ResourceUpdateInfo
            {
                .sampler = _sampler
            },
            ResourceUpdateInfo
            {
                .image = _font_texture,
            },
        };

        ResourceSetUpdateInfo update_infos[]{
            ResourceSetUpdateInfo{
                .resource_set = _resources[0],
                .resource_type = ResourceType::Sampler,
                .binding_index = 1,
                .array_element = 0,
                .resources = { resource_update + 0, 1 }
            },
            ResourceSetUpdateInfo{
                .resource_set = _resources[0],
                .resource_type = ResourceType::SampledImage,
                .binding_index = 2,
                .array_element = 0,
                .resources = { resource_update + 1, 1 }
            },
        };

        device.update_resourceset(update_infos);

        PipelinePushConstant const push_constants[]{
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

        ShaderInputAttribute attribs[]{
            ShaderInputAttribute{
                .location = 0,
                .offset = 0,
                .type = ShaderAttribType::Vec2f
            },
            ShaderInputAttribute{
                .location = 1,
                .offset = 8,
                .type = ShaderAttribType::Vec2f
            },
            ShaderInputAttribute{
                .location = 2,
                .offset = 16,
                .type = ShaderAttribType::Vec4f_Unorm8
            },
        };

        ShaderInputBinding bindings[]{
            ShaderInputBinding{
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
            .subpass_index = 2,
            .depth_test = true
        };

        _pipeline = device.create_pipeline(pipeline_info);

        ice::pod::array::push_back(
            _index_buffers,
            device.create_buffer(BufferType::Index, 1024 * 1024 * 4)
        );
        ice::pod::array::push_back(
            _index_buffers,
            device.create_buffer(BufferType::Index, 1024 * 1024 * 4)
        );
        ice::pod::array::push_back(
            _vertex_buffers,
            device.create_buffer(BufferType::Vertex, 1024 * 1024 * 4)
        );
        ice::pod::array::push_back(
            _vertex_buffers,
            device.create_buffer(BufferType::Vertex, 1024 * 1024 * 4)
        );

        co_await runner.schedule_next_frame();

        runner.execute_task(
            update_texture_task(gfx_device, runner.graphics_frame(), _font_texture, font_info),
            EngineContext::GraphicsFrame
        );

        co_await runner.schedule_next_frame();

        _initialized = true;
        co_return;
    }

    auto ImGuiTrait::task_destroy_render_objects(ice::gfx::GfxDevice& gfx_device) noexcept -> ice::Task<>
    {
        using namespace ice::gfx;
        using namespace ice::render;

        RenderDevice& device = gfx_device.device();
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

    auto ImGuiTrait::task_update_buffers(
        ice::EngineFrame& frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept -> ice::Task<>
    {
        IPT_ZONE_SCOPED_NAMED("DevUI - Gfx - ImGui update buffers.");

        using namespace ice::gfx;
        using namespace ice::render;

        ImDrawData* draw_data = ImGui::GetDrawData();
        if (draw_data == nullptr)
        {
            co_return;
        }

        ice::Span<detail::DrawCommand>* cmds_span = frame.named_object<ice::Span<detail::DrawCommand>>("ice.devui.imgui_draw_commands_span"_sid);
        if (cmds_span == nullptr)
        {
            co_return;
        }

        RenderDevice& device = gfx_device.device();

        ice::u32 buffer_update_count = 0;
        BufferUpdateInfo buffer_updates[10]{ };

        ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
        ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        ImTextureID last_texid = nullptr; // ImGui::GetIO().tex
        ice::u32 next_resource_idx = 1;
        ResourceUpdateInfo resource_update[]{
            ResourceUpdateInfo
            {
                .sampler = _sampler
            },
            ResourceUpdateInfo
            {
                .image = Image::Invalid,
            },
        };

        ResourceSetUpdateInfo resource_set_update[]{
            ResourceSetUpdateInfo{
                .resource_type = ResourceType::Sampler,
                .binding_index = 1,
                .array_element = 0,
                .resources = { resource_update + 0, 1 }
            },
            ResourceSetUpdateInfo{
                .resource_type = ResourceType::SampledImage,
                .binding_index = 2,
                .array_element = 0,
                .resources = { resource_update + 1, 1 }
            },
        };

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
                detail::DrawCommand& cmd = (*cmds_span)[command_idx];
                cmd.resource_set = ResourceSet::Invalid;

                if (pcmd->TextureId != nullptr && pcmd->TextureId != last_texid)
                {
                    last_texid = pcmd->TextureId;
                    resource_update[1].image = static_cast<Image>(reinterpret_cast<ice::uptr>(last_texid));
                    resource_set_update[0].resource_set = _resources[next_resource_idx];
                    resource_set_update[1].resource_set = _resources[next_resource_idx];
                    cmd.resource_set = _resources[next_resource_idx];

                    device.update_resourceset(resource_set_update);
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

            BufferUpdateInfo& vtx_info = buffer_updates[buffer_update_count];
            vtx_info.buffer = _vertex_buffers[0];
            vtx_info.offset = vtx_buffer_offset * sizeof(ImDrawVert);
            vtx_info.data = ice::data_view(cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), alignof(ImDrawVert));
            vtx_buffer_offset += cmd_list->VtxBuffer.Size;

            BufferUpdateInfo& idx_info = buffer_updates[buffer_update_count + 1];
            idx_info.buffer = _index_buffers[0];
            idx_info.offset = idx_buffer_offset * sizeof(ImDrawIdx);
            idx_info.data = ice::data_view(cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), alignof(ImDrawIdx));
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
        co_return;
    }


} // namespace ice::devui

//            using namespace core::math;
//
//            unsigned char* pixels;
//            int width, height;
//            io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
//
//            uint32_t upload_size = width * height * 4 * sizeof(char);
//
//            iceshard::renderer::api::v1_1::data::Texture font_texture;
//            font_texture.width = width;
//            font_texture.height = height;
//            font_texture.format = iceshard::renderer::api::TextureFormat::UnormRGBA;
//            font_texture.data = pixels;
//
//            asset::AssetData tex_data{ };
//            tex_data.content = { &font_texture, sizeof(font_texture) };
//
//            _font_texture = create_texture(
//                "imgui.fonts"_sid,
//                tex_data
//            );
//
//            _temp_texture_data = create_buffer(
//                api::BufferType::TransferBuffer,
//                upload_size
//            );
//
//            {
//                auto temp_buffer_arr = core::pod::array::create_view(&_temp_buffer, 1);
//                create_command_buffers(
//                    api::CommandBufferType::Primary,
//                    temp_buffer_arr
//                );
//            }
//
//            api::DataView buffer_view;
//            map_buffer(_temp_texture_data, buffer_view);
//            memcpy(buffer_view.data, pixels, upload_size);
//            unmap_buffer(_temp_texture_data);
//
//            commands::begin(_temp_buffer, api::CommandBufferUsage::RunOnce);
//            commands::update_texture(_temp_buffer, _font_texture, _temp_texture_data, vec2<u32>(width, height));
//            commands::end(_temp_buffer);
//            _render_system.submit_command_buffer_v2(_temp_buffer);
//        }
//
//        //_font_texture = detail::create_fonts_texture(alloc, _io, _render_system);
//        io.Fonts->TexID = reinterpret_cast<ImTextureID>(_font_texture);
//    }
//
//    auto ImGuiRenderer::draw_task(
//        iceshard::renderer::api::CommandBuffer cb,
//        ImDrawData* draw_data
//    ) noexcept -> cppcoro::task<>
//    {
//        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
//        int fb_width = static_cast<int>(_io.DisplaySize.x * _io.DisplayFramebufferScale.x);
//        int fb_height = static_cast<int>(_io.DisplaySize.y * _io.DisplayFramebufferScale.y);
//        if (fb_width == 0 || fb_height == 0)
//        {
//            co_return;
//        }
//
//        //auto draw_area_ext = render_area();
//
//        float scale[2];
//        scale[0] = 2.0f / fb_width;
//        scale[1] = 2.0f / fb_height;
//        float translate[2];
//        translate[0] = -1.0f; // -1.0f - width * scale[0];
//        translate[1] = -1.0f; //-1.0f - height * scale[1];
//
//        using iceshard::renderer::RenderPassStage;
//        using iceshard::renderer::api::Buffer;
//        using iceshard::renderer::api::DataView;
//        using namespace iceshard::renderer::commands;
//
//        push_constants(cb, _pipeline, { scale, sizeof(scale) });
//        push_constants(cb, _pipeline, { translate, sizeof(translate) }, sizeof(scale));
//        bind_pipeline(cb, _pipeline);
//        bind_resource_set(cb, _resource_set);
//        bind_vertex_buffers(cb, _vertice_buffers);
//        bind_index_buffer(cb, _indice_buffer);
//        set_viewport(cb, (uint32_t)_io.DisplaySize.x, (uint32_t)_io.DisplaySize.y);
//        set_scissor(cb, (uint32_t)_io.DisplaySize.x, (uint32_t)_io.DisplaySize.y);
//
//        // Upload vertex/index data into a single contiguous GPU buffer
//        {
//            Buffer buff[] = { _indice_buffer, _vertice_buffers[0] };
//            DataView buff_view[2];
//
//            iceshard::renderer::api::render_module_api->buffer_array_map_data_func(buff, buff_view, 2);
//
//            for (int n = 0; n < draw_data->CmdListsCount; n++)
//            {
//                const ImDrawList* cmd_list = draw_data->CmdLists[n];
//
//                IS_ASSERT(buff_view[0].size > cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), "how?");
//                IS_ASSERT(buff_view[1].size > cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), "how?");
//                buff_view[0].size -= cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);
//                buff_view[1].size -= cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
//
//                memcpy(buff_view[0].data, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
//                memcpy(buff_view[1].data, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
//                buff_view[0].data = core::memory::utils::pointer_add(buff_view[0].data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
//                buff_view[1].data = core::memory::utils::pointer_add(buff_view[1].data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
//            }
//
//            iceshard::renderer::api::render_module_api->buffer_array_unmap_data_func(buff, 2);
//        }
//
//        ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
//        ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)
//
//        uint32_t vtx_buffer_offset = 0;
//        uint32_t idx_buffer_offset = 0;
//        for (int32_t i = 0; i < draw_data->CmdListsCount; i++)
//        {
//            ImDrawList const* cmd_list = draw_data->CmdLists[i];
//            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
//            {
//                ImDrawCmd const* pcmd = &cmd_list->CmdBuffer[cmd_i];
//                if (pcmd->UserCallback)
//                {
//                    pcmd->UserCallback(cmd_list, pcmd);
//                }
//                else
//                {
//                    ImVec4 clip_rect;
//                    clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
//                    clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
//                    clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
//                    clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;
//
//
//
//                    if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
//                    {
//                        // Negative offsets are illegal for vkCmdSetScissor
//                        if (clip_rect.x < 0.0f)
//                            clip_rect.x = 0.0f;
//                        if (clip_rect.y < 0.0f)
//                            clip_rect.y = 0.0f;
//
//                        // Apply scissor/clipping rectangle
//                        core::math::vec4i scissor;
//                        scissor.x = (int32_t)(clip_rect.x);
//                        scissor.y = (int32_t)(clip_rect.y);
//                        scissor.z = (uint32_t)(clip_rect.z - clip_rect.x);
//                        scissor.w = (uint32_t)(clip_rect.w - clip_rect.y);
//
//                        set_scissor(cb,
//                            scissor.x,
//                            scissor.y,
//                            scissor.z,
//                            scissor.w
//                        );
//
//                        // Draw
//                        draw_indexed(cb, pcmd->ElemCount, 1, pcmd->IdxOffset + idx_buffer_offset, pcmd->VtxOffset + vtx_buffer_offset, 0);
//                    }
//                }
//            }
//
//            vtx_buffer_offset += cmd_list->VtxBuffer.Size;
//            idx_buffer_offset += cmd_list->IdxBuffer.Size;
//        }
//
//        co_return;
//    }
//
//} // namespace debugui::imgui
