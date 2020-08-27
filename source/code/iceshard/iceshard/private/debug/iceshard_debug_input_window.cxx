#include "iceshard_debug_input_window.hxx"

#include <iceshard/input/input_mouse.hxx>
#include <iceshard/input/input_keyboard.hxx>
#include <iceshard/input/input_controller.hxx>

#include <core/string_view.hxx>
#include <core/pod/queue.hxx>
#include <core/pod/hash.hxx>
#include <imgui/imgui.h>

namespace iceshard::debug
{

    static core::StringView device_message_type_names[] = {
        "Invalid",
        "Device Connected",
        "Device Disconnected",
        "Mouse Position",
        "Mouse Button Down",
        "Mouse Button Up",
        "Mouse Wheel",
        "Keyboard Button Down",
        "Keyboard Button Up",
        "Keyboard Modifier Down",
        "Keyboard Modifier Up",
        "Gamepad Button Down",
        "Gamepad Button Up",
        "Gamepad Trigger Left",
        "Gamepad Trigger Right",
        "Gamepad Axis Left XY",
        "Gamepad Axis Left X",
        "Gamepad Axis Left Y",
        "Gamepad Axis Right XY",
        "Gamepad Axis Right X",
        "Gamepad Axis Right Y",
    };

    RawInputsWindow::RawInputsWindow(core::allocator& alloc) noexcept
        : DebugWindow{ }
        , _allocator{ alloc }
        , _messages{ alloc }
        , _events{ alloc }
    {
        core::pod::queue::reserve(_messages, 20000);
    }

    RawInputsWindow::~RawInputsWindow() noexcept
    {
    }

    void RawInputsWindow::update(iceshard::Frame const& frame) noexcept
    {
        _current_frame += 1;

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

        auto const& input_queue = frame.input_queue();
        if (input_queue.empty())
        {
            return;
        }

        input_queue.for_each([this](iceshard::input::DeviceInputMessage const msg, void const* data) noexcept
            {
                if (core::pod::queue::space(_messages) == 0)
                {
                    core::pod::queue::pop_front(_messages);
                }
                core::pod::queue::push_back(_messages, msg);
            });
    }

    void RawInputsWindow::end_frame() noexcept
    {
        using namespace iceshard::input;

        static bool filter_states[] = {
            true, /* device connections */
            false, /* mouse position events */
            true, /* mouse button events */
            true, /* keyboard key events */
            true, /* keyboard modifier events */
            true, /* left axis events */
            true, /* right axis events */
            true, /* trigger events */
            true, /* button events */
        };

        static bool scroll_track = true;

        static bool filter_event[DeviceInputTypeCount]{ };
        static bool filter_event_initialized = [](auto& events) noexcept -> bool
        {
            for (auto& event : events)
            {
                event = true;
            }
            filter_event[(uint32_t)DeviceInputType::MousePosition] = false;
            return true;
        }(filter_event);

        if (ImGui::Begin("Inputs (raw)", _visible + 0))
        {
            if (ImGui::TreeNode("Filters"))
            {
                ImGui::Columns(2);

                {
                    ImGui::Text("General events");
                    if (ImGui::Checkbox("Connection events", filter_states + 0))
                    {
                        filter_event[(uint32_t)DeviceInputType::DeviceConnected] = filter_states[0];
                        filter_event[(uint32_t)DeviceInputType::DeviceDisconnected] = filter_states[0];
                    }
                    ImGui::NextColumn();
                }
                {
                    ImGui::Text("Mouse events");
                    if (ImGui::Checkbox("Position events", filter_states + 1))
                    {
                        filter_event[(uint32_t)DeviceInputType::MousePosition] = filter_states[1];
                    }
                    if (ImGui::Checkbox("Button events##Mouse", filter_states + 2))
                    {
                        filter_event[(uint32_t)DeviceInputType::MouseButtonDown] = filter_states[2];
                        filter_event[(uint32_t)DeviceInputType::MouseButtonUp] = filter_states[2];
                        filter_event[(uint32_t)DeviceInputType::MouseWheel] = filter_states[2];
                    }
                    ImGui::NextColumn();
                }

                ImGui::Columns(2);
                ImGui::Separator();

                {
                    ImGui::Text("Keyboard events");
                    if (ImGui::Checkbox("Key events", filter_states + 3))
                    {
                        filter_event[(uint32_t)DeviceInputType::KeyboardButtonDown] = filter_states[3];
                        filter_event[(uint32_t)DeviceInputType::KeyboardButtonUp] = filter_states[3];
                    }
                    if (ImGui::Checkbox("Modifier events", filter_states + 4))
                    {
                        filter_event[(uint32_t)DeviceInputType::KeyboardModifierDown] = filter_states[4];
                        filter_event[(uint32_t)DeviceInputType::KeyboardModifierUp] = filter_states[4];
                    }
                    ImGui::NextColumn();
                }
                {
                    ImGui::Text("Gamepad events");
                    if (ImGui::Checkbox("Left Axis events", filter_states + 5))
                    {
                        filter_event[(uint32_t)DeviceInputType::GamepadAxisLeft] = filter_states[5];
                        filter_event[(uint32_t)DeviceInputType::GamepadAxisLeftX] = filter_states[5];
                        filter_event[(uint32_t)DeviceInputType::GamepadAxisLeftY] = filter_states[5];
                    }
                    if (ImGui::Checkbox("Right Axis events", filter_states + 6))
                    {
                        filter_event[(uint32_t)DeviceInputType::GamepadAxisRight] = filter_states[6];
                        filter_event[(uint32_t)DeviceInputType::GamepadAxisRightX] = filter_states[6];
                        filter_event[(uint32_t)DeviceInputType::GamepadAxisRightY] = filter_states[6];
                    }
                    if (ImGui::Checkbox("Trigger events", filter_states + 7))
                    {
                        filter_event[(uint32_t)DeviceInputType::GamepadTriggerLeft] = filter_states[7];
                        filter_event[(uint32_t)DeviceInputType::GamepadTriggerRight] = filter_states[7];
                    }
                    if (ImGui::Checkbox("Button events##Gamepad", filter_states + 8))
                    {
                        filter_event[(uint32_t)DeviceInputType::GamepadButtonDown] = filter_states[8];
                        filter_event[(uint32_t)DeviceInputType::GamepadButtonUp] = filter_states[8];
                    }
                    ImGui::NextColumn();
                }

                ImGui::Columns(1);

                ImGui::TreePop();
                ImGui::Separator();
            }
            if (ImGui::Button("Clear"))
            {
                core::pod::queue::consume(_messages, core::pod::queue::size(_messages));
            }
            ImGui::Checkbox("Track", &scroll_track);
            ImGui::Separator();
            ImGui::BeginChild("##message_window");
            {
                core::pod::queue::for_each(_messages, [](DeviceInputMessage msg) noexcept
                    {
                        int32_t type = static_cast<uint8_t>(msg.input_type);
                        if (filter_event[type])
                        {
                            ImGui::Text("Message %s", device_message_type_names[type].data());
                        }
                    });

                if (scroll_track)
                {
                    ImGui::SetScrollHereY(1.0f);
                }
            }
            ImGui::EndChild();

            ImGui::End();
        }

        core::StringView const NoYes_String[] = { "No", "Yes" };

        if (ImGui::Begin("Input states", _visible + 1))
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

            ImGui::End();
        }
    }

    void RawInputsWindow::show() noexcept
    {
        _visible[0] = true;
        _visible[1] = true;
    }

} // namespace iceshard::debug
