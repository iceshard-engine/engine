
#include "input_action_internal_types.hxx"
#include "input_action_dsl_layer_builder.hxx"

#include <ice/input/input_keyboard.hxx>
#include <ice/input/input_mouse.hxx>
#include <ice/input_action_layer.hxx>
#include <ice/input_action_layer_builder.hxx>
#include <ice/input_action_definitions.hxx>
#include <ice/input_action_executor.hxx>
#include <ice/container/array.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/profiler.hxx>
#include <ice/clock.hxx>
#include <ice/log.hxx>

namespace ice
{

    struct StandardInputActionLayerParams
    {
        ice::Span<ice::InputActionSourceInfo const> sources;
        ice::Span<ice::InputActionInfo const> actions;
        ice::Span<ice::InputActionConditionData const> conditions;
        ice::Span<ice::InputActionStepData const> steps;
        ice::Span<ice::InputActionModifierData const> modifiers;
        ice::String strings;
    };

    auto params_from_data(ice::Data memory) noexcept -> ice::Expected<ice::StandardInputActionLayerParams>
    {
        ice::InputActionLayerInfo const* layer_info = reinterpret_cast<ice::InputActionLayerInfo const*>(memory.location);
        if (layer_info == nullptr)
        {
            return E_Fail;
        }

        ice::usize offset = ice::size_of<ice::InputActionLayerInfo>;
        ice::StandardInputActionLayerParams result{};

        result.sources = ice::span::from_data<ice::InputActionSourceInfo>(memory, layer_info->count_sources, offset);
        offset += ice::span::data_view(result.sources).size;
        result.actions = ice::span::from_data<ice::InputActionInfo>(memory, layer_info->count_actions, offset);
        offset += ice::span::data_view(result.actions).size;
        result.conditions = ice::span::from_data<ice::InputActionConditionData>(memory, layer_info->count_conditions, offset);
        offset += ice::span::data_view(result.conditions).size;
        result.steps = ice::span::from_data<ice::InputActionStepData>(memory, layer_info->count_steps, offset);
        offset += ice::span::data_view(result.steps).size;
        result.modifiers = ice::span::from_data<ice::InputActionModifierData>(memory, layer_info->count_modifiers, offset);
        offset += ice::span::data_view(result.modifiers).size;

        ICE_ASSERT_CORE(offset == ice::usize{ layer_info->offset_strings });
        result.strings = ice::string::from_data(memory, { layer_info->offset_strings }, ice::ucount(memory.size.value - layer_info->offset_strings));
        return result;
    }

    class StandardInputActionLayer final : public ice::InputActionLayer
    {
    public:
        StandardInputActionLayer(
            ice::Allocator& alloc,
            ice::Memory memory,
            ice::StandardInputActionLayerParams const& params
        ) noexcept
            : _allocator{ alloc }
            , _rawdata{ memory }
            , _sources{ params.sources }
            , _actions{ params.actions }
            , _conditions{ params.conditions }
            , _steps{ params.steps }
            , _modifiers{ params.modifiers }
            , _strings{ params.strings }
            , _runtime_sources{ alloc }
        {
            ice::array::resize(_runtime_sources, ice::count(_sources));
            ice::array::memset(_runtime_sources, 0);
        }

        ~StandardInputActionLayer() noexcept override
        {
            _allocator.deallocate(_rawdata);
        }

        auto name() const noexcept -> ice::String override
        {
            return "Default";
        }

        auto sources() const noexcept -> ice::Span<ice::InputActionSourceInfo const> override
        {
            return _sources;
        }

        auto source_name(ice::InputActionSourceInfo const& source) const noexcept -> ice::String override
        {
            return ice::string::substr(_strings, source.name_offset, source.name_length);
        }

        auto actions() const noexcept -> ice::Span<ice::InputActionInfo const> override
        {
            return _actions;
        }

        auto action_name(ice::InputActionInfo const& action) const noexcept -> ice::String override
        {
            return ice::string::substr(_strings, action.name_offset, action.name_length);
        }

        bool process_inputs(
            ice::Span<ice::input::InputEvent const> input_events,
            ice::Span<ice::InputActionSource* const> source_values
        ) const noexcept override
        {
            IPT_ZONE_SCOPED;

            ice::ucount index = 0;
            ice::ucount count = ice::count(input_events);

            while(index < count)
            {
                ice::input::InputEvent const ev = input_events[index];

                ice::i32 src_idx = -1;
                for (ice::InputActionSourceInfo const& src : _sources)
                {
                    src_idx += 1;

                    ICE_ASSERT_CORE(source_values[src.storage] != nullptr);
                    ice::InputActionSource& value = source_values[src.storage][ev.axis_idx];

                    if (src.input != ev.identifier)
                    {
                        // Reset event after it changed to 'KeyRelease'
                        if (value.event == InputActionSourceEvent::KeyRelease && value.changed == false)
                        {
                            value.event = InputActionSourceEvent::None;
                        }
                        continue;
                    }

                    if (ev.value_type == ice::input::InputValueType::Trigger)
                    {
                        value = { InputActionSourceEvent::Trigger, ev.value.axis.value_f32, true };
                    }
                    else if (ev.value_type == ice::input::InputValueType::AxisInt)
                    {
                        value = { InputActionSourceEvent::Axis, ice::f32(ev.value.axis.value_i32), true };
                    }
                    else if (ev.value_type == ice::input::InputValueType::AxisFloat)
                    {
                        // Check for deadzone values
                        if (src.param < ev.value.axis.value_f32)
                        {
                            value = { InputActionSourceEvent::Axis, ev.value.axis.value_f32, true };
                        }
                        else
                        {
                            value = { InputActionSourceEvent::AxisDeadzone, ev.value.axis.value_f32, true };
                        }
                    }
                    else
                    {
                        value = {
                            ev.value.button.state.released
                                ? InputActionSourceEvent::KeyRelease
                                : InputActionSourceEvent::KeyPress,
                            ev.value.button.state.released
                                ? 0.0f : 1.0f,
                            /*changed*/ true
                        };
                    }

                    // ICE_LOG(LogSeverity::Debug, LogTag::Engine, "SRC: '{}', VAL = {}", ice::string::substr(_strings, src.name_offset, src.name_length), value.value);

                    // Remove the processed input
                    // if (ice::exchange(removed, true) == false)
                    // {
                    //     ice::array::remove_at(inputs, index);
                    //     count -= 1;
                    // }
                }

                index += 1;
            }

            return count != ice::count(input_events);
        }

        bool update_actions(
            ice::InputActionExecutor const& executor,
            ice::Span<ice::InputActionSource* const> source_values,
            ice::HashMap<ice::InputActionRuntime>& actions
        ) const noexcept override
        {
            IPT_ZONE_SCOPED;
            using enum ice::InputActionConditionFlags;

            for (ice::InputActionInfo const& action : _actions)
            {
                ice::String const action_name = ice::string::substr(_strings, action.name_offset, action.name_length);

                ice::InputActionRuntime* const runtime = ice::hashmap::try_get(actions, ice::hash(action_name));
                if (action.behavior != InputActionBehavior::Accumulated)
                {
                    runtime->raw_value = {};
                }

                bool series_success = false;
                ice::Span const conditions = ice::span::subspan(_conditions, action.conditions.x, action.conditions.y);
                for (ice::InputActionConditionData const& cond : conditions)
                {
                    bool cond_result = false;
                    if (cond.id >= ice::InputActionCondition::Enabled)
                    {
                        #if 0
                        ice::InputActionRuntime const* tested_runtime = ice::hashmap::try_get(actions, ice::hash(_actions[cond.source.source_index].name));
                        ICE_ASSERT_CORE(tested_runtime != nullptr);
                        ICE_ASSERT_CORE(cond.id < ice::InputConditionID::UserCustom);

                        cond_result = _db->execute_condition(
                            cond.id,
                            *tested_runtime,
                            cond.param
                        );
                        #endif
                        // ICE_ASSERT_CORE(false);
                    }
                    else
                    {
                        ICE_ASSERT_CORE(source_values[cond.source.source_index] != nullptr);
                        ice::InputActionSource& value = source_values[cond.source.source_index][0];

                        if (value.event != InputActionSourceEvent::None)
                        {
                            cond_result = executor.execute_condition(cond.id, value, cond.param);
                        }
                    }

                    // Can't check for 'SeriesOr' since it's just a zero value
                    if (ice::has_all(cond.flags, InputActionConditionFlags::SeriesAnd))
                    {
                        series_success &= cond_result;
                    }
                    else
                    {
                        series_success |= cond_result;
                    }

                    bool const check_success = ice::has_all(cond.flags, SeriesCheck)
                        ? series_success
                        : cond_result;

                    if (ice::has_all(cond.flags, RunSteps) && check_success)
                    {
                        ice::Span const steps = ice::span::subspan(_steps, cond.steps.x, cond.steps.y);
                        for (ice::InputActionStepData const& step : steps)
                        {
                            if (step.id < InputActionStep::Set)
                            {
                                executor.execute_step(step.id, *runtime);
                            }
                            else
                            {
                                ICE_ASSERT_CORE(source_values[step.source.source_index] != nullptr);
                                ice::InputActionSource& value = source_values[step.source.source_index][step.source.source_axis];

                                executor.execute_step(step.id, value, runtime->raw_value.v[0][step.dst_axis]);
                            }
                        }
                    }

                    // If this is not 'series finish', continue
                    if (ice::has_none(cond.flags, SeriesFinish))
                    {
                        continue;
                    }

                    // // If the condition should activate/deactivate implicitly
                    // // TODO: Move before the final check to allow activating on regular checks?
                    // if (ice::has_all(cond.flags, Activate) && runtime->enabled)
                    // {
                    //     // This small operation allows use to implement 'Once' logic.
                    //     // Initial state == 1, unless deactivated from now on we will always be above 1
                    //     //   since we purposefully loosing bits due to overflow but always set the MSB again.
                    //     runtime->state = runtime->state * 2 + 1;
                    //     runtime->active = check_success;
                    // }
                    // else if (ice::has_all(cond.flags, Deactivate))
                    // {
                    //     runtime->state = 0;
                    //     runtime->active = false;
                    // }

                    // If the series is not successful, just continue
                    if (series_success == false || runtime->enabled == false)
                    {
                        // If the action finaly failes, we reset the state so we can again start counting from 0
                        runtime->state = 0;
                        runtime->active = false;
                        continue;
                    }

                    series_success = false;

                    // continue, if it's not a final condition
                    if (ice::has_none(cond.flags, Final))
                    {
                        continue;
                    }

                    // Stop checking futher conditions, we are in the final  condition
                    break;
                }
            }

            for (ice::InputActionInfo const& action : _actions)
            {
                ice::String const action_name = ice::string::substr(_strings, action.name_offset, action.name_length);
                ice::InputActionRuntime* const runtime = ice::hashmap::try_get(actions, ice::hash(action_name));

                // Handles 'Toggle'. We only activate of the first press, which is `state == 1`.
                if (action.behavior == InputActionBehavior::Toggled)
                {
                    if (runtime->state == 1)
                    {
                        runtime->toggle_enabled = !runtime->toggle_enabled;
                    }
                    else
                    {
                        runtime->active = runtime->toggle_enabled;
                    }
                }
                // Handles 'Once'. Once state is bigger than `1` we always deactivate the action.
                else if (action.behavior == InputActionBehavior::ActiveOnce && runtime->state > 1)
                {
                    runtime->active = false;
                }

                if (runtime->active == false)
                {
                    runtime->was_active = false;
                    continue;
                }

                if (runtime->was_active == false)
                {
                    runtime->was_active = true;
                    runtime->timestamp = ice::clock::now();
                }

                // Update the final value and run modifiers over it.
                runtime->value = { runtime->raw_value.x, runtime->raw_value.y };

                ice::Span const mods = ice::span::subspan(_modifiers, action.mods.x, action.mods.y);
                for (ice::InputActionModifierData const& mod : mods)
                {
                    executor.execute_modifier(mod.id, runtime->value.v[0][mod.axis], mod.param);
                }
            }
            return false;
        }

    private:
        ice::Allocator& _allocator;
        ice::Memory _rawdata;

        ice::Span<ice::InputActionSourceInfo const> _sources;
        ice::Span<ice::InputActionInfo const> _actions;
        ice::Span<ice::InputActionConditionData const> _conditions;
        ice::Span<ice::InputActionStepData const> _steps;
        ice::Span<ice::InputActionModifierData const> _modifiers;
        ice::String _strings;

        ice::Array<ice::InputActionSource> _runtime_sources;
    };

    auto create_input_action_layer(
        ice::Allocator& alloc,
        ice::Data layer_data
    ) noexcept -> ice::UniquePtr<ice::InputActionLayer>
    {
        ice::Memory const data_copy = alloc.allocate({ layer_data.size, layer_data.alignment });
        ice::memcpy(data_copy, layer_data);

        ice::Expected<ice::StandardInputActionLayerParams> params = ice::params_from_data(ice::data_view(data_copy));
        if (params.succeeded() == false)
        {
            return {};
        }

        return ice::make_unique<ice::StandardInputActionLayer>(alloc, alloc, data_copy, params.value());
    }

    auto create_input_action_layer(
        ice::Allocator& alloc,
        ice::Memory layer_data
    ) noexcept -> ice::UniquePtr<ice::InputActionLayer>
    {
        ice::Expected<ice::StandardInputActionLayerParams> params = ice::params_from_data(ice::data_view(layer_data));
        if (params.succeeded() == false)
        {
            return {};
        }

        return ice::make_unique<ice::StandardInputActionLayer>(alloc, alloc, layer_data, params.value());
    }

    auto parse_input_action_layer(
        ice::Allocator& alloc,
        ice::String definition
    ) noexcept -> ice::UniquePtr<ice::InputActionLayer>
    {
        InputActionDSLLayerBuilder dsl_builder{ ice::create_input_action_layer_builder(alloc) };
        if (ice::parse_action_input_definition(definition, dsl_builder))
        {
            return dsl_builder.finalize(alloc);
        }
        return {};
    }

    auto save_input_action_layer(
        ice::Allocator& alloc,
        ice::InputActionLayer const& action_layer
    ) noexcept -> ice::Expected<ice::Memory>
    {
        return {};
    }

} // namespace ice
