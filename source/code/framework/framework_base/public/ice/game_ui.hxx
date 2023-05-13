/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/stringid.hxx>
#include <ice/shard.hxx>
#include <ice/ui_types.hxx>
#include <ice/ecs/ecs_entity.hxx>

namespace ice
{

    static constexpr ice::StringID Constant_TraitName_GameUI
        = "ice.base-framework.trait-game-ui"_sid;


    static constexpr ice::Shard Shard_GameUI_Load
        = "action/ui/load`char const*"_shard;

    static constexpr ice::Shard Shard_GameUI_Show
        = "action/ui/show`char const*"_shard;

    static constexpr ice::Shard Shard_GameUI_Hide
        = "action/ui/hide`char const*"_shard;

    static constexpr ice::Shard Shard_GameUI_UpdateResource
        = "action/ui/update_resource"_shard;


    static constexpr ice::Shard Shard_GameUI_Loaded
        = "event/ui/loaded`char const*"_shard;

    static constexpr ice::Shard Shard_GameUI_Updated
        = "event/ui/updated`char const*"_shard;


    struct UpdateUIResource
    {
        char const* page;
        ice::StringID resource;
        ice::ui::ResourceType resource_type;
        void const* resource_data;
    };

} // namespace ice

template<>
constexpr ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::UpdateUIResource const*> = ice::shard_payloadid("ice::UpdateUIResource const*");
