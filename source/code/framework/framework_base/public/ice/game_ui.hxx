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
        = "action/ui/load`ice::c8utf const*"_shard;

    static constexpr ice::Shard Shard_GameUI_Show
        = "action/ui/show`ice::c8utf const*"_shard;

    static constexpr ice::Shard Shard_GameUI_Hide
        = "action/ui/hide`ice::c8utf const*"_shard;

    static constexpr ice::Shard Shard_GameUI_UpdateResource
        = "action/ui/update_resource"_shard;


    static constexpr ice::Shard Shard_GameUI_Loaded
        = "event/ui/loaded`ice::c8utf const*"_shard;

    static constexpr ice::Shard Shard_GameUI_Updated
        = "event/ui/updated`ice::c8utf const*"_shard;


    struct UpdateUIResource
    {
        ice::c8utf const* page;
        ice::StringID resource;
        ice::ui::ResourceType resource_type;
        void const* resource_data;
    };

} // namespace ice

template<>
constexpr ice::PayloadID ice::detail::Constant_ShardPayloadID<ice::UpdateUIResource const*> = ice::payload_id("ice::UpdateUIResource const*");
