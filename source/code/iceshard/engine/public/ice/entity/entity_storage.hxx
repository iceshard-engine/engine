#pragma once
#include <ice/archetype/archetype_index.hxx>
#include <ice/archetype/archetype_block_allocator.hxx>
#include <ice/entity/entity.hxx>

namespace ice
{

    enum class EntityInstance : ice::u64
    {
        Invalid = 0
    };

    class EntityStorage
    {
    public:
        EntityStorage(
            ice::Allocator& alloc,
            ice::ArchetypeIndex& archetype_index,
            ice::ArchetypeBlockAllocator& archetype_block_alloc
        ) noexcept;
        ~EntityStorage() noexcept;

        auto archetype_index() noexcept -> ice::ArchetypeIndex&;

        void set_archetype(
            ice::Entity entity,
            ice::ArchetypeHandle archetype
        ) noexcept;

        void set_archetypes(
            ice::ArchetypeHandle archetype,
            ice::Span<ice::Entity const> entities,
            ice::ArchetypeOperation operation
        ) noexcept;

        template<EntityComponent... Components>
        inline void set_archetype_with_data(
            ice::Entity entity,
            ice::ArchetypeHandle archetype,
            Components const&... values
        ) noexcept;

        void change_archetype(
            ice::Entity entity,
            ice::ArchetypeHandle archetype
        ) noexcept;


        void set_components(
            ice::Entity entity,
            ice::Span<ice::ArchetypeComponent const> components
        ) noexcept;

        void add_component(
            ice::Entity entity,
            ice::ArchetypeComponent component
        ) noexcept;

        void remove_component(
            ice::Entity entity,
            ice::StringID_Arg component_name
        ) noexcept;

        template<typename... Components>
        void set_components(
            ice::Entity entity
        ) noexcept;

        template<typename Component>
        void add_component(
            ice::Entity entity
        ) noexcept;

        template<typename Component>
        void remove_component(
            ice::Entity entity
        ) noexcept;


        void erase_data(
            ice::Entity entity
        ) noexcept;

        void query_blocks(
            ice::Span<ice::ArchetypeHandle const> archetypes,
            ice::pod::Array<ice::u32>& archetype_block_count,
            ice::pod::Array<ice::ArchetypeBlock*>& archetype_blocks
        ) noexcept;

        //bool query_entity(
        //    ice::Entity entity,
        //    ice::ArchetypeInfo* archetype_info_out,
        //    ice::ArchetypeBlock* entity_block_out,
        //    ice::u32 entity_index_out
        //) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::ArchetypeIndex& _archetype_index;
        ice::pod::Hash<ice::ArchetypeBlock*> _archetype_blocks;

        ice::pod::Hash<ice::EntityInstance> _instances;
    };

    template<EntityComponent... Components>
    inline void EntityStorage::set_archetype_with_data(
        ice::Entity entity,
        ice::ArchetypeHandle archetype,
        Components const&... values
    ) noexcept
    {

        if constexpr (sizeof...(values) == 0)
        {
            set_archetype(entity, archetype);
        }
        else
        {
            constexpr auto sum_of_values = [](auto const& range) noexcept -> ice::u32
            {
                ice::u32 sum = 0;
                for (ice::u32 value : range)
                {
                    sum += value;
                }
                return sum;
            };

            constexpr ice::Archetype<Components...> archetype_component_info;
            constexpr ice::u32 component_count = sizeof...(Components);
            constexpr ice::StringID unsorted_component_names[]{ Components::Identifier... };
            constexpr ice::u32 unsorted_component_sizes[]{ sizeof(Components)... };
            constexpr ice::u32 unsorted_component_alignments[]{ alignof(Components)... };

            ice::StringID component_names[component_count]{ };
            ice::u32 component_sizes[component_count]{ };
            ice::u32 component_alignments[component_count]{ };
            ice::u32 component_offsets[component_count]{ };

            ice::u32 component_idx_mapping[component_count];

            {
                for (ice::u32 idx = 0; idx < component_count; ++idx)
                {
                    // We skip the initial component which will always be the 'ice::Entity' object.
                    ice::ArchetypeComponent const& component = archetype_component_info.components[idx + 1];

                    component_names[idx] = ice::StringID{ component.name };
                    component_sizes[idx] = component.size;
                    component_alignments[idx] = component.alignment;

                    component_idx_mapping[idx] = 0;
                    for (ice::StringID_Arg component_arg : unsorted_component_names)
                    {
                        if (component_arg.hash_value == component.name)
                        {
                            break;
                        }

                        component_idx_mapping[idx] += 1;
                    }
                }
            }

            constexpr ice::u32 required_buffer_size = sum_of_values(unsorted_component_sizes) + sum_of_values(unsorted_component_alignments);
            ice::u8 buffer[required_buffer_size];

            {
                ice::u32 current_offset = 0;
                for (ice::u32 idx = 0; idx < component_count; ++idx)
                {
                    void* candidate_pointer = buffer + current_offset;

                    ice::uptr const pointer_value = reinterpret_cast<ice::uptr>(candidate_pointer);
                    if (ice::uptr const mod = pointer_value % component_alignments[idx]; mod != 0)
                    {
                        current_offset += (component_alignments[idx] - mod);
                    }

                    component_offsets[idx] = current_offset;
                    current_offset += component_sizes[idx];
                }

                ice::u32 idx = 0;
                ice::u32 helper_list[]{
                    (*reinterpret_cast<Components*>(buffer + component_offsets[component_idx_mapping[idx]]) = values, idx++)...,
                };
            }

            ice::ArchetypeInfo const archetype_info{
                .block_allocator = nullptr,
                .block_base_alignment = 0,
                .block_max_entity_count = 1,
                .components = component_names,
                .sizes = component_sizes,
                .alignments = component_alignments,
                .offsets = component_offsets,
            };

            ice::ArchetypeOperation const data_operation{
                .source_archetype = &archetype_info,
                .source_data = ice::data_view(buffer, required_buffer_size, alignof(ice::u32)),
                .source_data_offset = 0,
                .source_entity_count = 1
            };

            ice::Entity const entity_list[1]{ entity };
            set_archetypes(archetype, entity_list, data_operation);
        }
    }

    template<typename... Components>
    void EntityStorage::set_components(
        ice::Entity entity
    ) noexcept
    {
        static constexpr ice::Archetype<Components...> temp_archetype{ };
        set_components(entity, temp_archetype.components);
    }

    template<typename Component>
    void EntityStorage::add_component(
        ice::Entity entity
    ) noexcept
    {
        static constexpr ArchetypeComponent component_info{
            .name = ice::stringid_hash(ComponentIdentifier<Component>),
            .size = sizeof(Component),
            .alignment = alignof(Component),
        };

        add_component(entity, component_info);
    }

    template<typename Component>
    void EntityStorage::remove_component(
        ice::Entity entity
    ) noexcept
    {
        remove_component(entity, ComponentIdentifier<Component>);
    }

} // namespace ice
