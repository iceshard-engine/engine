#include "input_action_internal_types.hxx"
#include <ice/input_action_layer_builder.hxx>
#include <ice/input_action_layer.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/sort.hxx>

namespace ice
{

    namespace detail
    {

        auto parse_source(ice::String source) noexcept -> std::tuple<ice::String, ice::u8>
        {
            ice::u8 read_from = 0;
            ice::ucount source_size = ice::size(source);
            // We want to parse the following cases: <source>(.[xyz])
            if (source_size > 1 && source[source_size - 2] == '.')
            {
                source_size -= 2;
                read_from = source[source_size + 1] - 'x';
                ICE_ASSERT_CORE(read_from >= 0 && read_from < 3);
            }
            return { ice::string::substr(source, 0, source_size), read_from };
        }

    } // namespace detail

    class SimpleInputActionLayerBuilder;

    using ActionBuilderModifier = InputActionModifierData;

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
            ice::u8 axis,
            bool from_action
        ) noexcept
            : condition{ condition }
            , flags{ flags }
            , source{ alloc, name }
            , steps{ alloc }
            , param{ param }
            , axis{ axis }
            , from_action{ from_action }
        {
        }

        //! \brief The check to be performed.
        ice::InputActionCondition condition;

        //! \brief Flags that may affect how conditions are executed.
        ice::InputActionConditionFlags flags;

        //! \brief The name of the source which values to use.
        ice::HeapString<> source;

        //! \brief Steps to be executed.
        ice::Array<ActionBuilderStep> steps;

        ice::f32 param;

        ice::u8 axis;

        //! \brief Contains info the the condition source should be taken from an action instead.
        bool from_action = false;
    };

    template<>
    struct InputActionBuilder::BuilderBase::Internal<InputActionBuilder::Layer>
    {

    };

    template<>
    struct InputActionBuilder::BuilderBase::Internal<SimpleInputActionLayerBuilder> : InputActionBuilder::BuilderBase::Internal<InputActionBuilder::Layer>
    {
        void destroy(ice::Allocator& alloc) noexcept
        {
            alloc.destroy(this);
        }
    };

    template<>
    struct InputActionBuilder::BuilderBase::Internal<InputActionBuilder::Source>
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

    template<>
    struct InputActionBuilder::BuilderBase::Internal<InputActionBuilder::ConditionSeries>
    {
        Internal(ice::Allocator& alloc) noexcept
            : allocator{ alloc }
            , conditions{ alloc }
        {
            ice::array::reserve(conditions, 4);
        }

        void add_step(ActionBuilderStep&& step) noexcept
        {
            ice::array::push_back(
                ice::array::back(conditions).steps,
                ice::move(step)
            );
        }

        void add_condition(ActionBuilderCondition&& condition) noexcept
        {
            ice::array::push_back(conditions, ice::move(condition));
        }

        void finalize() noexcept
        {
            ActionBuilderCondition& final_condition = ice::array::back(conditions);

            // Ensure this series is finished after this condition.
            final_condition.flags |= InputActionConditionFlags::SeriesFinish;
        }

    public:
        ice::Allocator& allocator;
        ice::Array<ActionBuilderCondition> conditions;
    };

    template<>
    struct InputActionBuilder::BuilderBase::Internal<InputActionBuilder::Action>
    {
        Internal(
            ice::Allocator& alloc,
            ice::String name,
            ice::InputActionDataType type
        ) noexcept
            : allocator{ alloc }
            , name{ alloc, name }
            , type{ type }
            , cond_series{ alloc }
            , modifiers{ alloc }
        {
            ice::array::reserve(cond_series, 3);
            ice::array::reserve(modifiers, 2);
        }

        ice::Allocator& allocator;
        ice::HeapString<> name;
        ice::InputActionDataType type;
        ice::InputActionBehavior behavior;
        ice::Array<Internal<InputActionBuilder::ConditionSeries>> cond_series;
        ice::Array<ice::ActionBuilderModifier> modifiers;

        auto add_condition_series() noexcept -> InputActionBuilder::ConditionSeries
        {
            ice::array::push_back(cond_series, Internal<InputActionBuilder::ConditionSeries>{ allocator });
            Internal<InputActionBuilder::ConditionSeries>* const series_ptr = ice::addressof(ice::array::back(cond_series));
            return { series_ptr };
        }

        void finalize()
        {
            for (Internal<InputActionBuilder::ConditionSeries>& series : this->cond_series)
            {
                series.finalize();
            }
        }
    };


    class SimpleInputActionLayerBuilder : public ice::InputActionBuilder::Layer
    {
    public:
        SimpleInputActionLayerBuilder(
            ice::Allocator& alloc,
            ice::String name
        ) noexcept
            : Layer{ alloc.create<Internal<Layer>>() }
            , _allocator{ alloc }
            , _sources{ alloc }
            , _actions{ alloc }
            , _name{ alloc, name }
        {
            ice::hashmap::reserve(_sources, 16);
            ice::hashmap::reserve(_actions, 10);
        }

        ~SimpleInputActionLayerBuilder()
        {
            this->internal().destroy(_allocator);
        }

        auto set_name(
            ice::String name
        ) noexcept -> ice::InputActionBuilder::Layer& override
        {
            _name = name;
            return *this;
        }

        auto define_source(
            ice::String name,
            ice::InputActionSourceType type
        ) noexcept -> ice::InputActionBuilder::Source override
        {
            ice::hashmap::set(_sources, ice::hash(name), {_allocator, name, type});
            return { ice::hashmap::try_get(_sources, ice::hash(name)) };
        }

        auto define_action(
            ice::String name,
            ice::InputActionDataType type
        ) noexcept -> ice::InputActionBuilder::Action override
        {
            ice::hashmap::set(_actions, ice::hash(name), {_allocator, name, type});
            return { ice::hashmap::try_get(_actions, ice::hash(name)) };
        }

        auto finalize(ice::Allocator& alloc) noexcept -> ice::UniquePtr<ice::InputActionLayer> override
        {
            ice::u16 count_storage_values = 0;

            ice::HeapString<> strings{ _allocator };
            ice::Array<ice::InputActionSourceInputInfo> final_sources{ _allocator };
            ice::Array<ice::InputActionStepData> final_steps{ _allocator };
            ice::Array<ice::InputActionConditionData> final_conditions{ _allocator };
            ice::Array<ice::InputActionModifierData> final_modifiers{ _allocator };
            ice::Array<ice::InputActionInfo> final_actions{ _allocator };

            // Insert layer name as the first string
            ice::string::push_back(strings, _name);
            ice::string::push_back(strings, '\0');

            // Prepare data of all sources
            for (Internal<InputActionBuilder::Source> const& source : _sources)
            {
                for (ice::input::InputID input_event : source.events)
                {
                    ice::array::push_back(final_sources,
                        InputActionSourceInputInfo{
                            .name = { ice::u16(ice::size(strings)), ice::u16(ice::size(source.name)) },
                            .input = input_event,
                            .type = source.type,
                            .storage_offset = count_storage_values,
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
                    [&strings](ice::InputActionSourceInputInfo const& source, ice::String expected) noexcept
                    {
                        ice::String const source_name = ice::string::substr(
                            strings, source.name.offset, source.name.size
                        );
                        return expected == source_name;
                    },
                    idx_found
                );

                ICE_ASSERT_CORE(found && idx_found < ice::array::count(final_sources));
                ice::u16 const source_storage_idx = final_sources[idx_found].storage_offset;
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
                            strings, action.name.offset, action.name.size
                        );
                        return expected == source_name;
                    },
                    idx_found
                );

                ICE_ASSERT_CORE(found && idx_found < ice::array::count(final_actions));
                return ice::u16(idx_found);
            };

            // Run finalization on all internal action objects.
            for (Internal<InputActionBuilder::Action>& action : ice::hashmap::values(_actions))
            {
                action.finalize();
            }

            // Prepare data of all actions
            ice::u16 step_offset = 0, step_count = 0;
            ice::u16 condition_offset = 0, condition_count = 0;
            ice::u8 modifier_offset = 0, modifier_count = 0;

            for (Internal<InputActionBuilder::Action> const& action : _actions)
            {
                for (Internal<InputActionBuilder::ConditionSeries> const& series : action.cond_series)
                {
                    for (ActionBuilderCondition const& condition : series.conditions)
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

                        ICE_ASSERT_CORE(action.type != InputActionDataType::Invalid);
                        ice::InputActionIndex source_index = { .source_index = 0,.source_axis = 0 };
                        if (condition.from_action)
                        {
                            // If we are empty, it's a "self reference"
                            if (ice::string::any(condition.source))
                            {
                                source_index.source_index = find_action_storage_index(condition.source);
                                source_index.source_axis = condition.axis;
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
                            source_index.source_axis = condition.axis;
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
                } // for (ConditionSeries& series : ...)

                modifier_count = ice::u8(ice::count(action.modifiers));
                ice::array::push_back(final_modifiers, action.modifiers);

                ice::array::push_back(final_actions,
                    InputActionInfo{
                        .name = { ice::u16(ice::size(strings)), ice::u16(ice::size(action.name)) },
                        .type = action.type,
                        .behavior = action.behavior,
                        .conditions = { condition_offset, condition_count },
                        .mods = { modifier_offset, modifier_count }
                    }
                );

                ice::string::push_back(strings, action.name);

                modifier_offset += modifier_count;
                condition_offset += ice::exchange(condition_count, ice::u16_0);
            }

            ice::InputActionLayerInfoHeader final_info{
                .size_name = ice::u16(ice::size(_name)),
                .count_sources = ice::u16(ice::count(final_sources)),
                .count_actions = ice::u16(ice::count(final_actions)),
                .count_conditions = ice::u16(ice::count(final_conditions)),
                .count_steps = ice::u16(ice::count(final_steps)),
                .count_modifiers = ice::u16(ice::count(final_modifiers)),
                .offset_strings = 0,
            };

            // Allocate memory and copy all data
            ice::meminfo minfo_layer = ice::meminfo_of<ice::InputActionLayerInfoHeader>;
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
        ice::HashMap<Internal<InputActionBuilder::Source>> _sources;
        ice::HashMap<Internal<InputActionBuilder::Action>> _actions;
        ice::HeapString<> _name;
    };

    auto InputActionBuilder::Source::add_key(ice::input::KeyboardKey key) noexcept -> Source&
    {
        using ice::input::DeviceType;
        using enum ice::InputActionSourceType;

        ICE_ASSERT_CORE(internal().type == Key || internal().type == Button);
        ice::hashmap::set(internal().events, ice::hash(key), input_identifier(DeviceType::Keyboard, key));
        return *this;
    }

    auto InputActionBuilder::Source::add_keymod(ice::input::KeyboardMod keymod) noexcept -> Source&
    {
        using ice::input::DeviceType;
        using enum ice::InputActionSourceType;

        ice::input::InputID const iid = input_identifier(DeviceType::Keyboard, keymod, ice::input::mod_identifier_base_value);

        ICE_ASSERT_CORE(internal().type == Key || internal().type == Button);
        ice::hashmap::set(internal().events, ice::hash(iid), iid);
        return *this;
    }

    auto InputActionBuilder::Source::add_button(ice::input::MouseInput button) noexcept -> Source&
    {
        using ice::input::DeviceType;
        using enum ice::InputActionSourceType;

        ICE_ASSERT_CORE(internal().type == Key || internal().type == Button);
        ice::hashmap::set(internal().events, ice::hash(button), input_identifier(DeviceType::Mouse, button));
        return *this;
    }

    auto InputActionBuilder::Source::add_button(ice::input::ControllerInput button) noexcept -> Source&
    {
        using ice::input::DeviceType;
        using enum ice::InputActionSourceType;

        ICE_ASSERT_CORE(internal().type == Key || internal().type == Button);
        ice::hashmap::set(internal().events, ice::hash(button), input_identifier(DeviceType::Controller, button));
        return *this;
    }

    auto InputActionBuilder::Source::add_axis(ice::input::MouseInput axis) noexcept -> Source&
    {
        using ice::input::DeviceType;
        using ice::input::MouseInput;
        using enum ice::InputActionSourceType;

        ICE_ASSERT_CORE(internal().type == Axis2d);
        ICE_ASSERT_CORE(axis == MouseInput::PositionX);
        if (axis == MouseInput::PositionX)
        {
            ice::hashmap::set(internal().events, ice::hash(MouseInput::PositionX), input_identifier(DeviceType::Mouse, MouseInput::PositionX));
            ice::hashmap::set(internal().events, ice::hash(MouseInput::PositionY), input_identifier(DeviceType::Mouse, MouseInput::PositionY));
        }
        return *this;
    }

    auto InputActionBuilder::Source::add_axis(ice::input::ControllerInput axis) noexcept -> Source&
    {
        using ice::input::DeviceType;
        using ice::input::ControllerInput;
        using enum ice::InputActionSourceType;

        ICE_ASSERT_CORE(internal().type == Axis2d);
        ICE_ASSERT_CORE(axis == ControllerInput::LeftAxisX || axis == ControllerInput::RightAxisX);
        if (axis == ControllerInput::LeftAxisX)
        {
            ice::hashmap::set(internal().events, ice::hash(axis), input_identifier(DeviceType::Controller, ControllerInput::LeftAxisX));
            ice::hashmap::set(internal().events, ice::hash(axis), input_identifier(DeviceType::Controller, ControllerInput::LeftAxisY));
        }
        else if (axis == ControllerInput::RightAxisX)
        {
            ice::hashmap::set(internal().events, ice::hash(axis), input_identifier(DeviceType::Controller, ControllerInput::RightAxisX));
            ice::hashmap::set(internal().events, ice::hash(axis), input_identifier(DeviceType::Controller, ControllerInput::RightAxisY));
        }
        return *this;
    }

    void InputActionBuilder::ConditionSeries::set_finished(bool can_finalize_condition_checks) noexcept
    {
        if (can_finalize_condition_checks)
        {
            ice::array::back(internal().conditions).flags |= InputActionConditionFlags::Final;
        }
        else
        {
            ice::array::back(internal().conditions).flags |= InputActionConditionFlags::SeriesFinish;
        }
    }

    auto InputActionBuilder::ConditionSeries::add_condition(
        ice::String source,
        ice::InputActionCondition condition,
        ice::InputActionConditionFlags flags /*= None*/,
        ice::f32 param /*= 0.0f*/,
        bool from_action /*= false*/
    ) noexcept -> ConditionSeries&
    {
        auto const[source_name, read_from] = detail::parse_source(source);

        internal().add_condition(
            ActionBuilderCondition{
                internal().allocator,
                source_name,
                condition,
                flags,
                param,
                read_from,
                from_action
            }
        );

        return *this;
    }

    auto InputActionBuilder::ConditionSeries::add_step(
        ice::InputActionStep step
    ) noexcept -> ConditionSeries&
    {
        internal().add_step({
            .step = step,
            .source = ice::HeapString<>{ internal().allocator },
            .axis = { 0, 0 }
        });
        return *this;
    }

    auto InputActionBuilder::ConditionSeries::add_step(
        ice::String source,
        ice::InputActionStep step,
        ice::String target_axis /*= ".x"*/
    ) noexcept -> ConditionSeries&
    {
        auto const [source_name, read_from] = detail::parse_source(source);
        auto const [_, write_to] = detail::parse_source(target_axis);

        internal().add_step({
            .step = step,
            .source = { internal().allocator, source_name },
            .axis = { read_from, write_to }
        });
        return *this;
    }

    auto InputActionBuilder::Action::add_condition_series() noexcept -> ConditionSeries
    {
        return internal().add_condition_series();
    }

    auto InputActionBuilder::Action::set_behavior(
        ice::InputActionBehavior behavior
    ) noexcept -> Action&
    {
        internal().behavior = behavior;
        return *this;
    }

    auto InputActionBuilder::Action::add_modifier(
        ice::InputActionModifier modifier,
        ice::f32 param,
        ice::String target_axis /*= ".x"*/
    ) noexcept -> Action&
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
            ice::array::push_back(internal().modifiers, { .id = modifier, .axis = axis, .param = param });
        }
        return *this;
    }

    auto create_input_action_layer_builder(
        ice::Allocator& alloc,
        ice::String name
    ) noexcept -> ice::UniquePtr<ice::InputActionBuilder::Layer>
    {
        return ice::make_unique<SimpleInputActionLayerBuilder>(alloc, alloc, name);
    }

} // namespace ice
