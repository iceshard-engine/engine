/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/devui_widgets.hxx>

namespace ice
{

    bool colorpicker_oklch(
        ice::String label,
        ice::Color& inout_color,
        ImGui::OkLCHPickerFlags flags = ImGui::OkLCHPickerFlags::None,
        ice::Color const* ref_color = nullptr,
        ice::f32 max_chroma = 0.32f
    ) noexcept;

    bool colorbutton_oklch(
        ice::String label,
        ice::Color& inout_color,
        ImGui::OkLCHPickerFlags flags = ImGui::OkLCHPickerFlags::None,
        ice::Color const* ref_color = nullptr,
        ice::f32 max_chroma = 0.32f
    ) noexcept;


} // namespace ice
