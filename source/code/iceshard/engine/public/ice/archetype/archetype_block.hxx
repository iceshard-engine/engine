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

    namespace detail
    {

        struct ArchetypeDataOperation
        {
            ice::ArchetypeInfo const* source_archetype;
            ice::ArchetypeBlock const* source_block;
            ice::u32 source_index;

            ice::ArchetypeInfo const* destination_archetype;
            ice::ArchetypeBlock const* destination_block;
            ice::u32 destination_index;
        };

        void move_entity_data(
            ArchetypeDataOperation const& operation
        ) noexcept;

        void copy_entity_data(
            ArchetypeDataOperation const& operation
        ) noexcept;

    } // namespace detail

} // namespace ice
