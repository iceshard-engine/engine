#pragma once
#include <ice/data.hxx>

namespace ice
{

    struct ArchetypeInfo;

    struct ArchetypeBlock
    {
        ice::u32 entity_count_max = 0;
        ice::u32 entity_count = 0;
        ice::u32 block_size = 0;
        void* block_data = nullptr;

        ice::ArchetypeBlock* next = nullptr;
    };

    struct ArchetypeOperation
    {
        ice::ArchetypeInfo const* source_archetype;
        ice::Data source_data;
        ice::u32 source_data_offset;
        ice::u32 source_entity_count;
    };

    namespace detail
    {

        struct ArchetypeConstBlock
        {
            ice::u32 const entity_count = 0;
            ice::u32 const block_size = 0;
            void const* block_data = nullptr;
        };

        struct EntityDataOperation
        {
            ice::ArchetypeInfo const* source_archetype;
            ice::ArchetypeBlock const* source_block;
            ice::u32 source_index;

            ice::ArchetypeInfo const* destination_archetype;
            ice::ArchetypeBlock const* destination_block;
            ice::u32 destination_index;
        };

        void move_entity_data(
            EntityDataOperation const& operation
        ) noexcept;

        void copy_entity_data(
            EntityDataOperation const& operation
        ) noexcept;

        struct ArchetypeDataOperation
        {
            ice::ArchetypeInfo const* source_archetype;
            ice::detail::ArchetypeConstBlock const* source_block;
            ice::u32 source_offset;
            ice::u32 source_count;

            ice::ArchetypeInfo const* destination_archetype;
            ice::ArchetypeBlock const* destination_block;
            ice::u32 destination_offset;
            ice::u32 destination_count;
        };

        auto copy_archetype_data(
            ArchetypeDataOperation const& operation
        ) noexcept -> ArchetypeDataOperation;

    } // namespace detail

} // namespace ice
