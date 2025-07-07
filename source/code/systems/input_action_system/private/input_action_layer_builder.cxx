#include "input_action_internal_types.hxx"
#include <ice/input_action_layer_builder.hxx>
#include <ice/input_action_layer.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/sort.hxx>

namespace ice
{

    struct InputActionLayerBuilder::SourceBuilder::Internal
    {
        Internal(ice::Allocator& alloc, ice::String name, ice::InputActionSourceType type) noexcept
            : name{ alloc, name }
            , type{ type }
            , events{ alloc }
        {
            ice::hashmap::reserve(events, 2);
        }

        ice::HeapString<> name;
        ice::InputActionSourceType type;
        ice::HashMap<ice::input::InputID> events;
    };

    struct ActionBuilderStep
    {
        ice::InputActionStep step;
        ice::HeapString<> source;

        //! \brief Axis to 'source[axis_to_read]' and 'target[axis_to_write]'. [to_read, to_write]
        ice::arr<2, ice::u8> axis;

        //! \brief Contains info the the step source should be taken from an action instead.
        bool from_action = false;
    };

    struct ActionBuilderCondition
    {
        ActionBuilderCondition(
            ice::Allocator& alloc,
            ice::String name,
            ice::InputActionCondition condition,
            ice::InputActionConditionFlags flags,
            ice::f32 param,
            bool from_action
        ) noexcept
            : condition{ condition }
            , flags{ flags }
            , source{ alloc, name }
            , steps{ alloc }
            , param{ param }
            , from_action{ from_action }
        { }

        //! \brief The check to be performed.
        ice::InputActionCondition condition;

        //! \brief Flags that may affect how conditions are executed.
        ice::InputActionConditionFlags flags;

        //! \brief The name of the source which values to use.
        ice::HeapString<> source;

        //! \brief Steps to be executed.
        ice::Array<ActionBuilderStep> steps;

        ice::f32 param;

        //! \brief Contains info the the step source should be taken from an action instead.
        bool from_action = false;
    };

    using ActionBuilderModifier = InputActionModifierData;

    struct InputActionLayerBuilder::ActionBuilder::Internal
    {
        Internal(
            ice::Allocator& alloc,
            ice::String name,
            ice::InputActionData presentation
        ) noexcept
            : allocator{ alloc }
            , name{ alloc, name }
            , presentation{ presentation }
            , conditions{ alloc }
            , modifiers{ alloc }
        {
            ice::array::reserve(conditions, 4);
            ice::array::reserve(modifiers, 2);
        }

        ice::Allocator& allocator;
        ice::HeapString<> name;
        ice::InputActionData presentation;
        ice::InputActionBehavior behavior;
        ice::Array<ice::ActionBuilderCondition> conditions;
        ice::Array<ice::ActionBuilderModifier> modifiers;

        ice::ActionBuilderCondition* current_condition;
    };

    class SimpleInputActionLayerBuilder : public ice::InputActionLayerBuilder
    {
    public:
        SimpleInputActionLayerBuilder(ice::Allocator& alloc) noexcept
            : _allocator{ alloc }
            , _sources{ alloc }
            , _actions{ alloc }
        {
            ice::hashmap::reserve(_sources, 16);
            ice::hashmap::reserve(_actions, 10);
        }

        auto define_source(ice::String name, ice::InputActionSourceType type) noexcept -> SourceBuilder override
        {
            ice::hashmap::set(_sources, ice::hash(name), {_allocator, name, type});
            return { ice::hashmap::try_get(_sources, ice::hash(name)) };
        }

        auto define_action(
            ice::String name,
            ice::InputActionData presentation
        ) noexcept -> ActionBuilder override
        {
            ice::hashmap::set(_actions, ice::hash(name), {_allocator, name, presentation});
            return { ice::hashmap::try_get(_actions, ice::hash(name)) };
        }

        auto finalize(ice::Allocator& alloc) noexcept -> ice::UniquePtr<ice::InputActionLayer> override
        {
            ice::u16 count_storage_values = 0;

            ice::HeapString<> strings{ _allocator };
            ice::Array<ice::InputActionSourceEntryInfo> final_sources{ _allocator };
            ice::Array<ice::InputActionStepData> final_steps{ _allocator };
            ice::Array<ice::InputActionConditionData> final_conditions{ _allocator };
            ice::Array<ice::InputActionModifierData> final_modifiers{ _allocator };
            ice::Array<ice::InputActionInfo> final_actions{ _allocator };

            // Prepare data of all sources
            for (SourceBuilder::Internal const& source : _sources)
            {
                for (ice::input::InputID input_event : source.events)
                {
                    ice::array::push_back(final_sources,
                        InputActionSourceEntryInfo{
                            .name = { ice::u16(ice::size(strings)), ice::u16(ice::size(source.name)) },
                            .input = input_event,
                            .type = source.type,
                            .storage = count_storage_values,
                            .param = 0.0f,
                        }
                    );
                }
                ice::string::push_back(strings, source.name);
                // ice::string::push_back(strings, '\0');

                switch(source.type)
                {
                    using enum InputActionSourceType;
                    case Key:
                    case Button:
                    case Trigger:
                    case Axis1d: count_storage_values += 1; break;
                    case Axis2d: count_storage_values += 2; break;
                    default: ICE_ASSERT_CORE(false); break;
                }
            }

            auto find_source_storage_index = [&strings, &final_sources](ice::String source_name) noexcept -> ice::u16
            {
                ice::ucount idx_found = ice::ucount_max;
                bool const found = ice::search(
                    ice::array::slice(final_sources),
                    source_name,
                    [&strings](ice::InputActionSourceEntryInfo const& source, ice::String expected) noexcept
                    {
                        ice::String const source_name = ice::string::substr(
                            strings, source.name.offset, source.name.size
                        );
                        return expected == source_name;
                    },
                    idx_found
                );

                ICE_ASSERT_CORE(found && idx_found < ice::array::count(final_sources));
                ice::u16 const source_storage_idx = final_sources[idx_found].storage;
                return source_storage_idx;
            };

            auto find_action_storage_index = [&strings, &final_actions](ice::String source_name) noexcept -> ice::u16
            {
                ice::ucount idx_found = ice::ucount_max;
                bool const found = ice::search(
                    ice::array::slice(final_actions),
                    source_name,
                    [&strings](ice::InputActionInfo const& action, ice::String expected) noexcept
                    {
                        ice::String const source_name = ice::string::substr(
                            strings, action.name_offset, action.name_length
                        );
                        return expected == source_name;
                    },
                    idx_found
                );

                ICE_ASSERT_CORE(found && idx_found < ice::array::count(final_actions));
                return ice::u16(idx_found);
            };

            // Prepare data of all actions
            ice::u16 step_offset = 0, step_count = 0;
            ice::u16 condition_offset = 0, condition_count = 0;
            ice::u8 modifier_offset = 0, modifier_count = 0;
            for (ActionBuilder::Internal const& action : _actions)
            {
                for (ActionBuilderCondition const& condition : action.conditions)
                {
                    for (ActionBuilderStep const& step : condition.steps)
                    {
                        if (step.step < InputActionStep::Set)
                        {
                            ice::array::push_back(final_steps,
                                InputActionStepData{
                                    .source = { 0, 0 },
                                    .id = step.step,
                                    .dst_axis = 0
                                }
                            );
                        }
                        else
                        {
                            ice::array::push_back(final_steps,
                                InputActionStepData{
                                    .source = {
                                        .source_index = find_source_storage_index(step.source),
                                        .source_axis = step.axis.x
                                    },
                                    .id = step.step,
                                    .dst_axis = step.axis.y
                                }
                            );
                        }

                        step_count += 1;
                    }

                    ICE_ASSERT_CORE(action.presentation != InputActionData::Invalid);
                    ice::InputActionIndex source_index = {.source_index = 0,.source_axis = 0};
                    if (condition.from_action)
                    {
                        // If we are empty, it's a "self reference"
                        if (ice::string::any(condition.source))
                        {
                            source_index.source_index = find_action_storage_index(condition.source);
                            source_index.source_axis = ice::u8(static_cast<ice::u8>(action.presentation) - 1);
                        }
                        else
                        {
                            source_index.source_index = InputActionIndex::SelfIndex;
                        }
                    }
                    else /*if (condition.condition != InputActionCondition::AlwaysTrue
                        && condition.condition != InputActionCondition::ActionActive
                        && condition.condition != InputActionCondition::ActionInactive)*/
                    {
                        source_index.source_index = find_source_storage_index(condition.source);
                        source_index.source_axis = ice::u8(static_cast<ice::u8>(action.presentation) - 1);
                    }

                    ice::array::push_back(final_conditions,
                        InputActionConditionData{
                            .source = source_index,
                            .id = condition.condition,
                            .flags = condition.flags,
                            .steps = { step_offset, step_count },
                            .param = condition.param
                        }
                    );

                    step_offset += ice::exchange(step_count, ice::u16_0);
                    condition_count += 1;
                }

                modifier_count = ice::u8(ice::count(action.modifiers));
                ice::array::push_back(final_modifiers, action.modifiers);

                ice::array::push_back(final_actions,
                    InputActionInfo{
                        .name_offset = ice::u16(ice::size(strings)),
                        .name_length = ice::u16(ice::size(action.name)),
                        .presentation = action.presentation,
                        .behavior = action.behavior,
                        .conditions = { condition_offset, condition_count },
                        .mods = { modifier_offset, modifier_count }
                    }
                );

                ice::string::push_back(strings, action.name);
                // ice::string::push_back(strings, '\0');

                modifier_offset += modifier_count;
                condition_offset += ice::exchange(condition_count, ice::u16_0);
            }

            ice::InputActionLayerInfo final_info{
                .count_sources = ice::u16(ice::count(final_sources)),
                .count_actions = ice::u16(ice::count(final_actions)),
                .count_conditions = ice::u16(ice::count(final_conditions)),
                .count_steps = ice::u16(ice::count(final_steps)),
                .count_modifiers = ice::u16(ice::count(final_modifiers)),
                .offset_strings = 0,
            };

            // Allocate memory and copy all data
            ice::meminfo minfo_layer = ice::meminfo_of<ice::InputActionLayerInfo>;
            ice::usize const offset_sources = minfo_layer += ice::array::meminfo(final_sources);
            ice::usize const offset_actions = minfo_layer += ice::array::meminfo(final_actions);
            ice::usize const offset_conditions = minfo_layer += ice::array::meminfo(final_conditions);
            ice::usize const offset_steps = minfo_layer += ice::array::meminfo(final_steps);
            ice::usize const offset_modifiers = minfo_layer += ice::array::meminfo(final_modifiers);
            ice::usize const offset_strings = minfo_layer += ice::string::meminfo(ice::String{strings});
            final_info.offset_strings = ice::u32(offset_strings.value);

            ice::Memory const final_memory = alloc.allocate(minfo_layer);
            ice::memcpy(final_memory, ice::data_view(final_info));
            ice::memcpy(ice::ptr_add(final_memory, offset_sources), ice::array::data_view(final_sources));
            ice::memcpy(ice::ptr_add(final_memory, offset_actions), ice::array::data_view(final_actions));
            ice::memcpy(ice::ptr_add(final_memory, offset_conditions), ice::array::data_view(final_conditions));
            ice::memcpy(ice::ptr_add(final_memory, offset_steps), ice::array::data_view(final_steps));
            ice::memcpy(ice::ptr_add(final_memory, offset_modifiers), ice::array::data_view(final_modifiers));
            ice::memcpy(ice::ptr_add(final_memory, offset_strings), ice::string::data_view(strings));
            return ice::create_input_action_layer(alloc, final_memory);
        }

    private:
        ice::Allocator& _allocator;
        ice::HashMap<SourceBuilder::Internal> _sources;
        ice::HashMap<ActionBuilder::Internal> _actions;
    };


    InputActionLayerBuilder::SourceBuilder::SourceBuilder(Internal* internal) noexcept
        : _internal{ internal }
    {
    }

    auto InputActionLayerBuilder::SourceBuilder::add_key(ice::input::KeyboardKey key) noexcept -> SourceBuilder&
    {
        using ice::input::DeviceType;
        using enum ice::InputActionSourceType;

        ICE_ASSERT_CORE(_internal->type == Key || _internal->type == Button);
        ice::hashmap::set(_internal->events, ice::hash(key), input_identifier(DeviceType::Keyboard, key));
        return *this;
    }

    auto InputActionLayerBuilder::SourceBuilder::add_button(ice::input::MouseInput button) noexcept -> SourceBuilder&
    {
        using ice::input::DeviceType;
        using enum ice::InputActionSourceType;

        ICE_ASSERT_CORE(_internal->type == Key || _internal->type == Button);
        ice::hashmap::set(_internal->events, ice::hash(button), input_identifier(DeviceType::Mouse, button));
        return *this;
    }

    auto InputActionLayerBuilder::SourceBuilder::add_button(ice::input::ControllerInput button) noexcept -> SourceBuilder&
    {
        using ice::input::DeviceType;
        using enum ice::InputActionSourceType;

        ICE_ASSERT_CORE(_internal->type == Key || _internal->type == Button);
        ice::hashmap::set(_internal->events, ice::hash(button), input_identifier(DeviceType::Controller, button));
        return *this;
    }

    auto InputActionLayerBuilder::SourceBuilder::add_axis(ice::input::MouseInput axis) noexcept -> SourceBuilder&
    {
        using ice::input::DeviceType;
        using ice::input::MouseInput;
        using enum ice::InputActionSourceType;

        ICE_ASSERT_CORE(_internal->type == Axis2d);
        ICE_ASSERT_CORE(axis == MouseInput::PositionX);
        if (axis == MouseInput::PositionX)
        {
            ice::hashmap::set(_internal->events, ice::hash(MouseInput::PositionX), input_identifier(DeviceType::Mouse, MouseInput::PositionX));
            ice::hashmap::set(_internal->events, ice::hash(MouseInput::PositionY), input_identifier(DeviceType::Mouse, MouseInput::PositionY));
        }
        return *this;
    }

    auto InputActionLayerBuilder::SourceBuilder::add_axis(ice::input::ControllerInput axis) noexcept -> SourceBuilder&
    {
        using ice::input::DeviceType;
        using ice::input::ControllerInput;
        using enum ice::InputActionSourceType;

        ICE_ASSERT_CORE(_internal->type == Axis2d);
        ICE_ASSERT_CORE(axis == ControllerInput::LeftAxisX || axis == ControllerInput::RightAxisX);
        if (axis == ControllerInput::LeftAxisX)
        {
            ice::hashmap::set(_internal->events, ice::hash(axis), input_identifier(DeviceType::Controller, ControllerInput::LeftAxisX));
            ice::hashmap::set(_internal->events, ice::hash(axis), input_identifier(DeviceType::Controller, ControllerInput::LeftAxisY));
        }
        else if (axis == ControllerInput::RightAxisX)
        {
            ice::hashmap::set(_internal->events, ice::hash(axis), input_identifier(DeviceType::Controller, ControllerInput::RightAxisX));
            ice::hashmap::set(_internal->events, ice::hash(axis), input_identifier(DeviceType::Controller, ControllerInput::RightAxisY));
        }
        return *this;
    }

    InputActionLayerBuilder::ActionBuilder::ActionBuilder(Internal *internal) noexcept
        : _internal{ internal }
    {
    }

    auto InputActionLayerBuilder::ActionBuilder::set_behavior(ice::InputActionBehavior behavior) noexcept -> ActionBuilder&
    {
        _internal->behavior = behavior;
        return *this;
    }

    auto InputActionLayerBuilder::ActionBuilder::set_toggled(bool value /*= true*/) noexcept -> ActionBuilder&
    {
        _internal->behavior = value ? InputActionBehavior::Toggled : InputActionBehavior::Default;
        return *this;
    }

    auto InputActionLayerBuilder::ActionBuilder::set_runonce(bool value /*= true*/) noexcept -> ActionBuilder&
    {
        _internal->behavior = value ? InputActionBehavior::ActiveOnce : InputActionBehavior::Default;
        return *this;
    }

    auto InputActionLayerBuilder::ActionBuilder::add_condition(
        ice::String source,
        ice::InputActionCondition condition,
        ice::InputActionConditionFlags flags /*= None*/,
        ice::f32 param /*= 0.0f*/,
        bool from_action /*= false*/
    ) noexcept -> ActionBuilder&
    {
        ice::array::push_back(_internal->conditions,
            ActionBuilderCondition{
                _internal->allocator, source, condition, flags, param, from_action
            }
        );
        _internal->current_condition = ice::addressof(ice::array::back(_internal->conditions));

        // if (ice::has_all(flags, InputActionConditionFlags::Activate))
        // {
        //     this->add_step(InputActionStep::Activate);
        // }
        // else if (ice::has_all(flags, InputActionConditionFlags::Deactivate))
        // {
        //     this->add_step(InputActionStep::Deactivate);
        // }
        return *this;
    }

    auto InputActionLayerBuilder::ActionBuilder::add_step(
        ice::InputActionStep step
    ) noexcept -> ActionBuilder&
    {
        ice::array::push_back(_internal->current_condition->steps,
            ActionBuilderStep{
                .step = step,
                .source = ice::HeapString<>{ _internal->allocator },
                .axis = { 0, 0 }
            }
        );
        return *this;
    }

    auto InputActionLayerBuilder::ActionBuilder::add_step(
        ice::String source,
        ice::InputActionStep step,
        ice::String target_axis /*= ".x"*/
    ) noexcept -> ActionBuilder&
    {
        ICE_ASSERT_CORE(_internal->current_condition != nullptr);

        ice::ucount source_size = ice::size(source);
        ice::u8 read_from = 0;
        ice::u8 write_to = 0;

        if (source[source_size - 2] == '.')
        {
            source_size -= 2;
            read_from = source[source_size + 1] - 'x';
            ICE_ASSERT_CORE(read_from >= 0 && read_from < 3);
        }
        if (ice::size(target_axis) == 2 && target_axis[0] == '.')
        {
            write_to = target_axis[1] - 'x';
            ICE_ASSERT_CORE(write_to >= 0 && write_to < 3);
        }

        ice::array::push_back(_internal->current_condition->steps,
            ActionBuilderStep{
                .step = step,
                .source = { _internal->allocator, ice::string::substr(source, 0, source_size) },
                .axis = { read_from, write_to }
            }
        );
        return *this;
    }

    auto InputActionLayerBuilder::ActionBuilder::add_modifier(
        ice::InputActionModifier modifier,
        ice::f32 param,
        ice::String target_axis /*= ".x"*/
    ) noexcept -> ActionBuilder&
    {
        ICE_ASSERT_CORE(ice::size(target_axis) >= 2 && target_axis[0] == '.');
        if (ice::size(target_axis) < 2 || target_axis[0] != '.')
        {
            return *this;
        }

        for (char axis_component : ice::string::substr(target_axis, 1, 3))
        {
            ICE_ASSERT_CORE(axis_component >= 'x' && axis_component <= 'z'); // .xyz

            ice::u8 const axis = axis_component - 'x';
            ice::array::push_back(_internal->modifiers, { .id = modifier, .axis = axis, .param = param });
        }
        return *this;
    }

    auto create_input_action_layer_builder(ice::Allocator& alloc) noexcept -> ice::UniquePtr<ice::InputActionLayerBuilder>
    {
        return ice::make_unique<SimpleInputActionLayerBuilder>(alloc, alloc);
    }

} // namespace ice
