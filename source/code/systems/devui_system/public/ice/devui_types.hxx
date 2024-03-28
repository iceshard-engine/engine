#pragma once
#include <ice/stringid.hxx>

namespace ice
{

    class DevUIContext;
    class DevUIFrame;
    class DevUIWidget;

    struct DevUIWidgetInfo;
    struct DevUIWidgetState;

    using FnDevUIContextSetupCallback = bool(*)(
        ice::StringID_Arg context_name,
        void* native_context,
        void* userdata
    ) noexcept;

} // namespace ice
