#pragma once
#include <ice/shard.hxx>
#include <ice/stringid.hxx>

namespace ice::action
{

    struct ActionStage
    {
        ice::u32 success_trigger_offset;
        ice::u32 success_trigger_count;

        ice::u32 failure_trigger_offset;
        ice::u32 failure_trigger_count;

        ice::u32 reset_trigger_offset;
    };

    struct ActionTrigger
    {
        ice::StringID name;
        ice::Shard user_shard;
    };

    struct Action
    {
        ice::StringID name;

        ice::ShardID success_shardid;
        ice::ShardID failure_shardid;
        ice::ShardID reset_shardid;

        ice::u32 stage_count;
        ice::u32 trigger_count;
        ice::action::ActionStage const* stages;
        ice::action::ActionTrigger const* triggers;
    };

} // namespace ice::action
