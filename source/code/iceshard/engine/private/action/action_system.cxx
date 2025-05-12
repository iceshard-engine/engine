/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/action/action.hxx>
#include <ice/action/action_trigger.hxx>
#include <ice/action/action_system.hxx>
#include <ice/engine_frame.hxx>

#include <ice/mem_allocator.hxx>
#include <ice/mem_allocator_ring.hxx>
#include <ice/container/array.hxx>
#include <ice/clock.hxx>

namespace ice::action
{

    enum class ActionState : ice::u32
    {
        Idle,
        Active,
        FinishedSuccess,
        FinishedFailure,
    };

    struct ActionTriggerInstance
    {
        ice::Shard user_shard;
        ice::ShardID trigger_shardid;
        ice::action::ActionTriggerHandler* handler;
    };

    struct ActionInstance
    {
        ice::Memory mem;
        ice::StringID name;

        ice::u32 stage_count;
        ice::u32 trigger_count;

        ice::u32 current_stage_idx;
        ice::u32 current_failure_trigger_idx;
        ice::u32 current_success_trigger_idx;

        ice::Timeline stage_timeline;

        ice::action::ActionState state;
        ice::action::ActionStage const* stages;
        ice::action::ActionTriggerInstance const* trigger_instances;
    };

    auto create_action_instance(
        ice::Allocator& alloc,
        ice::action::ActionTriggerDatabase& triggers,
        ice::action::Action const& action
    ) noexcept -> ice::action::ActionInstance*
    {
        bool has_all_triggers = true;
        for (ice::u32 idx = 0; idx < action.trigger_count; ++idx)
        {
            has_all_triggers &= triggers.get_trigger(action.triggers[idx].name).trigger_shardid != ice::Shard_Invalid;
        }

        if (has_all_triggers == false)
        {
            return nullptr;
        }

        ice::meminfo action_meminfo = ice::meminfo_of<ActionInstance>;
        ice::usize const stages_offset = action_meminfo += ice::meminfo_of<ActionStage> * action.stage_count;
        ice::usize const triggers_offset = action_meminfo += ice::meminfo_of<ActionTriggerInstance> * action.trigger_count;

        ice::AllocResult const result = alloc.allocate(action_meminfo);

        ActionInstance* action_instance = reinterpret_cast<ActionInstance*>(result.memory);
        ActionStage* stages = reinterpret_cast<ActionStage*>(ice::ptr_add(result.memory, stages_offset));
        ActionTriggerInstance* trigger_instances = reinterpret_cast<ActionTriggerInstance*>(ice::ptr_add(result.memory, triggers_offset));

        action_instance->mem = result;
        action_instance->name = action.name;

        action_instance->stage_count = action.stage_count;
        action_instance->trigger_count = action.trigger_count;

        action_instance->current_stage_idx = 0;
        action_instance->current_failure_trigger_idx = 0;
        action_instance->current_success_trigger_idx = 0;

        action_instance->state = ActionState::Active;
        action_instance->stages = stages;
        action_instance->trigger_instances = trigger_instances;

        for (ice::u32 idx = 0; idx < action.stage_count; ++idx)
        {
            stages[idx] = action.stages[idx];
        }

        for (ice::u32 idx = 0; idx < action.trigger_count; ++idx)
        {
            ActionTriggerDefinition trigger_def = triggers.get_trigger(action.triggers[idx].name);
            trigger_instances[idx].user_shard = action.triggers[idx].user_shard;
            trigger_instances[idx].handler = trigger_def.trigger_handler;
            trigger_instances[idx].trigger_shardid = trigger_def.trigger_shardid;
        }

        return action_instance;
    }

    class SimpleActionSystem : public ice::action::ActionSystem
    {
    public:
        SimpleActionSystem(
            ice::Allocator& alloc,
            ice::Clock const& clock,
            ice::action::ActionTriggerDatabase& trigger_database
        ) noexcept;
        ~SimpleActionSystem() noexcept override;

        void create_action(
            ice::StringID_Arg action_name,
            ice::action::Action const& action
        ) noexcept override;

        void step_actions(
            ice::ShardContainer& shards
        ) noexcept override;

    private:
        ice::Allocator& _allocator;
        ice::RingAllocator _step_shards_alloc;
        ice::Clock const& _clock;
        ice::action::ActionTriggerDatabase& _trigger_database;
        ice::Array<ice::action::ActionInstance*> _actions;

    };

    SimpleActionSystem::SimpleActionSystem(
        ice::Allocator& alloc,
        ice::Clock const& clock,
        ice::action::ActionTriggerDatabase& trigger_database
    ) noexcept
        : _allocator{ alloc }
        , _step_shards_alloc{ alloc, { .ring_buffer_size = ice::size_of<ice::Shard> * 128 } }
        , _clock{ clock }
        , _trigger_database{ trigger_database }
        , _actions{ alloc }
    {
    }

    SimpleActionSystem::~SimpleActionSystem() noexcept
    {
        for (ActionInstance* action : _actions)
        {
            _allocator.deallocate(action->mem);
        }
    }

    void SimpleActionSystem::create_action(
        ice::StringID_Arg action_name,
        ice::action::Action const& action
    ) noexcept
    {
        ActionInstance* instance = create_action_instance(_allocator, _trigger_database, action);
        if (instance != nullptr)
        {
            instance->stage_timeline = ice::timeline::create_timeline(_clock);
            ice::array::push_back(
                _actions,
                instance
            );
        }
    }

    void SimpleActionSystem::step_actions(
        ice::ShardContainer& shards
    ) noexcept
    {
        ice::ShardContainer new_shards{ _step_shards_alloc };
        ice::shards::reserve(new_shards, 64);

        for (ActionInstance* const action : _actions)
        {
            if (action->state == ActionState::Active)
            {
                continue;
            }
            else if (action->state != ActionState::Idle)
            {
                action->stage_timeline = ice::timeline::create_timeline(_clock);
            }

            action->state = ActionState::Idle;

            ActionStage const* const stages = action->stages;
            ActionTriggerInstance const* const triggers = action->trigger_instances;

            ActionStage const* current_stage = stages + action->current_stage_idx;
            ActionTriggerInstance const* reset_trigger = triggers + current_stage->reset_trigger_offset;

            for (ice::Shard const shard : shards)
            {
                if (reset_trigger->handler(reset_trigger->user_shard, shard, ice::timeline::elapsed(action->stage_timeline)))
                {
                    action->current_stage_idx = 0;
                    action->current_failure_trigger_idx = 0;
                    action->current_success_trigger_idx = 0;
                    action->state = ActionState::Active;
                    break;
                }
            }

            if (action->state == ActionState::Active)
            {
                ice::StringID_Hash action_name = ice::stringid_hash(action->name);
                ice::shards::push_back(new_shards, ice::action::Shard_ActionEventReset | action_name);

                current_stage = stages + action->current_stage_idx;
                if (current_stage->stage_shardid != ice::Shard_Invalid)
                {
                    ice::shards::push_back(new_shards, ice::shard(current_stage->stage_shardid) | action_name);
                }
            }
        }

        for (ActionInstance* const action : _actions)
        {
            if (action->state != ActionState::Active)
            {
                continue;
            }

            ActionStage const* const stages = action->stages;
            ActionTriggerInstance const* const triggers = action->trigger_instances;

            ActionStage const* current_stage = stages + action->current_stage_idx;
            ActionTriggerInstance const* failure_trigger = triggers + current_stage->failure_trigger_offset + action->current_failure_trigger_idx;
            ActionTriggerInstance const* success_trigger = triggers + current_stage->success_trigger_offset + action->current_success_trigger_idx;

            ice::Tns const stage_elapsed_time = ice::timeline::elapsed(action->stage_timeline);

            for (ice::Shard const shard : shards)
            {
                if (current_stage->failure_trigger_count > 0 && shard == failure_trigger->trigger_shardid)
                {
                    if (failure_trigger->handler(failure_trigger->user_shard, shard, stage_elapsed_time))
                    {
                        action->current_failure_trigger_idx += 1;
                        if (action->current_failure_trigger_idx == current_stage->failure_trigger_count)
                        {
                            action->state = ActionState::FinishedFailure;
                        }
                        else
                        {
                            failure_trigger += 1;
                        }
                    }
                }

                if (action->state == ActionState::Active && shard == success_trigger->trigger_shardid)
                {
                    if (success_trigger->handler(success_trigger->user_shard, shard, stage_elapsed_time))
                    {
                        action->current_success_trigger_idx += 1;
                        if (action->current_success_trigger_idx == current_stage->success_trigger_count)
                        {
                            if ((action->current_stage_idx + 1) == action->stage_count)
                            {
                                action->state = ActionState::FinishedSuccess;
                            }
                            else
                            {
                                action->current_stage_idx += 1;
                                action->current_failure_trigger_idx = 0;
                                action->current_success_trigger_idx = 0;
                                action->stage_timeline = ice::timeline::create_timeline(_clock);

                                current_stage += 1;
                                failure_trigger = triggers + current_stage->failure_trigger_offset + action->current_failure_trigger_idx;
                                success_trigger = triggers + current_stage->success_trigger_offset + action->current_success_trigger_idx;

                                if (current_stage->stage_shardid != ice::Shard_Invalid)
                                {
                                    ice::shards::push_back(new_shards, ice::shard(current_stage->stage_shardid) | ice::stringid_hash(action->name));
                                }
                            }
                        }
                        else
                        {
                            success_trigger += 1;
                        }
                    }
                }
            }
        }

        for (ActionInstance* const action : _actions)
        {
            if (action->state == ActionState::FinishedSuccess)
            {
                ice::shards::push_back(new_shards, ice::action::Shard_ActionEventSuccess | ice::stringid_hash(action->name));
            }
            else if (action->state == ActionState::FinishedFailure)
            {
                ice::shards::push_back(new_shards, ice::action::Shard_ActionEventFailed | ice::stringid_hash(action->name));
            }
        }

        ice::shards::push_back(shards, new_shards._data);
    }

    auto create_action_system(
        ice::Allocator& alloc,
        ice::Clock const& clock,
        ice::action::ActionTriggerDatabase& triggers
    ) noexcept -> ice::UniquePtr<ice::action::ActionSystem>
    {
        return ice::make_unique<ice::action::SimpleActionSystem>(alloc, alloc, clock, triggers);
    }

} // namespace ice::action
;
