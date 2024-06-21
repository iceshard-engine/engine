/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container/array.hxx>
#include <ice/shard_container.hxx>
#include <ice/ecs/ecs_entity.hxx>
#include <ice/ecs/ecs_archetype.hxx>

namespace ice::ecs
{

    static constexpr ice::Shard Shard_EntityCreated = "event/ecs-v2/entity-created"_shard;
    static constexpr ice::Shard Shard_EntityDestroyed = "event/ecs-v2/entity-destroyed"_shard;
    static constexpr ice::Shard Shard_EntityHandleChanged = "event/ecs-v2/entity-handle-changed"_shard;
    static constexpr ice::Shard Shard_EntityArchetypeChanged = "event/ecs-v2/entity-archetype-changed"_shard;

    struct EntityOperation
    {
        ice::ecs::EntityOperation* next;

        ice::u32 entity_count : 31;
        ice::u32 notify_entity_changes : 1;
        ice::u32 component_data_size;

        ice::ecs::Archetype archetype;
        ice::ecs::EntityHandle* entities;
        void* component_data;
    };

    class EntityOperations
    {
    public:
        EntityOperations(ice::Allocator& alloc, ice::u32 prealloc = 16) noexcept;
        ~EntityOperations() noexcept;

        void clear() noexcept;
        void grow(ice::u32 count) noexcept;

        auto new_storage_operation() noexcept -> ice::ecs::EntityOperation*;

        auto new_storage_operation(
            ice::meminfo required_data_size,
            void*& out_operation_data_ptr
        ) noexcept -> ice::ecs::EntityOperation*;

        struct ComponentInfo
        {
            ice::Span<ice::StringID const> names;
            ice::Span<ice::u32 const> sizes;
            ice::Span<ice::u32 const> offsets;
        };

        struct EntityOperationData;
        struct OperationIterator
        {
            auto operator*() const noexcept -> EntityOperation const&;
            bool operator==(OperationIterator const& other) const noexcept;
            bool operator!=(OperationIterator const& other) const noexcept;
            auto operator++() noexcept -> OperationIterator&;

            ice::ecs::EntityOperation const* _entry;
        };

        auto begin() const noexcept -> OperationIterator;
        auto end() const noexcept -> OperationIterator;

    private:
        ice::Allocator& _allocator;
        ice::ecs::EntityOperation* _root;
        ice::ecs::EntityOperation* _operations;
        ice::ecs::EntityOperation* _free_operations;

        EntityOperationData* _data_nodes;
    };

    void queue_set_archetype(
        ice::ecs::EntityOperations& entity_operations,
        ice::ecs::Entity entity,
        ice::ecs::Archetype archetype
    ) noexcept;

    void queue_set_archetype(
        ice::ecs::EntityOperations& entity_operations,
        ice::Span<ice::ecs::Entity const> entities,
        ice::ecs::Archetype archetype,
        bool notify_changes = false
    ) noexcept;

    void queue_set_archetype_with_data(
        ice::ecs::EntityOperations& entity_operations,
        ice::Span<ice::ecs::Entity const> entities,
        ice::ecs::Archetype archetype,
        ice::ecs::EntityOperations::ComponentInfo component_info,
        ice::Span<ice::Data> component_data,
        bool notify_changes = false
    ) noexcept;

    template<ice::ecs::Component... Components>
    void queue_set_archetype_with_data(
        ice::ecs::EntityOperations& entity_operations,
        ice::ecs::Entity entity,
        ice::ecs::Archetype archetype,
        Components const&... components
    ) noexcept;

    template<ice::ecs::Component... Components>
    void queue_set_archetype_with_data(
        ice::ecs::EntityOperations& entity_operations,
        ice::Span<ice::ecs::Entity const> entities,
        ice::ecs::Archetype archetype,
        ice::Span<Components>&... components
    ) noexcept;

    void queue_remove_entity(
        ice::ecs::EntityOperations& entity_operations,
        ice::ecs::EntityHandle entity_handle
    ) noexcept;

    void queue_batch_remove_entities(
        ice::ecs::EntityOperations& entity_operations,
        ice::Span<ice::ecs::EntityHandle const> entities
    ) noexcept;


    template<ice::ecs::Component... Components>
    void queue_set_archetype_with_data(
        ice::ecs::EntityOperations& entity_operations,
        ice::ecs::Entity entity,
        ice::ecs::Archetype archetype,
        Components const&... components
    ) noexcept
    {
        constexpr ice::ecs::ArchetypeDefinition<Components...> const& pseudo_archetype_definition = ice::ecs::Constant_ArchetypeDefinition<Components...>;

        constexpr ice::StaticArray<ice::u32, sizeof...(Components)> const idx_map = ice::ecs::detail::argument_idx_map<Components...>(
            ice::span::from_std_const(pseudo_archetype_definition.component_identifiers)
        );

        constexpr ice::ecs::EntityOperations::ComponentInfo const component_info{
            .names = ice::span::subspan(ice::span::from_std_const(pseudo_archetype_definition.component_identifiers), 1),
            .sizes = ice::span::subspan(ice::span::from_std_const(pseudo_archetype_definition.component_sizes), 1),
            // We can store alignments here instead of offsets.
            .offsets = ice::span::subspan(ice::span::from_std_const(pseudo_archetype_definition.component_alignments), 1)
        };

        ice::Data const component_data_array_unsorted[]{
            ice::Data{ ice::addressof(components), ice::size_of<Components>, ice::align_of<Components> }...
        };

        ice::StaticArray<ice::Data, sizeof...(Components)> data_array;
        for (ice::u32 idx = 0; idx < sizeof...(Components); ++idx)
        {
            data_array[idx_map[idx] - 1] = component_data_array_unsorted[idx];
        }

        ice::ecs::queue_set_archetype_with_data(
            entity_operations,
            ice::Span<ice::ecs::Entity const>{ &entity, 1 },
            archetype,
            component_info,
            ice::span::from_std(data_array),
            true
        );
    }

    template<ice::ecs::Component... Components>
    void queue_set_archetype_with_data(
        ice::ecs::EntityOperations& entity_operations,
        ice::Span<ice::ecs::Entity const> entities,
        ice::ecs::Archetype archetype,
        ice::Span<Components>& ...component_spans
    ) noexcept
    {
        static ice::ecs::ArchetypeDefinition<Components...> constexpr HelperArchetype;

        static ice::StaticArray<ice::u32, sizeof...(Components)> constexpr ComponentIdxMap = ice::ecs::detail::argument_idx_map<Components...>(
            ice::span::from_std_const(HelperArchetype.component_identifiers)
        );

        static ice::ecs::EntityOperations::ComponentInfo constexpr ComponentsInfo{
            .names = ice::span::subspan(ice::span::from_std_const(HelperArchetype.component_identifiers), 1),
            .sizes = ice::span::subspan(ice::span::from_std_const(HelperArchetype.component_sizes), 1),
            .offsets = ice::span::subspan(ice::span::from_std_const(HelperArchetype.component_alignments), 1)
        };

        ice::ucount const entity_count = ice::count(entities);
        ice::ucount constexpr component_count = sizeof...(Components);
        ice::meminfo additional_data_size = ice::meminfo_of<ice::ecs::EntityHandle> * entity_count;

        // Data for storing component info
        additional_data_size.size += ice::size_of<ice::ecs::EntityOperations::ComponentInfo>;
        additional_data_size.size += ice::span::size_bytes(ComponentsInfo.names);
        additional_data_size.size += ice::span::size_bytes(ComponentsInfo.sizes);
        additional_data_size.size += ice::span::size_bytes(ComponentsInfo.offsets);

        // Use folded expression to calculate all the size for the components...
        additional_data_size.size += ((ice::usize{ alignof(Components) } + ice::size_of<Components> * entity_count) + ...);

        void* operation_data = nullptr;
        ice::ecs::EntityOperation* operation = entity_operations.new_storage_operation(
            additional_data_size,
            operation_data
        );

        // Set entity handles
        auto constexpr make_entity_handle = [](ice::ecs::Entity entity) noexcept -> ice::ecs::EntityHandle
        {
            ice::ecs::EntityHandleInfo const info{
                .entity = entity,
                .slot = ice::ecs::EntitySlot::Invalid
            };
            return std::bit_cast<ice::ecs::EntityHandle>(info);
        };

        ice::ecs::EntityHandle* entities_ptr = reinterpret_cast<ice::ecs::EntityHandle*>(operation_data);
        for (ice::u32 idx = 0; idx < entity_count; ++idx)
        {
            entities_ptr[idx] = make_entity_handle(entities[idx]);
        }

        // Set component info object
        ice::StringID* names_ptr = reinterpret_cast<ice::StringID*>(entities_ptr + entity_count);
        ice::memcpy(names_ptr, ice::span::data(ComponentsInfo.names), ice::span::size_bytes(ComponentsInfo.names));

        ice::u32* sizes_ptr = reinterpret_cast<ice::u32*>(names_ptr + component_count);
        ice::memcpy(sizes_ptr, ice::span::data(ComponentsInfo.sizes), ice::span::size_bytes(ComponentsInfo.sizes));

        ice::u32* offsets_ptr = reinterpret_cast<ice::u32*>(sizes_ptr + component_count);

        // We update now the operation data pointer to where we store the component info object.
        //  We will calculate data offsets from here too.
        operation_data = offsets_ptr + component_count;

        // Set the component info object with the above pointers.
        EntityOperations::ComponentInfo* component_info_ptr;
        {
            component_info_ptr = reinterpret_cast<EntityOperations::ComponentInfo*>(operation_data);
            component_info_ptr->names = ice::Span<ice::StringID const>{ names_ptr, component_count };
            component_info_ptr->sizes = ice::Span<ice::u32 const>{ sizes_ptr, component_count };
            component_info_ptr->offsets = ice::Span<ice::u32 const>{ offsets_ptr, component_count };
            operation_data = component_info_ptr + 1;
        }

        // We store the given span pointers so update them easily...
        void* const span_pointers[]{
            std::addressof(component_spans)...
        };

        auto const update_span_helper = [&]<typename ComponentType>(
            void* span_raw_ptr,
            void*& data_ptr,
            ComponentType*,
            ice::u32 offset_idx
        ) noexcept
        {
            using SpanType = ice::Span<ComponentType>;

            data_ptr = ice::align_to(data_ptr, ice::align_of<ComponentType>).value;
            offsets_ptr[offset_idx] = ice::u32(ice::ptr_distance(operation_data, data_ptr).value);

            // Update the span object...
            SpanType* span_ptr = reinterpret_cast<SpanType*>(span_raw_ptr);
            *span_ptr = SpanType{ reinterpret_cast<ComponentType*>(data_ptr), entity_count };

            // Move to the next data location...
            data_ptr = ice::ptr_add(data_ptr, ice::span::size_bytes(*span_ptr));
            return true;
        };

        using ComponentTypeTuple = std::tuple<Components...>;
        auto const update_spans_helper = [&]<std::size_t... Idx>(std::index_sequence<Idx...> seq) noexcept
        {
            void* operation_data_copy = operation_data;

            bool temp[]{
                // [dpenkala: 04/07/2022] We are casting here a nullptr to a type,
                //  so we can use the type in the first helper lambda.
                update_span_helper(
                    span_pointers[Idx],
                    operation_data_copy,
                    reinterpret_cast<std::tuple_element_t<Idx, ComponentTypeTuple>*>(0),
                    ComponentIdxMap[Idx] - 1
                )...
            };
        };

        update_spans_helper(std::make_index_sequence<component_count>{});

        operation->archetype = HelperArchetype;
        operation->entities = entities_ptr;
        operation->entity_count = entity_count;
        operation->notify_entity_changes = false;
        operation->component_data = component_info_ptr;
        operation->component_data_size = ice::ucount(ice::ptr_distance(component_info_ptr, component_info_ptr).value);
    }

} // namespace ice::ecs
