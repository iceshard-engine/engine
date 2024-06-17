/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <android/input.h>
#include <ice/input/device_event_queue.hxx>
#include <ice/expected.hxx>

namespace ice::platform::android
{

    static constexpr ice::ErrorCode S_ButtonActionIgnored{ "S.1100:Android:Motion ButtonAction ignored" };
    static constexpr ice::ErrorCode S_HooverActionIgnored{ "S.1101:Android:Motion HooverAction ignored" };

    auto handle_android_motion_event(
        AInputEvent const* input_event,
        ice::input::DeviceEventQueue& out_events
    ) noexcept -> ice::Result;

} // namespace ice::platform::android
