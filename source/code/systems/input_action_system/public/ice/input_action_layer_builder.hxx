#pragma once
#include <ice/mem_unique_ptr.hxx>
#include <ice/input_action_definitions.hxx>
#include <ice/concept/pimpl_type.hxx>

namespace ice
{

    struct InputActionBuilder
    {
        //! \brief Builder for input action layer objects. Allows to define Sources and Actions.
        class Layer;

        //! \brief Builder for input action sources. Allows to define a source and associated input events.
        class Source;

        //! \brief Builder for input actions. Allows to define an action, along with it's conditions and modifiers.
        class Action;

        //! \brief Builder for input action conditions. Allows to define conditions and steps of an input action. Used
        //!   to drive the logic of an action.
        class ActionCondition;

        //! \brief Utility base class is a pimpl type.
        using BuilderBase = ice::concepts::PimplType;
    };

    //! \brief Builder object for input action layers. Allows to easily define actions and sources without
    //!   the need to understand the underlying binary representation.
    class InputActionBuilder::Layer : public BuilderBase
    {
    public:
        using BuilderBase::BuilderBase;

        virtual ~Layer() noexcept = default;

        virtual auto set_name(
            ice::String name
        ) noexcept -> ice::InputActionBuilder::Layer& = 0;

        virtual auto define_source(
            ice::String name,
            ice::InputActionSourceType type
        ) noexcept -> ice::InputActionBuilder::Source = 0;

        virtual auto define_action(
            ice::String name,
            ice::InputActionDataType type
        ) noexcept -> ice::InputActionBuilder::Action = 0;

        virtual auto finalize(
            ice::Allocator& alloc
        ) noexcept -> ice::UniquePtr<ice::InputActionLayer> = 0;
    };

    class InputActionBuilder::Source : public BuilderBase
    {
    public:
        using BuilderBase::BuilderBase;

        ~Source() noexcept = default;

        auto add_key(ice::input::KeyboardKey key) noexcept -> Source&;
        auto add_keymod(ice::input::KeyboardMod keymod) noexcept -> Source&;
        auto add_button(ice::input::MouseInput button) noexcept -> Source&;
        auto add_button(ice::input::ControllerInput button) noexcept -> Source&;
        auto add_axis(ice::input::MouseInput axisx) noexcept -> Source&;
        auto add_axis(ice::input::ControllerInput button) noexcept -> Source&;
    };

    class InputActionBuilder::Action : public BuilderBase
    {
    public:
        using BuilderBase::BuilderBase;

        ~Action() noexcept = default;

        auto set_behavior(ice::InputActionBehavior behavior) noexcept -> Action&;

        auto add_condition(
            ice::String source,
            ice::InputActionCondition condition,
            ice::InputActionConditionFlags flags = InputActionConditionFlags::None,
            ice::f32 param = 0.0f,
            bool from_action = false
        ) noexcept -> Action&;

        auto add_step(
            ice::String source,
            ice::InputActionStep step,
            ice::String target_axis = ".x"
        ) noexcept -> Action&;

        auto add_step(
            ice::InputActionStep step
        ) noexcept -> Action&;

        auto add_modifier(
            ice::InputActionModifier modifier,
            ice::f32 param,
            ice::String target_axis = ".x"
        ) noexcept -> Action&;
    };

    auto create_input_action_layer_builder(
        ice::Allocator& alloc,
        ice::String layer_name
    ) noexcept -> ice::UniquePtr<ice::InputActionBuilder::Layer>;

} // namespace ice
