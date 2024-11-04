/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/shard.hxx>
#include <ice/shard_payloads.hxx>

namespace ice
{

    static constexpr ice::Shard Shard_FrameTick = "event/frame/tick"_shard;

    static constexpr ice::ShardID ShardID_WorldCreated = "event/world/created`ice::StringID_Hash"_shardid;
    static constexpr ice::ShardID ShardID_WorldDestroyed = "event/world/destroyed`ice::StringID_Hash"_shardid;

    static constexpr ice::ShardID ShardID_WorldActivate = "action/world/activate"_shardid;
    static constexpr ice::ShardID ShardID_WorldDeactivate = "action/world/deactivate"_shardid;

    static constexpr ice::ShardID ShardID_WorldActivated = "event/world/activated`ice::StringID_Hash"_shardid;
    static constexpr ice::ShardID ShardID_WorldDeactivated = "event/world/deactivated`ice::StringID_Hash"_shardid;

    static constexpr ice::ShardID ShardID_InputEvent = "event/input`ice::input::InputEvent"_shardid;

} // namespace ice
