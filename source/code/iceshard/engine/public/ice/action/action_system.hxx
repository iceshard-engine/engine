#pragma once
#include <ice/stringid.hxx>

namespace ice::action
{

    struct ActionDefinition;

    class ActionSystem
    {
    public:
        virtual ~ActionSystem() noexcept = default;

        virtual void create_action(
            ice::StringID_Arg action_name,
            ice::action::ActionDefinition const& action_definition
        ) noexcept = 0;
    };

} // namespace ice::action
