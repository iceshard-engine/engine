/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine_devui.hxx>
#include <ice/assert.hxx>

namespace ice
{

    class IceshardNoopDevUI : public ice::EngineDevUI
    {
    public:
        inline void register_widget(ice::devui::DevUIWidget* /*widget*/) noexcept override { }
        inline void unregister_widget(ice::devui::DevUIWidget* /*widget*/) noexcept override { }

        inline void internal_set_key(ice::devui::DevUIExecutionKey expected_key) noexcept override
        {
            _expected_key = expected_key;
        }

        inline void internal_build_widgets(
            ice::EngineFrame& /*frame*/,
            ice::devui::DevUIExecutionKey execution_key
        ) noexcept override
        {
            ICE_ASSERT(
                _expected_key == execution_key,
                "Method 'internal_build_widgets' was executed from an invalid context!"
            );
        }

    private:
        ice::devui::DevUIExecutionKey _expected_key;
    };

} // namespace ice
