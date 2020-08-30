#include "iceshard_debug_inputs_raw.hxx"

#include <core/pod/queue.hxx>
#include <core/string_view.hxx>

#include <imgui/imgui.h>

namespace iceshard::debug
{

    namespace detail
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

    } // namespace detail

    DebugWindow_InputsRaw::DebugWindow_InputsRaw(core::allocator& alloc, bool& open_ref) noexcept
        : DebugWindow{}
        , _open{ open_ref }
        , _messages{ alloc }
    {
        core::pod::queue::reserve(_messages, 1000);
    }

    void DebugWindow_InputsRaw::update(Frame const& frame) noexcept
    {
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

    void DebugWindow_InputsRaw::end_frame() noexcept
    {
        if (_open == false)
        {
            return;
        }

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

        if (ImGui::Begin("Inputs (raw)", &_open))
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
                            ImGui::Text("Message %s", detail::device_message_type_names[type].data());
                        }
                    });

                if (scroll_track)
                {
                    ImGui::SetScrollHereY(1.0f);
                }
            }
            ImGui::EndChild();
        }
        ImGui::End();
    }

} // namespace iceshard::debug
