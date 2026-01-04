/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/input_action_stack.hxx>
#include <ice/input_action_layer.hxx>
#include <ice/input_action_info.hxx>
#include <ice/input_action_executor.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/container/array.hxx>
#include <ice/container/queue.hxx>
#include <ice/profiler.hxx>
#include <ice/clock.hxx>
#include <ice/sort.hxx>

namespace ice
{

    class SimpleInputActionStack final : public InputActionStack
    {
    public:
        SimpleInputActionStack(
            ice::Allocator& alloc,
            ice::String actionid_prefix
        ) noexcept
            : _allocator{ alloc }
            , _idprefix{ _allocator, actionid_prefix }
            , _layers{ _allocator }
            , _layers_active{ _allocator }
            , _layers_sources_indices{ _allocator }
            , _sources_runtime_values{ _allocator }
            , _sources{ _allocator }
            , _actions{ _allocator }
            , _action_names{ _allocator }
        {
        }

        auto registered_layers(
            ice::Array<ice::InputActionLayer const*>& out_layers
        ) const noexcept -> ice::u32 override;

        auto register_layer(
            ice::InputActionLayer const* layer
        ) noexcept -> ice::Result override;


        auto active_layers(
            ice::Array<ice::InputActionLayer const*>& out_layers
        ) const noexcept -> ice::u32 override;

        void push_layer(
            ice::InputActionLayer const* layer
        ) noexcept override;

        void pop_layer(
            ice::InputActionLayer const* layer
        ) noexcept override;


        auto action(
            ice::String action
        ) const noexcept -> ice::Expected<ice::InputAction const*> override;

        bool action_check(
            ice::String action,
            ice::InputActionCheck check = InputActionCheck::Active
        ) const noexcept override;

        auto action_time(
            ice::String action
        ) const noexcept -> ice::Tms override;

        bool action_value(
            ice::String action,
            ice::vec2f& out_value
        ) const noexcept override;

        bool action_value(
            ice::String action,
            ice::vec2f& out_value,
            ice::Tns& out_timeactive
        ) const noexcept override;

        void process_inputs(ice::Span<ice::input::InputEvent const> events) noexcept override;
        auto publish_shards(ice::ShardContainer& out_shards) const noexcept -> void override;


        auto action_runtime(
            ice::InputActionLayer const& layer,
            ice::InputActionInfo const& action_info
        ) const noexcept -> ice::InputActionRuntime const& override;

        auto source_runtime(
            ice::InputActionLayer const& layer,
            ice::InputActionSourceInputInfo const& source_info
        ) const noexcept -> ice::InputActionSource const& override;

    private:
        struct StackLayer
        {
            ice::InputActionLayer const* layer;
            ice::ref16 sources_indices;
        };

        struct ActiveStackLayer
        {
            ice::u32 index;
        };

        struct StackSourceIdx
        {
            ice::u32 index;
        };

        // Helpers
        static bool compare_layers(StackLayer const& slayer, ice::InputActionLayer const* layer) noexcept
        {
            return slayer.layer == layer;
        }

    private:
        ice::Allocator& _allocator;
        ice::HeapString<> _idprefix;

        ice::Array<StackLayer> _layers;
        ice::Array<ActiveStackLayer> _layers_active;
        ice::Array<ice::u32> _layers_sources_indices;
        ice::Array<ice::InputActionSource> _sources_runtime_values;

        ice::HashMap<StackSourceIdx> _sources;
        ice::HashMap<InputActionRuntime> _actions;
        ice::HashMap<HeapString<>> _action_names;
    };

    auto SimpleInputActionStack::registered_layers(
        ice::Array<ice::InputActionLayer const*>& out_layers
    ) const noexcept -> ice::u32
    {
        for (StackLayer const& layer : _layers)
        {
            ice::array::push_back(out_layers, layer.layer);
        }
        return ice::count(_layers);
    }

    auto SimpleInputActionStack::register_layer(
        ice::InputActionLayer const* layer
    ) noexcept -> ice::Result
    {
        IPT_ZONE_SCOPED;
        if (layer == nullptr)
        {
            return E_NullPointer;
        }

        ice::u32 layer_idx;
        if (ice::search(ice::Span{ _layers }, layer, compare_layers, layer_idx))
        {
            return S_LayerAlreadyRegistered;
        }

        layer_idx = ice::count(_layers);

        // We reserve enough space to store all actual indices for accessed layer sources.
        ice::u32 const layer_sources_count = ice::count(layer->sources());
        ice::u32 const layer_sources_offset = ice::count(_layers_sources_indices);
        ice::array::reserve(_layers_sources_indices, layer_sources_offset + layer_sources_count);

        // Go through each resource and set the indices
        ice::u64 prev_name_hash = 0;
        ice::Span<ice::InputActionSourceInputInfo const> sources = layer->sources();
        ice::u32 const count_sources = ice::count(sources);
        for (ice::u32 idx = 0; idx < count_sources; ++idx)
        {
            ice::InputActionSourceInputInfo const& source = sources[idx];
            ice::String const source_name = layer->source_name(source);
            ice::u64 const source_name_hash = ice::hash(source_name);

            // First we check if a source with a name like this already exists, if so we take the index to it's location
            //   else we push back a new value to the list of runtime values and store it's index under the hashed name.
            ice::u32 values_index = 0;
            if (ice::hashmap::has(_sources, source_name_hash))
            {
                if (prev_name_hash != source_name_hash)
                {
                    values_index = ice::hashmap::try_get(_sources, source_name_hash)->index;

                    // Stores the index for the source
                    ice::array::push_back(_layers_sources_indices, values_index);

                    if (source.type == InputActionSourceType::Axis2d)
                    {
                        // Stores the index for the source
                        ice::array::push_back(_layers_sources_indices, values_index);
                    }
                }
            }
            else
            {
                values_index = ice::array::count(_sources_runtime_values);

                ice::array::push_back(_sources_runtime_values, InputActionSource{});
                // If we have an Axis2d source, we need to actually push back two values for both axis
                if (source.type == InputActionSourceType::Axis2d)
                {
                    ice::array::push_back(_sources_runtime_values, InputActionSource{});
                }

                // Save the index where we store the runtime value(s)
                ice::hashmap::set(_sources, source_name_hash, { values_index });

                // Stores the index for the source
                ice::array::push_back(_layers_sources_indices, values_index);

                if (source.type == InputActionSourceType::Axis2d)
                {
                    // Stores the index for the source
                    ice::array::push_back(_layers_sources_indices, values_index);
                }
            }

            prev_name_hash = source_name_hash;
        }

        // Go through each resource and assing the pointers.
        for (ice::InputActionInfo const& action : layer->actions())
        {
            ice::String const action_name = layer->action_name(action);
            ice::u64 const action_name_hash = ice::hash(action_name);

            if (ice::hashmap::has(_actions, action_name_hash) == false)
            {
                // Create the final name with the prefix and store it so the pointer reimains valid.
                // #TODO: Consider using refs instead?
                ice::HeapString<> final_name = _idprefix;
                final_name.push_back(action_name);
                ice::hashmap::set(_action_names, action_name_hash, ice::move(final_name));

                ice::HeapString<> const* final_name_ptr = ice::hashmap::try_get(_action_names, action_name_hash);
                ICE_ASSERT_CORE(final_name_ptr != nullptr);

                // Save the pointer where the values are stored
                ice::hashmap::set(_actions, action_name_hash, { .type = action.type, .name = *final_name_ptr });
            }
        }

        // Stores the layer along with a ref where it's indices for all sources are stored.
        ice::array::push_back(
            _layers,
            StackLayer{ layer, { ice::u16(layer_sources_offset), ice::u16(ice::count(_layers_sources_indices) - layer_sources_offset) } }
        );
        return S_Ok;
    }

    auto SimpleInputActionStack::active_layers(
        ice::Array<ice::InputActionLayer const*>& out_layers
    ) const noexcept -> ice::u32
    {
        auto it = ice::array::rbegin(_layers_active);
        auto const end = ice::array::rend(_layers_active);

        while (it != end)
        {
            ice::array::push_back(out_layers, _layers[it->index].layer);
            it += 1;
        }
        return ice::count(_layers_active);
    }

    void SimpleInputActionStack::push_layer(
        ice::InputActionLayer const* layer
    ) noexcept
    {
        ice::u32 idx;
        if (ice::search(ice::Span{ _layers }, layer, compare_layers, idx) == false)
        {
            // #TODO: Create a proper error return value.
            ICE_ASSERT_CORE(false);
        }

        // Check if we are already at the top of the stack.
        if (ice::array::any(_layers_active) && ice::array::back(_layers_active).index == idx)
        {
            return; // #TODO: Implement success with info.
        }

        // Push back the new active layer.
        ice::array::push_back(_layers_active, { idx });
    }

    void SimpleInputActionStack::pop_layer(
        ice::InputActionLayer const* layer
    ) noexcept
    {
        ice::u32 idx;
        if (ice::search(ice::Span{ _layers }, layer, compare_layers, idx))
        {
            // We just cut anything below this index, because we want to pop everything up to this layer
            ice::array::resize(_layers, idx);
            ice::array::pop_back(_layers); // And pop the item itself
        }
    }

    auto SimpleInputActionStack::action(ice::String action_name) const noexcept -> ice::Expected<ice::InputAction const*>
    {
        ice::InputActionRuntime const* action = ice::hashmap::try_get(_actions, ice::hash(action_name));
        if (action == nullptr && action_name.starts_with(_idprefix))
        {
            // Try again after removing the prefix
            action = ice::hashmap::try_get(
                _actions,
                ice::hash(
                    action_name.substr(_idprefix.size())
                )
            );
        }
        if (action != nullptr)
        {
            if (action->enabled == false)
            {
                return E_InputActionDisabled;
            }
            else if (action->active == false)
            {
                return E_InputActionInactive;
            }
            else
            {
                return action;
            }
        }
        return E_UnknownInputAction;
    }

    bool SimpleInputActionStack::action_check(
        ice::String action_name,
        ice::InputActionCheck check
    ) const noexcept
    {
        // We check for validity not for success. NOTE: 'operator bool()' checks for success.
        if (ice::Expected const action = this->action(action_name); action.valid())
        {
            switch (check)
            {
                using enum InputActionCheck;
            case Exists: return true;
            case Enabled: return action != E_InputActionDisabled;
            case Disabled: return action == E_InputActionDisabled;
            case Active: return action.succeeded();
            case Inactive: return action == E_InputActionInactive;
            default: ICE_ASSERT_CORE(false); break;
            }
        }
        return false;
    }

    auto SimpleInputActionStack::action_time(
        ice::String action_name
    ) const noexcept -> ice::Tms
    {
        // Check for success
        if (ice::Expected const action = this->action(action_name); action.succeeded())
        {
            return (Tms) ice::clock::elapsed(action.value()->timestamp, ice::clock::now());
        }
        return Tms{ };
    }

    bool SimpleInputActionStack::action_value(
        ice::String action_name,
        ice::vec2f& out_value
    ) const noexcept
    {
        // Check for success
        if (ice::Expected const action = this->action(action_name); action.succeeded())
        {
            out_value = action.value()->value;
            return true;
        }
        return false;
    }

    bool SimpleInputActionStack::action_value(
        ice::String action_name,
        ice::vec2f& out_value,
        ice::Tns& out_timeactive
    ) const noexcept
    {
        // Check for success
        if (ice::Expected const action = this->action(action_name); action.succeeded())
        {
            out_value = action.value()->value;
            out_timeactive = ice::clock::elapsed(action.value()->timestamp, ice::clock::now());
            return true;
        }
        return false;
    }

    void SimpleInputActionStack::process_inputs(
        ice::Span<ice::input::InputEvent const> events
    ) noexcept
    {
        IPT_ZONE_SCOPED;
        using Iterator = ice::Array<ActiveStackLayer>::ConstReverseIterator;

        ice::u32 remaining_events = ice::count(events);
        ice::Array<ice::input::InputEvent> events_copy{ _allocator, events };
        ice::Array<ice::InputActionSource*> source_values{ _allocator };

        // We go in reverse order since, the recently pushed layers should be processed first as they might override inputs.
        Iterator const start = ice::array::rbegin(_layers_active);
        Iterator const end = ice::array::rend(_layers_active);

        for (Iterator it = start; it != end; ++it)
        {
            StackLayer const& layer = _layers[it->index];
            for (ice::u32 offset : ice::array::slice(_layers_sources_indices, layer.sources_indices))
            {
                ice::array::push_back(source_values, ice::addressof(_sources_runtime_values[offset]));
            }

            ice::u32 const processed_events = layer.layer->process_inputs(
                ice::array::slice(events_copy, 0, remaining_events),
                source_values
            );
            ICE_ASSERT_CORE(processed_events <= remaining_events);
            remaining_events -= processed_events;

            ice::array::clear(source_values);

            // TODO: Should we change how this loop is finishing?
            if (remaining_events == 0)
            {
                break;
            }
        }

        ice::InputActionExecutor ex{};
        for (Iterator it = start; it != end; ++it)
        {
            StackLayer const& layer = _layers[it->index];
            for (ice::u32 offset : ice::array::slice(_layers_sources_indices, layer.sources_indices))
            {
                ice::array::push_back(source_values, ice::addressof(_sources_runtime_values[offset]));
            }

            ex.prepare_constants(*layer.layer);
            layer.layer->update_actions(ex, source_values, _actions);

            ice::array::clear(source_values);
        }
    }

    auto SimpleInputActionStack::publish_shards(
        ice::ShardContainer& out_shards
    ) const noexcept -> void
    {
        for (ice::InputActionRuntime const& action : _actions)
        {
            if (action.active)
            {
                ice::ShardID const sid = ice::shardid(action.name);
                switch (action.type)
                {
                    using enum InputActionDataType;
                case Bool:
                    ice::shards::push_back(out_shards, sid | bool(action.value.x > 0.0f));
                    break;
                case Float1:
                    ice::shards::push_back(out_shards, sid | action.value.x);
                    break;
                case Float2:
                    ice::shards::push_back(out_shards, sid | action.value);
                    break;
                case ActionObject:
                    ice::shards::push_back(out_shards, sid | static_cast<ice::InputAction const*>(ice::addressof(action)));
                    break;
                default: ICE_ASSERT_CORE(false); break;
                }
            }
        }
    }

    auto SimpleInputActionStack::action_runtime(
        ice::InputActionLayer const& layer,
        ice::InputActionInfo const& action_info
    ) const noexcept -> ice::InputActionRuntime const&
    {
        static ice::InputActionRuntime invalid{.name="<invalid-action-info>"};
        ice::String const action_name = layer.action_name(action_info);
        return ice::hashmap::get(_actions, ice::hash(action_name), invalid);
    }

    auto SimpleInputActionStack::source_runtime(
        ice::InputActionLayer const& layer,
        ice::InputActionSourceInputInfo const& source_info
    ) const noexcept -> ice::InputActionSource const&
    {
        static ice::InputActionSource invalid{};
        ice::String const source_name = layer.source_name(source_info);

        ice::u32 const values_idx = ice::hashmap::get(
            _sources,
            ice::hash(source_name),
            {.index=ice::u32_max}
        ).index;

        if (values_idx == ice::u32_max)
        {
            return invalid;
        }
        return _sources_runtime_values[values_idx];
    }

    auto create_input_action_stack(
        ice::Allocator& alloc,
        ice::String actionid_prefix
    ) noexcept -> ice::UniquePtr<ice::InputActionStack>
    {
        return ice::make_unique<ice::SimpleInputActionStack>(alloc, alloc, actionid_prefix);
    }

} // namespace ice
