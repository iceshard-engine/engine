/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/shard.hxx>
#include <ice/stringid.hxx>

namespace ice::action
{

    static constexpr ice::Shard Shard_ActionEventSuccess = "event/action/success"_shard;
    static constexpr ice::Shard Shard_ActionEventFailed = "event/action/failed"_shard;
    static constexpr ice::Shard Shard_ActionEventReset = "event/action/reset"_shard;

    struct ActionStage
    {
        ice::ShardID stage_shardid;

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

        ice::u32 stage_count;
        ice::u32 trigger_count;
        ice::action::ActionStage const* stages;
        ice::action::ActionTrigger const* triggers;
    };

} // namespace ice::action
