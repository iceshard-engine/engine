/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/stringid.hxx>

namespace ice
{

    class DevUIContext;
    class DevUIFrame;
    class DevUIWidget;

    struct DevUIWidgetInfo;
    struct DevUIWidgetState;
    struct DevUIContextSetupParams;

    using FnDevUIAlloc = void*(*)(size_t size, void* userdata) noexcept;
    using FnDevUIDealloc = void(*)(void* size, void* userdata) noexcept;

    using FnDevUIContextSetupCallback = bool(*)(
        ice::StringID_Arg context_name,
        ice::DevUIContextSetupParams const& params,
        void* userdata
    ) noexcept;

} // namespace ice
