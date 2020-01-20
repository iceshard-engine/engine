#pragma once
#include <debugui/debugui.hxx>
#include <imgui/imgui.h>

namespace iceshard::debug
{

    class IceshardDebugUI : public debugui::DebugUI
    {
    public:
        IceshardDebugUI(debugui::debugui_context_handle ctx) noexcept;

        void end_frame() noexcept;
    };

} // namespace iceshard::debug

