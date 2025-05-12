/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/shard.hxx>
#include <ice/string_types.hxx>
#include <ice/clock_types.hxx>

namespace ice
{

    template<>
    constexpr inline ice::ShardPayloadID Constant_ShardPayloadID<ice::String const*> = ice::shard_payloadid("ice::String const*");

    template<>
    constexpr inline ice::ShardPayloadID Constant_ShardPayloadID<ice::Tns> = ice::shard_payloadid("ice::Tns");

} // namespace ice
