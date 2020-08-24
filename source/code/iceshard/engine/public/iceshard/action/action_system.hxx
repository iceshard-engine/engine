#pragma once
#include <core/cexpr/stringid.hxx>

namespace iceshard
{

    struct ActionDefinition;

    class ActionTriggerDatabase;

    class ActionSystem
    {
    public:
        virtual ~ActionSystem() noexcept = default;

        virtual auto trigger_database() noexcept -> ActionTriggerDatabase& = 0;

        virtual auto trigger_database() const noexcept -> ActionTriggerDatabase const& = 0;

        virtual void create_action(
            core::stringid_arg_type name,
            ActionDefinition action_definition
        ) noexcept = 0;
    };

} // namespace iceshard
