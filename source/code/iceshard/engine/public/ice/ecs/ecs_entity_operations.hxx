/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container/array.hxx>
#include <ice/shard_container.hxx>
#include <ice/ecs/ecs_entity.hxx>
#include <ice/ecs/ecs_archetype.hxx>
#include <ice/ecs/ecs_entity_index.hxx>
#include <ice/ecs/ecs_query_operations.hxx>

namespace ice::ecs
{

    static constexpr ice::Shard Shard_EntityCreated = "event/ecs-v2/entity-created"_shard;
    static constexpr ice::Shard Shard_EntityDestroyed = "event/ecs-v2/entity-destroyed"_shard;
    static constexpr ice::Shard Shard_EntityHandleChanged = "event/ecs-v2/entity-handle-changed"_shard;
    static constexpr ice::Shard Shard_EntityArchetypeChanged = "event/ecs-v2/entity-archetype-changed"_shard;

    struct EntityOperation
    {
        ice::ecs::EntityOperation* next;

        ice::u32 entity_count;
        ice::u32 component_data_size;

        ice::ecs::Archetype archetype;
        ice::ecs::Entity* entities;
        void const* filter_data;
        void* component_data;
    };

    struct OperationComponentInfo
    {
        ice::Span<ice::StringID const> names;
        ice::Span<ice::u32 const> sizes;
        ice::Span<ice::u32 const> offsets;
    };

    struct OperationBuilder
    {
        ice::ecs::EntityOperations& operations;
        ice::ecs::Archetype archetype;
        union
        {
            ice::Span<ice::ecs::Entity const> entities;
            struct // for mode == 1
            {
                ice::ecs::EntityIndex* index;
                ice::u32 index_create_count;
            };
        };
        ice::usize filter_data_size;
        char filter_data[16]; // We assume filter data not requiring more than 16 bytes.
        ice::u32 mode = 0; // Default

        struct Result
        {
            auto one() const noexcept -> ice::ecs::Entity { return ice::span::front(_builder.entities); }
            auto all() const noexcept -> ice::Span<ice::ecs::Entity const> { return _builder.entities; }

            auto store(ice::Array<ice::ecs::Entity>& out_entities, bool append = true) const noexcept
            {
                if (append == false) ice::array::clear(out_entities);
                ice::array::push_back(out_entities, all());
            }

            Result(OperationBuilder& builder) noexcept : _builder{ builder } { }
        private:
            OperationBuilder& _builder;
        };

        OperationBuilder(
            ice::ecs::EntityOperations& operations,
            ice::ecs::Archetype archetype,
            ice::Span<ice::ecs::Entity const> entites
        ) noexcept;

        OperationBuilder(
            ice::ecs::EntityOperations& operations,
            ice::ecs::Archetype archetype,
            ice::ecs::EntityIndex& index,
            ice::ucount entity_count
        ) noexcept;

        ~OperationBuilder() noexcept;

        template<ice::ecs::detail::FilterType T>
        auto with_filter(T const& filter) noexcept -> OperationBuilder&;

        auto with_data(
            ice::ecs::OperationComponentInfo component_info,
            ice::Span<ice::Data const> components_data
        ) noexcept -> Result;

        template<ice::ecs::Component... Components>
        auto with_data(Components const&... components) noexcept -> Result;

        template<ice::ecs::Component... Components>
        auto with_data(ice::Span<Components>&... out_component_spans) noexcept -> Result;

        template<ice::ecs::Component... Components>
        auto with_data(ice::Span<Components const>... component_spans) noexcept -> Result;

        auto finalize() noexcept -> Result;

        auto one() noexcept -> ice::ecs::Entity { return finalize().one(); }

        auto all() noexcept -> ice::Span<ice::ecs::Entity const> { return finalize().all(); }
    };

    class EntityOperations
    {
    public:
        EntityOperations(
            ice::Allocator& alloc,
            ice::ecs::EntityIndex& entities,
            ice::ecs::ArchetypeIndex const& archetypes,
            ice::u32 prealloc = 16
        ) noexcept;

        ~EntityOperations() noexcept;

        void clear() noexcept;
        void grow(ice::u32 count) noexcept;

        auto new_storage_operation() noexcept -> ice::ecs::EntityOperation*;

        auto new_storage_operation(
            ice::meminfo required_data_size,
            void*& out_operation_data_ptr
        ) noexcept -> ice::ecs::EntityOperation*;

        auto set(
            ice::ecs::concepts::ArchetypeRef auto archetype,
            ice::ecs::Entity entity
        ) noexcept -> ice::ecs::OperationBuilder;

        auto set(
            ice::ecs::concepts::ArchetypeRef auto archetype,
            ice::Span<ice::ecs::Entity const> entities
        ) noexcept -> ice::ecs::OperationBuilder;

        auto create(
            ice::ecs::concepts::ArchetypeRef auto archetype,
            ice::u32 count
        ) noexcept -> ice::ecs::OperationBuilder;

        void destroy(
            ice::Span<ice::ecs::Entity const> entities
        ) noexcept;

        void destroy(
            ice::ecs::QueryStorage& queries,
            ice::ecs::concepts::ArchetypeRef auto archetype
        ) noexcept
        {
            ice::ecs::detail::ArchetypeInstanceInfo const* instance = nullptr;
            ice::ecs::detail::DataBlock const* block = nullptr;
            bool const found = queries.query_provider().query_archetype_block(
                this->get_archetype(archetype), instance, block
            );
            if (found)
            {
                while (block != nullptr)
                {
                    if (block->block_entity_count > 0)
                    {
                        ice::ecs::Entity const* entities = reinterpret_cast<ice::ecs::Entity const*>(
                            ice::ptr_add(block->block_data, ice::usize{ instance->component_offsets[0] })
                        );

                        destroy({ entities, block->block_entity_count });

                        // We want to 'unalive' entities when using this method
                        _entities.destroy_many({ entities, block->block_entity_count });
                    }
                    block = block->next;
                }
            }
        }

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
        auto get_archetype(ice::ecs::concepts::ArchetypeRef auto ref) const noexcept -> ice::ecs::Archetype
        {
            if constexpr (std::is_same_v<ice::ecs::Archetype, decltype(ref)>)
            {
                return ref;
            }
            else
            {
                return _archetypes.find_archetype_by_name(ref);
            }
        }

        ice::Allocator& _allocator;
        ice::ecs::EntityIndex& _entities;
        ice::ecs::ArchetypeIndex const& _archetypes;
        ice::ecs::EntityOperation* _root;
        ice::ecs::EntityOperation* _operations;
        ice::ecs::EntityOperation* _free_operations;

        EntityOperationData* _data_nodes;
    };

    void queue_remove_entity(
        ice::ecs::EntityOperations& entity_operations,
        ice::ecs::Entity entity_handle
    ) noexcept;

    inline OperationBuilder::OperationBuilder(
        ice::ecs::EntityOperations& operations,
        ice::ecs::Archetype archetype,
        ice::Span<ice::ecs::Entity const> entities
    ) noexcept
        : operations{ operations }
        , archetype{ archetype }
        , entities{ entities }
        , filter_data_size{ 0_B }
        , filter_data{ }
        , mode{ 1 }
    { }

    inline OperationBuilder::OperationBuilder(
        ice::ecs::EntityOperations& operations,
        ice::ecs::Archetype archetype,
        ice::ecs::EntityIndex& index,
        ice::ucount entity_count
    ) noexcept
        : operations{ operations }
        , archetype{ archetype }
        , index{ ice::addressof(index) }
        , index_create_count{ entity_count }
        , filter_data_size{ 0_B }
        , filter_data{ }
        , mode{ 2 }
    { }

    inline auto EntityOperations::set(
        ice::ecs::concepts::ArchetypeRef auto ref,
        ice::ecs::Entity entity
    ) noexcept -> ice::ecs::OperationBuilder
    {
        return this->set(ref, { &entity, 1 });
    }

    inline auto EntityOperations::set(
        ice::ecs::concepts::ArchetypeRef auto ref,
        ice::Span<ice::ecs::Entity const> entities
    ) noexcept -> ice::ecs::OperationBuilder
    {
        return ice::ecs::OperationBuilder{ *this, this->get_archetype(ref), entities };
    }

    inline auto EntityOperations::create(
        ice::ecs::concepts::ArchetypeRef auto ref,
        ice::ucount count
    ) noexcept -> ice::ecs::OperationBuilder
    {
        return OperationBuilder{ *this, this->get_archetype(ref), _entities, count };
    }

    template<ice::ecs::detail::FilterType T>
    inline auto OperationBuilder::with_filter(T const& filter) noexcept -> OperationBuilder&
    {
        ICE_ASSERT_CORE(sizeof(T) <= sizeof(this->filter_data));
        this->filter_data_size = ice::size_of<T>;
        ice::memcpy(this->filter_data, ice::addressof(filter), sizeof(T));
        return *this;
    }

    template<ice::ecs::Component... Components>
    auto OperationBuilder::with_data(Components const&... components) noexcept -> Result
    {
        static ice::ecs::ArchetypeDefinition<Components...> constexpr HelperArchetype;

        static ice::StaticArray<ice::u32, sizeof...(Components)> constexpr ComponentIdxMap = ice::ecs::detail::make_argument_idx_map<Components...>(
            ice::span::from_std_const(HelperArchetype.component_identifiers)
        );

        static ice::ecs::OperationComponentInfo constexpr ComponentsInfo{
            .names = ice::span::subspan(ice::span::from_std_const(HelperArchetype.component_identifiers), 1),
            .sizes = ice::span::subspan(ice::span::from_std_const(HelperArchetype.component_sizes), 1),
            // We store alignments in this span just for convenience
            .offsets = ice::span::subspan(ice::span::from_std_const(HelperArchetype.component_alignments), 1)
        };

        ice::Data const unsorted_component_Data[]{
            ice::Data{ ice::addressof(components), ice::size_of<Components>, ice::align_of<Components> }...
        };

        ice::StaticArray<ice::Data, sizeof...(Components)> sorted_data_array;
        for (ice::u32 idx = 0; idx < sizeof...(Components); ++idx)
        {
            sorted_data_array[ComponentIdxMap[idx] - 1] = unsorted_component_Data[idx];
        }

        return this->with_data(ComponentsInfo, ice::span::from_std_const(sorted_data_array));
    }

    template<ice::ecs::Component... Components>
    inline auto OperationBuilder::with_data(ice::Span<Components>&... out_component_spans) noexcept -> Result
    {
        if (ice::span::empty(entities) && mode != 2)
        {
            return Result{ *this };
        }

        static ice::ecs::ArchetypeDefinition<Components...> constexpr HelperArchetype;

        static ice::StaticArray<ice::u32, sizeof...(Components)> constexpr ComponentIdxMap = ice::ecs::detail::make_argument_idx_map<Components...>(
                ice::span::from_std_const(HelperArchetype.component_identifiers)
        );

        static ice::ecs::OperationComponentInfo constexpr ComponentsInfo{
            .names = ice::span::subspan(ice::span::from_std_const(HelperArchetype.component_identifiers), 1),
            .sizes = ice::span::subspan(ice::span::from_std_const(HelperArchetype.component_sizes), 1),
            // We store alignments in this span just for convenience
            .offsets = ice::span::subspan(ice::span::from_std_const(HelperArchetype.component_alignments), 1)
        };

        ice::ucount const entity_count = mode == 2 ? index_create_count : ice::count(entities);
        ice::ucount constexpr component_count = sizeof...(Components);
        ice::meminfo additional_data_size = ice::meminfo{ filter_data_size, ice::ualign::b_8 };
        additional_data_size += ice::meminfo_of<ice::ecs::Entity> * entity_count;

        // Data for storing component info
        additional_data_size += ice::meminfo_of<ice::ecs::OperationComponentInfo>;
        additional_data_size.size += ice::span::size_bytes(ComponentsInfo.names);
        additional_data_size.size += ice::span::size_bytes(ComponentsInfo.sizes);
        additional_data_size.size += ice::span::size_bytes(ComponentsInfo.offsets);

        // Use folded expression to calculate all the size for the components...
        additional_data_size.size += ((ice::usize{ alignof(Components) } + ice::size_of<Components> * entity_count) + ...);

        void* operation_data = nullptr;
        ice::ecs::EntityOperation* operation = operations.new_storage_operation(
            additional_data_size,
            operation_data
        );

        ice::memcpy(operation_data, filter_data, ice::usize{ filter_data_size });
        void const* filter_ptr = operation_data;
        operation_data = ice::ptr_add(operation_data, filter_data_size);

        ice::ecs::Entity* entities_ptr = reinterpret_cast<ice::ecs::Entity*>(operation_data);
        if (mode == 1)
        {
            ice::memcpy(entities_ptr, ice::span::data(entities), ice::span::size_bytes(entities));
        }
        else
        {
            ICE_ASSERT_CORE(mode == 2);
            index->create_many({ entities_ptr, index_create_count });
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
        OperationComponentInfo* component_info_ptr;
        {
            component_info_ptr = reinterpret_cast<OperationComponentInfo*>(operation_data);
            component_info_ptr->names = ice::Span<ice::StringID const>{ names_ptr, component_count };
            component_info_ptr->sizes = ice::Span<ice::u32 const>{ sizes_ptr, component_count };
            component_info_ptr->offsets = ice::Span<ice::u32 const>{ offsets_ptr, component_count };
            operation_data = component_info_ptr + 1;
        }

        // We store the given span pointers so update them easily...
        void* span_pointers[]{
            std::addressof(out_component_spans)...
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
            span_ptr->_data = reinterpret_cast<ComponentType*>(data_ptr);
            span_ptr->_count = entity_count;

            // Move to the next data location...
            data_ptr = ice::ptr_add(data_ptr, ice::span::size_bytes(*span_ptr));
            return true;
        };

        using ComponentTypeTuple = std::tuple<Components...>;
        auto const update_spans_helper = [&]<std::size_t... Idx>(std::index_sequence<Idx...> seq) noexcept
        {
            void* operation_data_copy = operation_data;

            [[maybe_unused]]
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

        operation->archetype = archetype;
        operation->entities = entities_ptr;
        operation->entity_count = entity_count;
        operation->component_data = component_info_ptr;
        operation->component_data_size = ice::ucount(ice::ptr_distance(component_info_ptr, component_info_ptr).value);
        operation->filter_data = filter_ptr;

        if (mode == 2)
        {
            entities = { entities_ptr, entity_count };
        }
        mode = 0;

        return { *this };
    }

    template<ice::ecs::Component... Components>
    inline auto OperationBuilder::with_data(ice::Span<Components const>... component_spans) noexcept -> Result
    {
        if (ice::span::empty(entities) && mode != 2)
        {
            return { *this };
        }

        static ice::ecs::ArchetypeDefinition<Components...> constexpr HelperArchetype;

        static ice::StaticArray<ice::u32, sizeof...(Components)> constexpr ComponentIdxMap = ice::ecs::detail::make_argument_idx_map<Components...>(
            ice::span::from_std_const(HelperArchetype.component_identifiers)
        );

        static ice::ecs::OperationComponentInfo constexpr ComponentsInfo{
            .names = ice::span::subspan(ice::span::from_std_const(HelperArchetype.component_identifiers), 1),
            .sizes = ice::span::subspan(ice::span::from_std_const(HelperArchetype.component_sizes), 1),
            // We store alignments in this span just for convenience
            .offsets = ice::span::subspan(ice::span::from_std_const(HelperArchetype.component_alignments), 1)
        };

        ice::ucount const entity_count = mode == 2 ? index_create_count : ice::count(entities);
        ice::ucount constexpr component_count = sizeof...(Components);
        ice::meminfo additional_data_size = ice::meminfo{ filter_data_size, ice::ualign::b_8 };
        additional_data_size += ice::meminfo_of<ice::ecs::Entity> * entity_count;

        // Data for storing component info
        additional_data_size += ice::meminfo_of<ice::ecs::OperationComponentInfo>;
        additional_data_size.size += ice::span::size_bytes(ComponentsInfo.names);
        additional_data_size.size += ice::span::size_bytes(ComponentsInfo.sizes);
        additional_data_size.size += ice::span::size_bytes(ComponentsInfo.offsets);

        // Use folded expression to calculate all the size for the components...
        additional_data_size.size += ((ice::usize{ alignof(Components) } + ice::size_of<Components> * entity_count) + ...);

        void* operation_data = nullptr;
        ice::ecs::EntityOperation* operation = operations.new_storage_operation(
            additional_data_size,
            operation_data
        );

        ice::memcpy(operation_data, filter_data, ice::usize{ filter_data_size });
        void const* filter_ptr = operation_data;
        operation_data = ice::ptr_add(operation_data, filter_data_size);

        ice::ecs::Entity* entities_ptr = reinterpret_cast<ice::ecs::Entity*>(operation_data);
        if (mode == 1)
        {
            ice::memcpy(entities_ptr, ice::span::data(entities), ice::span::size_bytes(entities));
        }
        else
        {
            ICE_ASSERT_CORE(mode == 2);
            index->create_many({ entities_ptr, index_create_count });
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
        OperationComponentInfo* component_info_ptr;
        {
            component_info_ptr = reinterpret_cast<OperationComponentInfo*>(operation_data);
            component_info_ptr->names = ice::Span<ice::StringID const>{ names_ptr, component_count };
            component_info_ptr->sizes = ice::Span<ice::u32 const>{ sizes_ptr, component_count };
            component_info_ptr->offsets = ice::Span<ice::u32 const>{ offsets_ptr, component_count };
            operation_data = component_info_ptr + 1;
        }

        // We store the given span pointers so update them easily...
        void* const span_pointers[]{
            std::addressof(component_spans)...
        };

        auto const update_span_data_helper = [&]<typename ComponentType>(
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
            ice::memcpy(data_ptr, span_ptr->_data, ice::span::size_bytes(*span_ptr));

            // Move to the next data location...
            data_ptr = ice::ptr_add(data_ptr, ice::span::size_bytes(*span_ptr));
            return true;
        };

        using ComponentTypeTuple = std::tuple<Components...>;
        auto const update_spans_data_helper = [&]<std::size_t... Idx>(std::index_sequence<Idx...> seq) noexcept
        {
            void* operation_data_copy = operation_data;

            [[maybe_unused]]
            bool temp[]{
                // [dpenkala: 04/07/2022] We are casting here a nullptr to a type,
                //  so we can use the type in the first helper lambda.
                update_span_data_helper(
                    span_pointers[Idx],
                    operation_data_copy,
                    reinterpret_cast<std::tuple_element_t<Idx, ComponentTypeTuple>*>(0),
                    ComponentIdxMap[Idx] - 1
                )...
            };
        };

        update_spans_data_helper(std::make_index_sequence<component_count>{});

        operation->archetype = archetype;
        operation->entities = entities_ptr;
        operation->entity_count = entity_count;
        operation->component_data = component_info_ptr;
        operation->component_data_size = ice::ucount(ice::ptr_distance(component_info_ptr, component_info_ptr).value);
        operation->filter_data = filter_ptr;

        if (mode == 2)
        {
            entities = { entities_ptr, entity_count };
        }
        mode = 0;

        return { *this };
    }

} // namespace ice::ecs
