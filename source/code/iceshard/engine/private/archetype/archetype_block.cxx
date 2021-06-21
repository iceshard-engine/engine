#include <ice/archetype/archetype_block.hxx>
#include <ice/archetype/archetype_info.hxx>
#include <ice/entity/entity.hxx>
#include <ice/memory/pointer_arithmetic.hxx>
#include <ice/assert.hxx>

namespace ice::detail
{

    auto get_entity_array(
        ice::ArchetypeInfo const* archetype_info,
        ice::ArchetypeBlock const* block
    ) noexcept -> ice::Span<ice::Entity>
    {
        void* const entity_array_ptr = ice::memory::ptr_add(
            block->block_data,
            archetype_info->offsets[0]
        );

        return ice::Span<ice::Entity>(
            reinterpret_cast<ice::Entity*>(block->block_data),
            block->entity_count
        );
    }

    void move_entity_data(ArchetypeDataOperation const& operation) noexcept
    {
        ice::ArchetypeInfo const* src_archetype = operation.source_archetype;

        ice::u32 const component_count = ice::size(src_archetype->components);

        // Iterate over each component in the source archetype
        for (ice::u32 component_idx = 0; component_idx < component_count; ++component_idx)
        {
            ice::u32 const size = src_archetype->sizes[component_idx];
            ice::u32 const src_offset = src_archetype->offsets[component_idx] + size * operation.source_index;
            ice::u32 const dst_offset = src_archetype->offsets[component_idx] + size * operation.destination_index;

            void* const src_ptr = ice::memory::ptr_add(operation.source_block->block_data, src_offset);
            void* const dst_ptr = ice::memory::ptr_add(operation.destination_block->block_data, dst_offset);

            // Do a plain copy (we require components to be trivially copyable)
            ice::memcpy(dst_ptr, src_ptr, size);
        }

        ice::Span<ice::Entity> entities = get_entity_array(operation.source_archetype, operation.source_block);
        entities[operation.source_index] = Entity{ };
    }

    void copy_entity_data(ArchetypeDataOperation const& operation) noexcept
    {
        ice::ArchetypeInfo const* src_archetype = operation.source_archetype;
        ice::ArchetypeInfo const* dst_archetype = operation.destination_archetype;

        ice::u32 const src_component_count = ice::size(src_archetype->components);
        ice::u32 const dst_component_count = ice::size(dst_archetype->components);
        ice::u32 const max_component_count = ice::max(src_component_count, dst_component_count);
        bool const source_is_smaller = src_component_count < max_component_count;

        ice::u32 src_component_index = 0;
        ice::u32 dst_component_index = 0;

        ice::u32& main_component_index = (source_is_smaller ? dst_component_index : src_component_index);
        ice::u32& sub_component_index = (source_is_smaller ? src_component_index : dst_component_index);

        // Iterate over each component in the source archetype
        for (; main_component_index < max_component_count && src_component_index < src_component_count; ++main_component_index)
        {
            ice::u32 const dst_size = dst_archetype->sizes[dst_component_index];
            ice::u32 const dst_offset = dst_archetype->offsets[dst_component_index] + dst_size * operation.destination_index;

            // If components do not match, skip the copy and clear memory
            if (src_archetype->components[src_component_index] != dst_archetype->components[dst_component_index])
            {
                void* const ptr = ice::memory::ptr_add(operation.destination_block->block_data, dst_offset);

                ice::memset(ptr, '\0', dst_size);
                continue;
            }

            ice::u32 const src_size = src_archetype->sizes[src_component_index];
            ice::u32 const src_offset = src_archetype->offsets[src_component_index] + src_size * operation.source_index;

            ICE_ASSERT(
                src_size == dst_size,
                "Mismatched data size {} != {} for components with the same ID. source: {} ({}), destination: {} ({})",
                src_size, dst_size,
                ice::stringid_hint(src_archetype->components[src_component_index]),
                ice::hash(src_archetype->components[src_component_index]),
                ice::stringid_hint(src_archetype->components[dst_component_index]),
                ice::hash(src_archetype->components[dst_component_index])
            );

            void* const src_ptr = ice::memory::ptr_add(operation.source_block->block_data, src_offset);
            void* const dst_ptr = ice::memory::ptr_add(operation.destination_block->block_data, dst_offset);

            // Do a plain copy (we require components to be standard layout and trivially copyable)
            ice::memcpy(dst_ptr, src_ptr, src_size);

            // Increment the sub index
            sub_component_index += 1;
        }
    }

} // namespace ice::detail
