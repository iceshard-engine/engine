#pragma once
#include <iceshard/debug/debug_window.hxx>
#include <imgui/imgui.h>

namespace iceshard::debug
{

    class IceshardDebugUI : public iceshard::debug::DebugWindow
    {
    public:
        void update(iceshard::input::DeviceInputQueue const& inputs) noexcept override;

        void end_frame() noexcept override;

    private:
        bool _visible = false;
        bool _demo_window = false;
    };

} // namespace iceshard::debug

