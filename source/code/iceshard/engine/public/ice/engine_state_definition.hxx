#pragma once
#include <ice/engine_state.hxx>
#include <ice/shard_container.hxx>
#include <ice/shard.hxx>

namespace ice
{

    struct EngineStateTrigger
    {
        ice::ShardID when;
        ice::EngineState before;
        ice::EngineState from;
        ice::EngineState to;
        ice::ShardID results = ice::Shard_Invalid;
    };

    struct EngineStateCommitter
    {
        virtual ~EngineStateCommitter() noexcept = default;

        virtual bool commit(
            ice::EngineStateTrigger const& trigger,
            ice::Shard trigger_shard,
            ice::ShardContainer& out_shards
        ) noexcept = 0;
    };

} // namespace ice
