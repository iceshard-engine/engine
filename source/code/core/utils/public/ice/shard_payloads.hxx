#pragma once
#include <ice/shard.hxx>
#include <ice/string_types.hxx>

namespace ice
{

    template<>
    constexpr ice::ShardPayloadID Constant_ShardPayloadID<ice::String const*> = ice::shard_payloadid("ice::String const*");

} // namespace ice
