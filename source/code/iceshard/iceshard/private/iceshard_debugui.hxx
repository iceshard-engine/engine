#pragma once
#include <iceshard/debug/debug_window.hxx>
#include <imgui/imgui.h>

namespace iceshard::debug
{

    class IceshardDebugUI : public iceshard::debug::DebugWindow
    {
    public:
        IceshardDebugUI(iceshard::debug::debugui_context_handle ctx) noexcept;

        void update(core::MessageBuffer const& messages) noexcept;

        void end_frame() noexcept;

    private:
        bool _visible = false;
        bool _demo_window = false;
    };

} // namespace iceshard::debug

