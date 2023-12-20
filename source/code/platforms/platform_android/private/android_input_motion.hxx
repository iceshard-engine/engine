/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <android/input.h>
#include <ice/result_codes.hxx>
#include <ice/input/device_event_queue.hxx>

namespace ice::platform::android
{

    static constexpr ice::ResCode I_ButtonActionIgnored = ice::ResCode::create(ResultSeverity::Info, "Motion ButtonAction ignored");
    static constexpr ice::ResCode I_HooverActionIgnored = ice::ResCode::create(ResultSeverity::Info, "Motion HooverAction ignored");

    auto handle_android_motion_event(
        AInputEvent const* input_event,
        ice::input::DeviceEventQueue& out_events
    ) noexcept -> ice::Result;

} // namespace ice::platform::android
