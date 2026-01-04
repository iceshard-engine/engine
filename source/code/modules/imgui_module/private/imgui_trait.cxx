/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
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
#include <ice/render/render_image.hxx>
#include <ice/render/render_device.hxx>

#include <ice/input/input_event.hxx>
#include <ice/input/input_keyboard.hxx>
#include <ice/input/input_mouse.hxx>

#include <ice/asset.hxx>
#include <ice/config/config_builder.hxx>
#include <ice/profiler.hxx>
#include <ice/task_utils.hxx>
#include <ice/string_utils.hxx>
#include <ice/task.hxx>

namespace ice::devui
{
    namespace detail
    {

        constexpr auto ice_to_imgui_key(ice::input::KeyboardKey key) noexcept -> ImGuiKey
        {
            switch (key)
            {
                using enum ice::input::KeyboardKey;
            case Tab: return ImGuiKey_Tab;
            case Left: return ImGuiKey_LeftArrow;
            case Right: return ImGuiKey_RightArrow;
            case Up: return ImGuiKey_UpArrow;
            case Down: return ImGuiKey_DownArrow;
            case PageUp: return ImGuiKey_PageUp;
            case PageDown: return ImGuiKey_PageDown;
            case Home: return ImGuiKey_Home;
            case End: return ImGuiKey_End;
            case Insert: return ImGuiKey_Insert;
            case Delete: return ImGuiKey_Delete;
            case Backspace: return ImGuiKey_Backspace;
            case Space: return ImGuiKey_Space;
            case Return: return ImGuiKey_Enter;
            case Escape: return ImGuiKey_Escape;
            case Quote: return ImGuiKey_Apostrophe;
            case Comma: return ImGuiKey_Comma;
            case Minus: return ImGuiKey_Minus;
            case Period: return ImGuiKey_Period;
            case Slash: return ImGuiKey_Slash;
            case SemiColon: return ImGuiKey_Semicolon;
            case Equals: return ImGuiKey_Equal;
            case LeftBracket: return ImGuiKey_LeftBracket;
            case BackSlash: return ImGuiKey_Backslash;
            case RightBracket: return ImGuiKey_RightBracket;
            case BackQuote: return ImGuiKey_GraveAccent;
            case KeyCapsLock: return ImGuiKey_CapsLock;
            case ScrollLock: return ImGuiKey_ScrollLock;
            case NumPadNumlockClear: return ImGuiKey_NumLock;
            case PrintScreen: return ImGuiKey_PrintScreen;
            case Pause: return ImGuiKey_Pause;
            case NumPad0: return ImGuiKey_Keypad0;
            case NumPad1: return ImGuiKey_Keypad1;
            case NumPad2: return ImGuiKey_Keypad2;
            case NumPad3: return ImGuiKey_Keypad3;
            case NumPad4: return ImGuiKey_Keypad4;
            case NumPad5: return ImGuiKey_Keypad5;
            case NumPad6: return ImGuiKey_Keypad6;
            case NumPad7: return ImGuiKey_Keypad7;
            case NumPad8: return ImGuiKey_Keypad8;
            case NumPad9: return ImGuiKey_Keypad9;
            //case DLK_KP_PERIOD: return ImGuiKey_KeypadDecimal; // todo?
            case NumPadDivide: return ImGuiKey_KeypadDivide;
            case NumPadMultiply: return ImGuiKey_KeypadMultiply;
            case NumPadMinus: return ImGuiKey_KeypadSubtract;
            case NumPadPlus: return ImGuiKey_KeypadAdd;
            case NumPadEnter: return ImGuiKey_KeypadEnter;
            //case SDLK_KP_EQUALS: return ImGuiKey_KeypadEqual; // todo?
            case KeyLeftCtrl: return ImGuiKey_LeftCtrl;
            case KeyLeftShift: return ImGuiKey_LeftShift;
            case KeyLeftAlt: return ImGuiKey_LeftAlt;
            case KeyLeftGui: return ImGuiKey_LeftSuper;
            case KeyRightCtrl: return ImGuiKey_RightCtrl;
            case KeyRightShift: return ImGuiKey_RightShift;
            case KeyRightAlt: return ImGuiKey_RightAlt;
            case KeyRightGui: return ImGuiKey_RightSuper;
            //case SDLK_APPLICATION: return ImGuiKey_Menu; // todo?
            case Key0: return ImGuiKey_0;
            case Key1: return ImGuiKey_1;
            case Key2: return ImGuiKey_2;
            case Key3: return ImGuiKey_3;
            case Key4: return ImGuiKey_4;
            case Key5: return ImGuiKey_5;
            case Key6: return ImGuiKey_6;
            case Key7: return ImGuiKey_7;
            case Key8: return ImGuiKey_8;
            case Key9: return ImGuiKey_9;
            case KeyA: return ImGuiKey_A;
            case KeyB: return ImGuiKey_B;
            case KeyC: return ImGuiKey_C;
            case KeyD: return ImGuiKey_D;
            case KeyE: return ImGuiKey_E;
            case KeyF: return ImGuiKey_F;
            case KeyG: return ImGuiKey_G;
            case KeyH: return ImGuiKey_H;
            case KeyI: return ImGuiKey_I;
            case KeyJ: return ImGuiKey_J;
            case KeyK: return ImGuiKey_K;
            case KeyL: return ImGuiKey_L;
            case KeyM: return ImGuiKey_M;
            case KeyN: return ImGuiKey_N;
            case KeyO: return ImGuiKey_O;
            case KeyP: return ImGuiKey_P;
            case KeyQ: return ImGuiKey_Q;
            case KeyR: return ImGuiKey_R;
            case KeyS: return ImGuiKey_S;
            case KeyT: return ImGuiKey_T;
            case KeyU: return ImGuiKey_U;
            case KeyV: return ImGuiKey_V;
            case KeyW: return ImGuiKey_W;
            case KeyX: return ImGuiKey_X;
            case KeyY: return ImGuiKey_Y;
            case KeyZ: return ImGuiKey_Z;
            case KeyF1: return ImGuiKey_F1;
            case KeyF2: return ImGuiKey_F2;
            case KeyF3: return ImGuiKey_F3;
            case KeyF4: return ImGuiKey_F4;
            case KeyF5: return ImGuiKey_F5;
            case KeyF6: return ImGuiKey_F6;
            case KeyF7: return ImGuiKey_F7;
            case KeyF8: return ImGuiKey_F8;
            case KeyF9: return ImGuiKey_F9;
            case KeyF10: return ImGuiKey_F10;
            case KeyF11: return ImGuiKey_F11;
            case KeyF12: return ImGuiKey_F12;
            //case SDLK_F13: return ImGuiKey_F13; // todo?
            //case SDLK_F14: return ImGuiKey_F14;
            //case SDLK_F15: return ImGuiKey_F15;
            //case SDLK_F16: return ImGuiKey_F16;
            //case SDLK_F17: return ImGuiKey_F17;
            //case SDLK_F18: return ImGuiKey_F18;
            //case SDLK_F19: return ImGuiKey_F19;
            //case SDLK_F20: return ImGuiKey_F20;
            //case SDLK_F21: return ImGuiKey_F21;
            //case SDLK_F22: return ImGuiKey_F22;
            //case SDLK_F23: return ImGuiKey_F23;
            //case SDLK_F24: return ImGuiKey_F24;
            //case SDLK_AC_BACK: return ImGuiKey_AppBack; // todo?
            //case SDLK_AC_FORWARD: return ImGuiKey_AppForward; // todo?
            default: break;
            }
            return ImGuiKey_None;
        }

        struct ImTextureAssetDataBinding : ice::AssetDataBinding
        {
            ImTextureAssetDataBinding(
                ice::Allocator& alloc,
                ImTextureData const& texture
            ) noexcept
                : AssetDataBinding{ .state = AssetState::Loaded }
                , _allocator{ alloc }
            {
                using namespace ice::render;

                ice::Data const texture_data{
                    texture.Pixels,
                    ice::usize(texture.GetSizeInBytes()),
                    ice::ualign::b_1
                };

                _texdata = alloc.allocate(
                    ice::AllocRequest{
                        ice::size_of<ImageInfo> + texture_data.size,
                        ice::align_of<ImageInfo>
                    }
                );

                ice::Memory texmem = ice::ptr_add(_texdata, ice::size_of<ImageInfo>);
                ice::memcpy(texmem, { texture_data });

                ImageInfo* const info = reinterpret_cast<ImageInfo*>(_texdata.location);
                info->width = texture.Width;
                info->height = texture.Height;
                info->format = ImageFormat::UNORM_RGBA;
                info->type = ImageType::Image2D;
                info->usage = ImageUsageFlags::Sampled | ImageUsageFlags::TransferDst;
                info->data = texmem.location;
                this->content = ice::data_view(_texdata);
            }

            ~ImTextureAssetDataBinding()
            {
                _allocator.deallocate(_texdata);
            }

            ice::Allocator& _allocator;
            ice::Memory _texdata;
        };

        inline auto total_command_count(ImDrawData const& draw_data) noexcept -> ice::u32
        {
            ice::u32 cmd_count = 0;
            for (ice::i32 i = 0; i < draw_data.CmdListsCount; i++)
            {
                ImDrawList const* cmd_list = draw_data.CmdLists[i];
                cmd_count += cmd_list->CmdBuffer.Size;
            }
            return cmd_count;
        }

        inline bool is_invalid_texture(ImTextureRef const& ref) noexcept
        {
            return ref.GetTexID() == ImTextureID_Invalid;
        }

        inline bool is_invalid_texture(ImDrawCmd const& cmd) noexcept
        {
            return is_invalid_texture(cmd.TexRef);
        }

        inline bool is_different_texture(ImDrawCmd const& cmd, ImTextureID last_id) noexcept
        {
            ImTextureID const texid = cmd.TexRef.GetTexID();
            return texid != ImTextureID_Invalid && texid != last_id;
        }

        static constexpr fmt::string_view TextureNameFormat = "imgui-backend-texture-{}";

    } // namespace detail


    ImGuiTrait::ImGuiTrait(ice::TraitContext& ctx, ice::Allocator& alloc, ImGuiSystem& system) noexcept
        : ice::Trait{ ctx }
        , ice::TraitDevUI{ {.category="Traits/Debug",.name="ImGUI-DevUI"} }
        , _allocator{ alloc, "trait:devui-imgui" }
        , _system{ system }
        , _stats{ }
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

        ice::vec2f temp_pos{};
        io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);

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
                        io.AddMouseButtonEvent(ImGuiMouseButton_Left, input.value.button.state.pressed);
                        break;
                    case MouseInput::ButtonRight:
                        io.AddMouseButtonEvent(ImGuiMouseButton_Right, input.value.button.state.pressed);
                        break;
                    case MouseInput::ButtonMiddle:
                        io.AddMouseButtonEvent(ImGuiMouseButton_Middle, input.value.button.state.pressed);
                        break;
                    case MouseInput::PositionX:
                        temp_pos.x = (ice::f32)input.value.axis.value_i32;
                        break;
                    case MouseInput::PositionY:
                        temp_pos.y = (ice::f32)input.value.axis.value_i32;
                        io.AddMousePosEvent(temp_pos.x, temp_pos.y);
                        break;
                    case MouseInput::Wheel:
                        io.AddMouseWheelEvent(0.0f, (ice::f32)input.value.axis.value_i32);
                        break;
                    }
                }
                if (ice::input::input_identifier_device(input.identifier) == ice::input::DeviceType::Keyboard)
                {
                    using namespace ice::input;
                    ice::u32 const input_source = ice::input::input_identifier_value(input.identifier);

                    io.AddKeyEvent(
                        detail::ice_to_imgui_key(KeyboardKey(input_source)),
                        input.value.button.state.pressed
                    );

                    InputID constexpr left_ctrl_key_mod = ice::input::input_identifier(DeviceType::Keyboard, KeyboardMod::CtrlLeft, mod_identifier_base_value);
                    InputID constexpr right_ctrl_key_mod = ice::input::input_identifier(DeviceType::Keyboard, KeyboardMod::CtrlRight, mod_identifier_base_value);
                    InputID constexpr left_shift_key_mod = ice::input::input_identifier(DeviceType::Keyboard, KeyboardMod::ShiftLeft, mod_identifier_base_value);
                    InputID constexpr right_shift_key_mod = ice::input::input_identifier(DeviceType::Keyboard, KeyboardMod::ShiftRight, mod_identifier_base_value);
                    InputID constexpr left_alt_key_mod = ice::input::input_identifier(DeviceType::Keyboard, KeyboardMod::AltLeft, mod_identifier_base_value);
                    InputID constexpr right_alt_key_mod = ice::input::input_identifier(DeviceType::Keyboard, KeyboardMod::AltRight, mod_identifier_base_value);
                    InputID constexpr left_gui_key_mod = ice::input::input_identifier(DeviceType::Keyboard, KeyboardMod::GuiLeft, mod_identifier_base_value);
                    InputID constexpr right_gui_key_mod = ice::input::input_identifier(DeviceType::Keyboard, KeyboardMod::GuiRight, mod_identifier_base_value);

                    if (input.identifier == left_ctrl_key_mod || input.identifier == right_ctrl_key_mod)
                    {
                        io.AddKeyEvent(ImGuiMod_Ctrl, input.value.button.state.pressed);
                    }
                    if (input.identifier == left_shift_key_mod || input.identifier == right_shift_key_mod)
                    {
                        io.AddKeyEvent(ImGuiMod_Shift, input.value.button.state.pressed);
                    }
                    if (input.identifier == left_alt_key_mod || input.identifier == right_alt_key_mod)
                    {
                        io.AddKeyEvent(ImGuiMod_Alt, input.value.button.state.pressed);
                    }
                    if (input.identifier == left_gui_key_mod || input.identifier == right_gui_key_mod)
                    {
                        io.AddKeyEvent(ImGuiMod_Super, input.value.button.state.pressed);
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

    auto ImGuiTrait::gfx_shutdown(
        ice::gfx::GfxStateChange const& params,
        ice::gfx::GfxContext& ctx
    ) noexcept -> ice::Task<>
    {
        params.registry.remove_stage("iceshard.devui"_sid);
        co_return;
    }

    auto ImGuiTrait::load_texture(
        ice::gfx::GfxContext& gfx,
        ice::AssetStorage& assets,
        ice::TaskScheduler scheduler,
        ImTextureData* texture
    ) noexcept -> ice::Task<>
    {
        IPT_ZONE_SCOPED_NAMED("ImGui - Load texture");

        ice::HeapString<> texture_name{ _allocator };
        ice::string::push_format(texture_name, detail::TextureNameFormat, texture->UniqueID);
        IPT_ZONE_TEXT_STR(texture_name);

        detail::ImTextureAssetDataBinding texture_binding{ _allocator, *texture };
        ice::Asset tex_asset = assets.bind_data(
            ice::render::AssetCategory_Texture2D,
            texture_name,
            texture_binding
        );

        // Store the asset handle in the backend user-data pointer.
        texture->BackendUserData = tex_asset._handle;

        ice::Data tex_handle = co_await ice::await_on(tex_asset[AssetState::Runtime], scheduler);

        // Get the image handle
        ice::render::Image const* const tex = reinterpret_cast<ice::render::Image const*>(tex_handle.location);

        // Update the texture object
        texture->SetTexID(static_cast<ImTextureID>(*tex));
        texture->SetStatus(ImTextureStatus_OK);
    }

    auto ImGuiTrait::gfx_update(
        ice::gfx::RenderFrameUpdate const& update,
        ice::AssetStorage& assets
    ) noexcept -> ice::Task<>
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
            if (draw_data->Textures != nullptr)
            {
                for (ImTextureData* texture : *draw_data->Textures)
                {
                    if (texture->Status == ImTextureStatus_WantCreate
                        && texture->BackendUserData == nullptr)
                    {
                        ice::execute_task(
                            load_texture(update.context, assets, update.stages.scheduler, texture)
                        );
                    }
                }
            }

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

    void ImGuiTrait::build_internal_command_list(ice::Array<ImGuiGfxStage::DrawCommand>& out_draw_cmds) noexcept
    {
        IPT_ZONE_SCOPED;
        _stats = {}; // reset stats

        ice::array::clear(_imgui_gfx_stage->draw_commands);

        ImDrawData* draw_data = ImGui::GetDrawData();
        if (draw_data == nullptr)
        {
            return;
        }

        // Reserve enough space for all possible commands
        ice::array::reserve(
            _imgui_gfx_stage->draw_commands,
            detail::total_command_count(*draw_data)
        );

        using ice::render::Image;
        using ice::render::ResourceSet;

        ice::Timestamp const start_ts = ice::clock::now();

        ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
        ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        ImTextureID last_texid = ImTextureID_Invalid;
        ice::u32 curr_resource_idx = 1;

        // Upload vertex/index data into a single contiguous GPU buffer
        ice::u32 vtx_buffer_offset = 0;
        ice::u32 idx_buffer_offset = 0;

        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            ImDrawList const* cmd_list = draw_data->CmdLists[n];
            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
            {
                ImDrawCmd const* pcmd = &cmd_list->CmdBuffer[cmd_i];
                if (detail::is_invalid_texture(*pcmd))
                {
                    continue;
                }

                if (detail::is_different_texture(*pcmd, last_texid))
                {
                    last_texid = pcmd->GetTexID();
                    curr_resource_idx += 1;
                }

                ice::array::push_back(out_draw_cmds, ImGuiGfxStage::DrawCommand{});
                ImGuiGfxStage::DrawCommand& cmd = ice::array::back(out_draw_cmds);
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
