/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/shard.hxx>

namespace ice
{

    static constexpr ice::Shard Shard_FrameTick = "event/frame/tick"_shard;

    static constexpr ice::ShardID ShardID_WorldActivate = "action/world/activate`ice::StringID_Hash"_shardid;
    static constexpr ice::ShardID ShardID_WorldDeactivate = "action/world/deactivate`ice::StringID_Hash"_shardid;

    static constexpr ice::ShardID ShardID_WorldActivated = "event/world/activated`ice::StringID_Hash"_shardid;
    static constexpr ice::ShardID ShardID_WorldDeactivated = "event/world/deactivated`ice::StringID_Hash"_shardid;

    static constexpr ice::ShardID ShardID_InputEvent = "event/input`ice::input::InputEvent"_shardid;

    //static constexpr ice::Shard Shard_InputEventButton = "event/input/button`ice::input::InputEvent"_shard;
    //static constexpr ice::Shard Shard_InputEventAxis = "event/input/axis`ice::input::InputEvent"_shard;

} // namespace ice
