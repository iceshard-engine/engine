/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/devui_types.hxx>

namespace ice
{

    struct DevUIWidgetState
    {
        DevUIWidgetState const* const owner = nullptr;
        bool active = false;
    };

    class DevUIFrame
    {
    public:
        virtual ~DevUIFrame() noexcept = default;

        virtual void mainmenu(ice::DevUIWidget const& widget, ice::DevUIWidgetState& state) noexcept = 0;

        virtual bool begin(ice::DevUIWidget const& widget, ice::DevUIWidgetState& state) noexcept = 0;
        virtual void end() noexcept = 0;
    };

} // namespace ice
