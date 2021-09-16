#pragma once
#include <ice/stringid.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/shard_container.hxx>
#include <ice/engine_types.hxx>
#include <ice/clock.hxx>

namespace ice::action
{

    struct Action;

    class ActionSystem
    {
    public:
        virtual ~ActionSystem() noexcept = default;

        virtual void create_action(
            ice::StringID_Arg action_name,
            ice::action::Action const& action
        ) noexcept = 0;

        virtual void step_actions(
            ice::ShardContainer& shards
        ) noexcept = 0;
    };

    class ActionTriggerDatabase;

    auto create_action_system(
        ice::Allocator& alloc,
        ice::Clock const& clock,
        ice::action::ActionTriggerDatabase& triggers
    ) noexcept -> ice::UniquePtr<ice::action::ActionSystem>;

} // namespace ice::action
