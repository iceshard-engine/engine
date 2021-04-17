#pragma once
#include <ice/memory/memory_globals.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/render/render_pipeline.hxx>
#include <ice/render/render_command_buffer.hxx>
#include <ice/render/render_buffer.hxx>
#include <ice/render/render_image.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_device.hxx>

#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_queue.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>
#include <ice/gfx/gfx_subpass.hxx>

#include <ice/engine.hxx>
#include <ice/engine_runner.hxx>

#include <ice/asset.hxx>
#include <ice/asset_system.hxx>

#include <ice/input/input_mouse.hxx>
#include <ice/input/input_keyboard.hxx>
#include <ice/input/input_event.hxx>

#include <ice/world/world_trait.hxx>
#include <ice/gfx/gfx_stage.hxx>

#include <imgui/imgui.h>
#undef assert

namespace ice
{

    class Ice_ImGui : public ice::WorldTrait, public ice::gfx::GfxStage
    {
    public:
        Ice_ImGui(
            ice::Allocator&,
            ice::Engine& engine,
            ice::render::RenderSwapchain const& swapchain
        ) noexcept
            : _engine{ engine }
        {
            _context = ImGui::CreateContext();

            auto& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

            ice::vec2u extent = swapchain.extent();
            io.DisplaySize.x = extent.x;
            io.DisplaySize.y = extent.y;
        }

        ~Ice_ImGui() noexcept
        {
            ImGui::DestroyContext(_context);
        }

        void on_activate(
            ice::EngineRunner& runner,
            ice::World& world
        ) noexcept
        {
            using namespace ice::gfx;
            using namespace ice::render;

            ice::u8* pixels;
            ice::i32 width, height;

            RenderDevice& device = runner.graphics_device().device();

            auto& io = ImGui::GetIO();
            io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

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

            ice::u32 upload_size = width * height * 4 * sizeof(char);

            ice::render::ImageInfo font_texture;
            font_texture.type = ImageType::Image2D;
            font_texture.usage = ImageUsageFlags::Sampled | ImageUsageFlags::TransferDst;
            font_texture.format = ImageFormat::UNORM_RGBA;
            font_texture.width = width;
            font_texture.height = height;

            _font_texture = device.create_image(font_texture, { });
            _font_transfer_buffer = device.create_buffer(BufferType::Transfer, upload_size);

            BufferUpdateInfo font_buffer_update[]{
                BufferUpdateInfo{
                    .buffer = _font_transfer_buffer,
                    .data = { pixels, upload_size }
                }
            };

            device.update_buffers(font_buffer_update);

            GfxQueue* gfx_queue = runner.graphics_frame().get_queue("default"_sid);

            CommandBuffer cmds;
            gfx_queue->alloc_command_buffers(CommandBufferType::Primary, { &cmds, 1 });

            RenderCommands& commands = runner.graphics_device().device().get_commands();
            commands.begin(cmds);
            commands.update_texture(cmds, _font_texture, _font_transfer_buffer, ice::vec2u(width, height));
            commands.end(cmds);

            gfx_queue->submit_command_buffers({ &cmds, 1 });

            io.Fonts->TexID = reinterpret_cast<ImTextureID>(_font_texture);

            auto& res = runner.graphics_device().resource_tracker();
            using namespace ice::gfx;

            ResourceSetLayout res_layouts[]{
                find_resource<ResourceSetLayout>(res, GfxSubpass_ImGui::ResName_ResourceLayout)
            };

            device.create_resourcesets(res_layouts, { &_resources, 1 });

            SamplerInfo sampler_info{
                .min_filter = SamplerFilter::Linear,
                .mag_filter = SamplerFilter::Linear,
                .address_mode = {
                    .u = SamplerAddressMode::Repeat,
                    .v = SamplerAddressMode::Repeat,
                    .w = SamplerAddressMode::Repeat,
                },
                .mip_map_mode = SamplerMipMapMode::Linear,
            };

            _sampler = device.create_sampler(sampler_info);

            ResourceUpdateInfo resource_update[]{
                ResourceUpdateInfo
                {
                    .image = _font_texture,
                },
                ResourceUpdateInfo
                {
                    .sampler = _sampler
                }
            };

            ResourceSetUpdateInfo update_infos[]{
                ResourceSetUpdateInfo{
                    .resource_set = _resources,
                    .resource_type = ResourceType::SampledImage,
                    .binding_index = 0,
                    .array_element = 0,
                    .resources = { resource_update, 1 }
                },
                ResourceSetUpdateInfo{
                    .resource_set = _resources,
                    .resource_type = ResourceType::Sampler,
                    .binding_index = 1,
                    .array_element = 0,
                    .resources = { resource_update + 1, 1 }
                }
            };

            device.update_resourceset(update_infos);

            auto& assets = _engine.asset_system();
            Asset vert_asset =  assets.request(AssetType::Shader, "/shaders/debug/imgui-vert"_sid);
            Asset frag_asset = assets.request(AssetType::Shader, "/shaders/debug/imgui-frag"_sid);

            Data vert_data;
            Data frag_data;
            ice::asset_data(vert_asset, vert_data);
            ice::asset_data(frag_asset, frag_data);

            ShaderStageFlags shader_stages[]{
                ShaderStageFlags::VertexStage,
                ShaderStageFlags::FragmentStage,
            };

            ShaderInfo shader_infos[]{
                ShaderInfo{.shader_data = *reinterpret_cast<Data const*>(vert_data.location) },
                ShaderInfo{.shader_data = *reinterpret_cast<Data const*>(frag_data.location) },
            };

            _shaders[0] = device.create_shader(shader_infos[0]);
            _shaders[1] = device.create_shader(shader_infos[1]);

            ShaderInputAttribute attribs[]{
                ShaderInputAttribute{.location = 0, .offset = 0, .type = ShaderAttribType::Vec2f },
                ShaderInputAttribute{.location = 1, .offset = 8, .type = ShaderAttribType::Vec2f },
                ShaderInputAttribute{.location = 2, .offset = 16, .type = ShaderAttribType::Vec4f_Unorm8 },
            };

            ShaderInputBinding bindings[]{
                ShaderInputBinding
                {
                    .binding = 0,
                    .stride = 20,
                    .attributes = attribs,
                }
            };

            _pl_layout = find_resource<PipelineLayout>(res, GfxSubpass_ImGui::ResName_PipelineLayout);

            _pipeline = device.create_pipeline(
                PipelineInfo
                {
                    .layout = _pl_layout,
                    .renderpass = find_resource<Renderpass>(res, "renderpass.default"_sid),
                    .shaders = _shaders,
                    .shaders_stages = shader_stages,
                    .shader_bindings = bindings,
                    .cull_mode = CullMode::Disabled,
                    .front_face = FrontFace::CounterClockWise,
                    .subpass_index = 1,
                    .depth_test = false,
                }
            );

            _indices = device.create_buffer(BufferType::Index, 256 * 1024);
            _vertices = device.create_buffer(BufferType::Vertex, 1 * 1024 * 1024);
            _device = &device;

            //ImGui::NewFrame();
        }

        void on_deactivate(
            ice::EngineRunner& runner,
            ice::World& world
        ) noexcept
        {
            using namespace ice::render;

            RenderDevice& device = runner.graphics_device().device();
            device.destroy_buffer(_vertices);
            device.destroy_buffer(_indices);

            device.destroy_pipeline(_pipeline);
            device.destroy_shader(_shaders[0]);
            device.destroy_shader(_shaders[1]);
            device.destroy_resourcesets({ &_resources, 1 });

            device.destroy_sampler(_sampler);

            device.destroy_buffer(_font_transfer_buffer);
            device.destroy_image(_font_texture);
        }

        void on_update(
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            ice::World& world
        ) noexcept
        {
            using ice::input::InputEvent;
            using ice::input::MouseInput;

            auto& io = ImGui::GetIO();

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
            }

            ImGui::NewFrame();
            ImGui::ShowDemoWindow();
        }

        void record_commands(
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer cmds,
            ice::render::RenderCommands& render_commands
        ) noexcept
        {
            auto& _io = ImGui::GetIO();
            // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
            int fb_width = static_cast<int>(_io.DisplaySize.x * _io.DisplayFramebufferScale.x);
            int fb_height = static_cast<int>(_io.DisplaySize.y * _io.DisplayFramebufferScale.y);
            if (fb_width == 0 || fb_height == 0)
            {
                return;
            }

            //auto draw_area_ext = render_area();

            float scale[2];
            scale[0] = 2.0f / fb_width;
            scale[1] = 2.0f / fb_height;
            float translate[2];
            translate[0] = -1.0f; // -1.0f - width * scale[0];
            translate[1] = -1.0f; //-1.0f - height * scale[1];

            using namespace ice::render;

            ImGui::EndFrame();
            ImGui::Render();

            ImDrawData* draw_data = ImGui::GetDrawData();

            using namespace ice::render;

            // Upload vertex/index data into a single contiguous GPU buffer
            {
                ice::u32 vertex_total_offset = 0;
                ice::u32 index_total_offset = 0;
                ice::pod::Array<BufferUpdateInfo> update_infos{ ice::memory::default_scratch_allocator() };

                for (int n = 0; n < draw_data->CmdListsCount; n++)
                {
                    const ImDrawList* cmd_list = draw_data->CmdLists[n];

                    BufferUpdateInfo update_info{ };
                    update_info.buffer = _vertices;
                    update_info.data.size = cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
                    update_info.data.location = cmd_list->VtxBuffer.Data;
                    update_info.offset = vertex_total_offset;
                    vertex_total_offset += cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
                    ice::pod::array::push_back(update_infos, update_info);

                    update_info.buffer = _indices;
                    update_info.data.size = cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);
                    update_info.data.location = cmd_list->IdxBuffer.Data;
                    update_info.offset = index_total_offset;
                    index_total_offset += cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);
                    ice::pod::array::push_back(update_infos, update_info);
                }

                _device->update_buffers(update_infos);
            }

            render_commands.push_constant(cmds, _pl_layout, ShaderStageFlags::VertexStage, { scale, sizeof(scale) }, 0);
            render_commands.push_constant(cmds, _pl_layout, ShaderStageFlags::VertexStage, { translate, sizeof(translate) }, sizeof(scale));
            render_commands.bind_pipeline(cmds, _pipeline);
            render_commands.bind_resource_set(cmds, _pl_layout, _resources, 0);
            render_commands.bind_vertex_buffer(cmds, _vertices, 0);
            render_commands.bind_index_buffer(cmds, _indices);
            render_commands.set_viewport(cmds, { 0, 0, (uint32_t)_io.DisplaySize.x, (uint32_t)_io.DisplaySize.y });
            render_commands.set_scissor(cmds, { 0, 0, (uint32_t)_io.DisplaySize.x, (uint32_t)_io.DisplaySize.y });


            ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
            ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

            uint32_t vtx_buffer_offset = 0;
            uint32_t idx_buffer_offset = 0;
            for (int32_t i = 0; i < draw_data->CmdListsCount; i++)
            {
                ImDrawList const* cmd_list = draw_data->CmdLists[i];
                for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
                {
                    ImDrawCmd const* pcmd = &cmd_list->CmdBuffer[cmd_i];
                    if (pcmd->UserCallback)
                    {
                        pcmd->UserCallback(cmd_list, pcmd);
                    }
                    else
                    {
                        ImVec4 clip_rect;
                        clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                        clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                        clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                        clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;



                        if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                        {
                            // Negative offsets are illegal for vkCmdSetScissor
                            if (clip_rect.x < 0.0f)
                                clip_rect.x = 0.0f;
                            if (clip_rect.y < 0.0f)
                                clip_rect.y = 0.0f;

                            // Apply scissor/clipping rectangle
                            ice::vec4u scissor;
                            scissor.x = (int32_t)(clip_rect.x);
                            scissor.y = (int32_t)(clip_rect.y);
                            scissor.z = (uint32_t)(clip_rect.z - clip_rect.x);
                            scissor.w = (uint32_t)(clip_rect.w - clip_rect.y);

                            render_commands.set_scissor(cmds, scissor);

                            // Draw
                            render_commands.draw_indexed(cmds, pcmd->ElemCount, 1, pcmd->IdxOffset + idx_buffer_offset, pcmd->VtxOffset + vtx_buffer_offset, 0);
                        }
                    }
                }

                vtx_buffer_offset += cmd_list->VtxBuffer.Size;
                idx_buffer_offset += cmd_list->IdxBuffer.Size;
            }
        }

    private:
        ice::Engine& _engine;
        ImGuiContext* _context;

        ice::render::Image _font_texture;
        ice::render::Buffer _font_transfer_buffer;

        ice::render::RenderDevice* _device;
        ice::render::Sampler _sampler;
        ice::render::Shader _shaders[2];
        ice::render::ResourceSet _resources;
        ice::render::PipelineLayout _pl_layout;
        ice::render::Pipeline _pipeline;
        ice::render::Buffer _indices;
        ice::render::Buffer _vertices;
    };

} // namespace ice
