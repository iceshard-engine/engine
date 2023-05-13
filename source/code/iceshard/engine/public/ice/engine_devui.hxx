/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>

namespace ice
{

    static constexpr ice::StringID Constant_TraitName_DevUI
        = "ice.iceshard-engine.trait-devui"_sid;

    static constexpr ice::StringID Constant_GfxStage_DevUI
        = "ice.iceshard-engine.gfx-stage-devui"_sid;

    namespace devui
    {

        class DevUIWidget;
        class DevUITrait;

        enum class DevUIExecutionKey : ice::u32;

    } // namespace devui

    class EngineFrame;

    class EngineDevUI
    {
    public:
        virtual ~EngineDevUI() noexcept = default;

        virtual void register_widget(ice::devui::DevUIWidget* widget) noexcept = 0;
        virtual void unregister_widget(ice::devui::DevUIWidget* widget) noexcept = 0;

    public:
        virtual void internal_set_key(devui::DevUIExecutionKey) noexcept = 0;
        virtual void internal_build_widgets(
            ice::EngineFrame& frame,
            ice::devui::DevUIExecutionKey
        ) noexcept = 0;
    };

} // namespace ice
