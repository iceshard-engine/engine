/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine_types.hxx>
#include <ice/shard.hxx>

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

    } // namespace devui

    class EngineDevUI
    {
    public:
        virtual ~EngineDevUI() noexcept = default;

        virtual void register_widget(ice::devui::DevUIWidget* widget) noexcept = 0;
        virtual void unregister_widget(ice::devui::DevUIWidget* widget) noexcept = 0;

        virtual void register_trait(ice::TraitArchive& archive) noexcept = 0;

        virtual void render_builtin_widgets(ice::EngineFrame& frame) noexcept = 0;
    };

    static constexpr ice::ShardID ShardID_RegisterDevUI = "action/devui/register`ice::EngineDevUI*"_shardid;

} // namespace ice

template<>
constexpr ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::EngineDevUI*> = ice::shard_payloadid("ice::EngineDevUI*");
