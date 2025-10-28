/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/ecs/ecs_entity_index.hxx>
#include <ice/container/array.hxx>
#include <ice/container/queue.hxx>
#include <ice/assert.hxx>

namespace ice::ecs
{

    namespace detail
    {

        auto make_entity(
            ice::u32 index,
            ice::u32 generation
        ) noexcept -> ice::ecs::Entity
        {
            ice::ecs::EntityInfo const info{
                .index = index,
                .generation = generation
            };

            return std::bit_cast<ice::ecs::Entity>(info);
        }

    } // namespace detail

    EntityIndex::EntityIndex(
        ice::Allocator& alloc,
        ice::u32 estimated_entity_count,
        ice::u32 maximum_entity_count /*= ice::u32_max*/
    ) noexcept
        : _allocator{ alloc }
        , _max_entity_count{ maximum_entity_count }
        , _free_indices{ _allocator }
        , _generation{ _allocator }
    {
        ICE_ASSERT(
            estimated_entity_count <= _max_entity_count,
            "Estimated entity count is higher than the maximum allowed number of entities! [ estimated: {}, maximum: {} ]",
            estimated_entity_count,
            _max_entity_count
        );

        ice::array::reserve(_generation, estimated_entity_count);

        // #todo: decide if we need this
        [[maybe_unused]]
        ice::ecs::Entity const initial_entity = create(); // The first entity will be invalid due to how the index works
        ICE_ASSERT(initial_entity == Entity::Invalid, "The definition of an 'Invalid' entity changed!");
        // Ensure this entity is always seen as "not-alive"
        _generation[0] = ice::u8_max;
    }

    auto EntityIndex::count() const noexcept -> ice::u32
    {
        return ice::array::count(_generation) - ice::queue::count(_free_indices);
    }

    bool EntityIndex::is_alive(ice::ecs::Entity entity) const noexcept
    {
        using ice::ecs::EntityInfo;

        EntityInfo const info = ice::ecs::entity_info(entity);
        return ice::count(_generation) > info.index && _generation[info.index] == info.generation;
    }

    auto EntityIndex::create() noexcept -> ice::ecs::Entity
    {
        ice::u32 index = 0;

        if (ice::queue::count(_free_indices) >= ice::ecs::Constant_MinimumFreeIndicesBeforeReuse)
        {
            index = ice::queue::front(_free_indices);
            ice::queue::pop_front(_free_indices);
        }
        else
        {
            index = ice::array::count(_generation);
            ice::array::push_back(_generation, ice::u8{ 0 });
        }

        return ice::ecs::detail::make_entity(index, _generation[index]);
    }

    bool EntityIndex::create_many(ice::Span<ice::ecs::Entity> out_entities) noexcept
    {
        ice::u32 total_indices_taken = 0;
        auto out_it = ice::span::begin(out_entities);

        ice::u32 indices[256];
        ice::i32 free_count = ice::i32(ice::queue::count(_free_indices)) - ice::ecs::Constant_MinimumFreeIndicesBeforeReuse;
        while(free_count > 0)
        {
            ice::u32 const indices_taken = ice::queue::take_front(
                _free_indices,
                ice::span::subspan(ice::Span{indices}, 0, ice::min<ice::u32>(free_count, ice::count(indices)))
            );

            for (ice::u32 idx = 0; idx < indices_taken; ++idx)
            {
                *out_it = detail::make_entity(indices[idx], _generation[indices[idx]]);
                out_it += 1;
            }

            free_count -= indices_taken;
            total_indices_taken += indices_taken;
        }

        ice::u32 gen_index = ice::array::count(_generation);
        ice::u32 const missing_entities = ice::count(out_entities) - total_indices_taken;
        ice::u32 const final_index = gen_index + missing_entities;

        if (final_index > 0)
        {
            ICE_ASSERT(
                final_index < _max_entity_count,
                "Moved past the maximum allowed number of entities!"
            );

            ice::array::resize(_generation, final_index);
        }

        while (gen_index < final_index)
        {
            _generation[gen_index] = ice::u8{ 0 };

            *out_it = detail::make_entity(gen_index, 0);

            gen_index += 1;
            out_it += 1;
        }
        return true;
    }

    void EntityIndex::destroy(ice::ecs::Entity entity) noexcept
    {
        using ice::ecs::EntityInfo;

        EntityInfo const info = ice::ecs::entity_info(entity);
        _generation[info.index] += 1;

        ice::queue::push_back(_free_indices, info.index);
    }

    void EntityIndex::destroy_many(ice::Span<ice::ecs::Entity const> entities) noexcept
    {
        for (ice::ecs::Entity entity : entities)
        {
            this->destroy(entity);
        }
    }

    bool EntityIndex::recreate(ice::Array<ice::ecs::Entity>& entities, ice::u32 new_count) noexcept
    {
        destroy_many(entities);
        ice::array::resize(entities, new_count);
        create_many(entities);
        return false;
    }

} // namespace ice::ecs
