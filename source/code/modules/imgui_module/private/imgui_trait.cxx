/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "imgui_trait.hxx"
#include "imgui_system.hxx"

#include <ice/engine.hxx>
#include <ice/engine_devui.hxx>
#include <ice/engine_frame.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_shards.hxx>
#include <ice/engine_types_mappers.hxx>
#include <ice/world/world_updater.hxx>

#include <ice/gfx/gfx_context.hxx>
#include <ice/gfx/gfx_shards.hxx>
#include <ice/gfx/gfx_stage_registry.hxx>
#include <ice/render/render_swapchain.hxx>

#include <ice/input/input_event.hxx>
#include <ice/input/input_keyboard.hxx>
#include <ice/input/input_mouse.hxx>

#include <ice/asset.hxx>
#include <ice/profiler.hxx>
#include <ice/task_utils.hxx>
#include <ice/task.hxx>

namespace ice::devui
{

    ImGuiTrait::ImGuiTrait(ice::TraitContext& ctx, ice::Allocator& alloc, ImGuiSystem& system) noexcept
        : ice::Trait{ ctx }
        , ice::TraitDevUI{ {.category="Traits/Debug",.name="ImGUI-DevUI"} }
        , _allocator{ alloc, "trait:devui-imgui" }
        , _system{ system }
        , _imgui_gfx_stage{ }
        , _resized{ false }
    {
        _context.bind<&ImGuiTrait::update>();
        _context.bind<&ImGuiTrait::gfx_start, Render>(ice::gfx::ShardID_GfxStartup);
        _context.bind<&ImGuiTrait::gfx_shutdown, Render>(ice::gfx::ShardID_GfxShutdown);
        _context.bind<&ImGuiTrait::gfx_update, Render>(ice::gfx::ShardID_RenderFrameUpdate);
        _context.bind<&ImGuiTrait::on_window_resized>(ice::platform::ShardID_WindowResized);
    }

    ImGuiTrait::~ImGuiTrait() noexcept
    {
    }

    auto ImGuiTrait::activate(ice::WorldStateParams const& update) noexcept -> ice::Task<>
    {
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

        ice::u8* pixels;
        ice::i32 font_texture_width, font_texture_height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &font_texture_width, &font_texture_height);
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
                    InputID constexpr left_alt_key_mod = ice::input::input_identifier(DeviceType::Keyboard, KeyboardMod::AltLeft, mod_identifier_base_value);
                    InputID constexpr right_alt_key_mod = ice::input::input_identifier(DeviceType::Keyboard, KeyboardMod::AltRight, mod_identifier_base_value);

                    if (input.identifier == left_ctrl_key_mod || input.identifier == right_ctrl_key_mod)
                    {
                        io.KeyCtrl = input.value.button.state.pressed;
                    }
                    if (input.identifier == left_shift_key_mod || input.identifier == right_shift_key_mod)
                    {
                        io.KeyShift = input.value.button.state.pressed;
                    }
                    if (input.identifier == left_alt_key_mod || input.identifier == right_alt_key_mod)
                    {
                        io.KeyAlt = input.value.button.state.pressed;
                    }
                }

            }
        );
        co_return;
    }

    void ImGuiTrait::build_content() noexcept
    {
        _system.devui_draw(_stats);
    }

    auto ImGuiTrait::gfx_start(
        ice::gfx::GfxStateChange const& params,
        ice::gfx::GfxContext& ctx,
        ice::AssetStorage& assets
    ) noexcept -> ice::Task<>
    {
        _imgui_gfx_stage = ice::make_unique<ImGuiGfxStage>(_allocator, _allocator, assets);
        params.registry.register_stage("iceshard.devui"_sid, _imgui_gfx_stage.get());

        ice::vec2u const size = ctx.swapchain().extent();
        co_await on_window_resized(ice::vec2i{ size });
        co_return;
    }

    auto ImGuiTrait::gfx_shutdown(ice::gfx::GfxStateChange const& params) noexcept -> ice::Task<>
    {
        params.registry.remove_stage("iceshard.devui"_sid);
        co_return;
    }

    auto ImGuiTrait::gfx_update(ice::gfx::RenderFrameUpdate const& update) noexcept -> ice::Task<>
    {
        IPT_ZONE_SCOPED;
        ImGuiIO const& io = ImGui::GetIO();
        if (io.DisplaySize.x <= 0.f || io.DisplaySize.y <= 0.f || _resized)
        {
            _resized = false;
            co_return;
        }

        ImGui::Render();

        ImDrawData* draw_data = ImGui::GetDrawData();
        if (draw_data != nullptr)
        {
            ice::u32 cmd_count = 0;
            for (ice::i32 i = 0; i < draw_data->CmdListsCount; i++)
            {
                ImDrawList const* cmd_list = draw_data->CmdLists[i];
                cmd_count += cmd_list->CmdBuffer.Size;
            }

            ice::array::resize(_imgui_gfx_stage->draw_commands, cmd_count);

            build_internal_command_list(_imgui_gfx_stage->draw_commands);
        }
    }

    auto ImGuiTrait::on_window_resized(ice::vec2i new_size) noexcept -> ice::Task<>
    {
        auto& io = ImGui::GetIO();
        io.DisplaySize.x = (ice::f32)new_size.x;
        io.DisplaySize.y = (ice::f32)new_size.y;
        _resized = true;
        co_return;
    }

    void ImGuiTrait::build_internal_command_list(ice::Span<ImGuiGfxStage::DrawCommand> draw_cmds) noexcept
    {
        IPT_ZONE_SCOPED;
        _stats = {}; // reset stats

        ImDrawData* draw_data = ImGui::GetDrawData();
        if (draw_data == nullptr)
        {
            return;
        }

        using ice::render::Image;
        using ice::render::ResourceSet;

        ice::Timestamp const start_ts = ice::clock::now();

        ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
        ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        ImTextureID last_texid = nullptr;
        ice::u32 curr_resource_idx = 1;

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
                ImGuiGfxStage::DrawCommand& cmd = draw_cmds[command_idx];

                if (pcmd->TextureId != nullptr && pcmd->TextureId != last_texid)
                {
                    last_texid = pcmd->TextureId;
                    curr_resource_idx += 1;
                }

                cmd.resource_set_idx = curr_resource_idx;

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

            _stats.draw_calls += cmd_list->CmdBuffer.Size;
            _stats.draw_vertices += cmd_list->VtxBuffer.Size;
            _stats.draw_indices += cmd_list->IdxBuffer.Size;
            _stats.draw_datasize += ice::size_of<ImDrawVert> * cmd_list->VtxBuffer.Size;
            _stats.draw_datasize += ice::size_of<ImDrawIdx> * cmd_list->IdxBuffer.Size;
        }

        ICE_ASSERT(
            vtx_buffer_offset * sizeof(ImDrawVert) < (1024 * 1024 * 4),
            "ImGui vertex buffer to big"
        );
        ICE_ASSERT(
            idx_buffer_offset * sizeof(ImDrawIdx) < (1024 * 1024 * 4),
            "ImGui index buffer to big"
        );

        _stats.draw_processtime = ice::clock::elapsed(start_ts, ice::clock::now());
    }

} // namespace ice::devui
