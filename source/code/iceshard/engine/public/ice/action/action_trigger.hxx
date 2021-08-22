#pragma once
#include <ice/shard.hxx>
#include <ice/stringid.hxx>
#include <ice/unique_ptr.hxx>

namespace ice::action
{

    using ActionTriggerHandler = bool(ice::Shard const& user_shard, ice::Shard const& event_shard, ice::f32 stage_time_elapsed) noexcept;

    struct ActionTriggerDefinition
    {
        ice::ShardID trigger_shardid;
        ice::action::ActionTriggerHandler* trigger_handler;
    };

    class ActionTriggerDatabase
    {
    public:
        virtual ~ActionTriggerDatabase() noexcept = default;

        virtual void add_trigger(
            ice::StringID_Arg name,
            ice::action::ActionTriggerDefinition trigger_info
        ) noexcept = 0;

        virtual auto get_trigger(
            ice::StringID_Arg name
        ) const noexcept -> ice::action::ActionTriggerDefinition = 0;
    };

    auto create_trigger_database(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::action::ActionTriggerDatabase>;

    void setup_common_triggers(
        ice::action::ActionTriggerDatabase& database
    ) noexcept;

} // namespace ice::action