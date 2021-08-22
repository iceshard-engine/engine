#pragma once
#include <ice/shard.hxx>
#include <ice/stringid.hxx>

namespace ice::action
{

    struct ActionTriggerDefinition
    {
        ice::StringID name;
        ice::Shard user_shard;
    };

    struct ActionStageDefinition
    {
        ice::ShardID success_shardid;
        ice::ShardID failure_shardid;
        ice::ShardID reset_shardid;

        ice::u32 success_trigger_offset;
        ice::u32 success_trigger_count;

        ice::u32 failure_trigger_offset;
        ice::u32 failure_trigger_count;

        ice::u32 reset_trigger_offset;
    };

    struct ActionDefinition
    {
        ice::StringID name;

        ice::u32 stage_count;
        ice::u32 trigger_count;
        ice::action::ActionStageDefinition const* stages;
        ice::action::ActionTriggerDefinition const* triggers;
    };

} // namespace ice::action
