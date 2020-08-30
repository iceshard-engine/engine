#include "iceshard_debug_input_states.hxx"

#include <iceshard/input/input_mouse.hxx>
#include <iceshard/input/input_keyboard.hxx>
#include <iceshard/input/input_controller.hxx>

#include <core/pod/hash.hxx>
#include <core/string_view.hxx>

#include <imgui/imgui.h>

namespace iceshard::debug
{

    DebugWindow_InputsStates::DebugWindow_InputsStates(core::allocator& alloc, bool& open_ref) noexcept
        : DebugWindow{}
        , _open{ open_ref }
        , _events{ alloc }
    {
    }

    void DebugWindow_InputsStates::update(Frame const& frame) noexcept
    {
        for (iceshard::input::InputEvent event : frame.input_events())
        {
            if (event.button_state == false || event.initial_event == true)
            {
                core::pod::hash::set(
                    _events,
                    core::hash(event.identifier),
                    InputEventEx{
                        .last_update = core::timeline::create_timeline(frame.engine_clock()),
                        .input = event,
                    }
                );
            }
            else if (event.button_state == true && event.initial_event == false)
            {
                if (core::pod::hash::has(_events, core::hash(event.identifier)))
                {
                    InputEventEx temp_event{
                        .last_update = core::Timeline{.clock = nullptr}
                    };

                    InputEventEx merged_event = core::pod::hash::get(_events, core::hash(event.identifier), temp_event);
                    merged_event.input.value.button.state_value |= event.value.button.state_value;
                    if (merged_event.input.value.button.state.pressed)
                    {
                        merged_event.input.value.button.state.released = false;
                    }
                    core::pod::hash::set(_events, core::hash(event.identifier), merged_event);
                }

            }
        }
    }

    void DebugWindow_InputsStates::end_frame() noexcept
    {
        if (_open == false)
        {
            return;
        }

        using namespace iceshard::input;

        core::StringView const NoYes_String[] = { "No", "Yes" };

        if (ImGui::Begin("Inputs (states)", &_open))
        {
            static auto InvalidTimeline = core::Timeline{ .clock = nullptr, };
            static auto InvalidEvent = InputEventEx{
                .last_update = InvalidTimeline
            };

            auto ImGuiText = [&](core::StringView format, core::Timeline const& timeline, bool state, auto... args) noexcept
            {
                auto const state_text = NoYes_String[state].data();

                ImVec4 color;
                if (timeline.clock == nullptr || state == false)
                {
                    ImGui::Text(format.data(), state_text, args...);
                }
                else
                {
                    auto const elapsed_time = core::timeline::elapsed(timeline);
                    if (elapsed_time <= 0.5f)
                    {
                        color = ImVec4{ 0.2f, 0.8f, 0.2f, 1.f };
                    }
                    else if (elapsed_time <= 2.0f)
                    {
                        color = ImVec4{ 0.8f, 0.8f, 0.2f, 1.f };
                    }
                    else
                    {
                        color = ImVec4{ 0.8f, 0.2f, 0.2f, 1.f };
                    }

                    ImGui::TextColored(color, format.data(), state_text, args...);
                }
                ImGui::NextColumn();
            };

            ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
            if (ImGui::TreeNode("Mouse"))
            {
                static constexpr auto Id_MousePosX = core::hash(create_inputid(DeviceType::Mouse, MouseInput::PositionX));
                static constexpr auto Id_MousePosY = core::hash(create_inputid(DeviceType::Mouse, MouseInput::PositionY));
                static constexpr auto Id_MouseWheel = core::hash(create_inputid(DeviceType::Mouse, MouseInput::Wheel));

                static constexpr InputID Id_Buttons[] = {
                    create_inputid(DeviceType::Mouse, MouseInput::ButtonLeft),
                    create_inputid(DeviceType::Mouse, MouseInput::ButtonMiddle),
                    create_inputid(DeviceType::Mouse, MouseInput::ButtonRight),
                    create_inputid(DeviceType::Mouse, MouseInput::ButtonCustom0),
                    create_inputid(DeviceType::Mouse, MouseInput::ButtonCustom1),
                };

                static constexpr core::StringView Name_Buttons[] = {
                    "Left", "Middle", "Right", "Custom0", "Custom1"
                };

                auto posEventX = core::pod::hash::get(_events, Id_MousePosX, InvalidEvent);
                auto posEventY = core::pod::hash::get(_events, Id_MousePosY, InvalidEvent);

                ImGui::Text("Position: %d x %d", posEventX.input.value.axis.value_i32, posEventY.input.value.axis.value_i32);

                ImGui::Columns(6);
                ImGui::Separator();

                ImGui::Text("Button");
                ImGui::NextColumn();
                ImGui::Text("Pressed");
                ImGui::NextColumn();
                ImGui::Text("Hold");
                ImGui::NextColumn();
                ImGui::Text("Released");
                ImGui::NextColumn();
                ImGui::Text("Clicked");
                ImGui::NextColumn();
                ImGui::Text("Repeat (num)");
                ImGui::NextColumn();


                uint32_t index = 0;
                for (auto eventId : Id_Buttons)
                {
                    auto const event_data = core::pod::hash::get(_events, core::hash(eventId), InvalidEvent);
                    auto const button_state = event_data.input.value.button.state;

                    ImGui::Separator();

                    ImGui::Text("%s", Name_Buttons[index].data());
                    ImGui::NextColumn();
                    ImGuiText("%s", event_data.last_update, button_state.pressed);
                    ImGuiText("%s", event_data.last_update, button_state.hold);
                    ImGuiText("%s", event_data.last_update, button_state.released);
                    ImGuiText("%s", event_data.last_update, button_state.clicked);
                    ImGuiText("%s (%d)", event_data.last_update, button_state.repeat, button_state.repeat);

                    index += 1;
                }
                ImGui::Columns(1);

                ImGui::TreePop();
            }

            ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
            if (ImGui::TreeNode("Keyboard"))
            {
                ImGui::Columns(6);
                ImGui::Separator();

                ImGui::Text("Key / ID");
                ImGui::NextColumn();
                ImGui::Text("Pressed");
                ImGui::NextColumn();
                ImGui::Text("Hold");
                ImGui::NextColumn();
                ImGui::Text("Released");
                ImGui::NextColumn();
                ImGui::Text("Clicked");
                ImGui::NextColumn();
                ImGui::Text("Repeat (num)");
                ImGui::NextColumn();

                for (auto const& event_entry : _events)
                {
                    InputEventEx const event_data = event_entry.value;
                    DeviceType const device_type = DeviceType{ (static_cast<uint16_t>(event_data.input.identifier) & 0xf000) >> 12 };

                    if (device_type == DeviceType::Keyboard)
                    {
                        uint16_t const key_id = static_cast<uint16_t>(event_data.input.identifier) & 0x0fff;

                        ImGui::Separator();

                        if (KeyboardKey{ key_id } >= KeyboardKey::KeyA && KeyboardKey{ key_id } <= KeyboardKey::KeyZ)
                        {
                            ImGui::Text("Key '%c'", (key_id - static_cast<uint16_t>(KeyboardKey::KeyA)) + 'A');
                        }
                        else
                        {
                            ImGui::Text("KeyID: %d", key_id);
                        }
                        ImGui::NextColumn();

                        auto const button_state = event_data.input.value.button.state;

                        ImGuiText("%s", event_data.last_update, button_state.pressed);
                        ImGuiText("%s", event_data.last_update, button_state.hold);
                        ImGuiText("%s", event_data.last_update, button_state.released);
                        ImGuiText("%s", event_data.last_update, button_state.clicked);
                        ImGuiText("%s (%d)", event_data.last_update, button_state.repeat, button_state.repeat);
                    }
                }
                ImGui::Columns(1);

                ImGui::TreePop();
            }

        }
        ImGui::End();
    }

} // namespace iceshard::debug
