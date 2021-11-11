#include <ice/ecs/ecs_entity_index.hxx>
#include <ice/pod/array.hxx>
#include <ice/pod/queue.hxx>
#include <ice/assert.hxx>

namespace ice::ecs
{

    namespace detail
    {

        struct EntityInfo
        {
            ice::u32 index : 24;
            ice::u32 generation : 8;
        };

        auto entity_info(
            ice::ecs::Entity entity
        ) noexcept -> ice::ecs::detail::EntityInfo
        {
            union
            {
                ice::ecs::Entity entity;
                ice::ecs::detail::EntityInfo info;
            } const helper{ .entity = entity };

            return helper.info;
        }

        auto make_entity(
            ice::u32 index,
            ice::u32 generation
        ) noexcept -> ice::ecs::Entity
        {
            union
            {
                ice::ecs::detail::EntityInfo info;
                ice::ecs::Entity entity;
            } const helper{
                .info = {
                    .index = index,
                    .generation = generation
                }
            };

            return helper.entity;
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

        ice::pod::array::reserve(_generation, estimated_entity_count);

        // #todo: decide if we need this
        //[[maybe_unused]]
        //ice::ecs::Entity const initial_entity = create(); // The first entity will be invalid due to how the index works
        //ICE_ASSERT(initial_entity == Entity::Invalid, "The definition of an 'Invalid' entity changed!");
    }

    auto EntityIndex::count() const noexcept -> ice::u32
    {
        return ice::pod::array::size(_generation) - ice::pod::queue::size(_free_indices);
    }

    bool EntityIndex::is_alive(ice::ecs::Entity entity) const noexcept
    {
        using ice::ecs::detail::EntityInfo;

        EntityInfo const info = ice::ecs::detail::entity_info(entity);
        return _generation[info.index] == info.generation;
    }

    auto EntityIndex::create() noexcept -> ice::ecs::Entity
    {
        ice::u32 index = 0;

        if (ice::pod::queue::size(_free_indices) >= ice::ecs::Constant_MinimumFreeIndicesBeforeReuse)
        {
            index = ice::pod::queue::front(_free_indices);
            ice::pod::queue::pop_front(_free_indices);
        }
        else
        {
            index = ice::pod::array::size(_generation);
            ice::pod::array::push_back(_generation, ice::u8{ 0 });
        }

        return ice::ecs::detail::make_entity(index, _generation[index]);
    }

    bool EntityIndex::create_many(ice::Span<ice::ecs::Entity> out_entities) noexcept
    {
        // #todo: For now when creating entities in bulk. We skip the check of _free_indices.
        ice::u32 index = ice::pod::array::size(_generation);
        ice::u32 const final_index = index + ice::size(out_entities);

        ICE_ASSERT(
            final_index < _max_entity_count,
            "Moved past the maximum allowed number of entities!"
        );

        ice::pod::array::resize(_generation, final_index);

        auto out_it = out_entities.begin();
        while (index < final_index)
        {
            _generation[index] = ice::u8{ 0 };

            *out_it = detail::make_entity(index, 0);

            index += 1;
            out_it += 1;
        }
        return true;
    }

    void EntityIndex::destroy(ice::ecs::Entity entity) noexcept
    {
        using ice::ecs::detail::EntityInfo;

        EntityInfo const info = ice::ecs::detail::entity_info(entity);
        _generation[info.index] += 1;

        ice::pod::queue::push_back(_free_indices, info.index);
    }

    void EntityIndex::destroy_many(ice::Span<ice::ecs::Entity> entities) noexcept
    {
        for (ice::ecs::Entity entity : entities)
        {
            this->destroy(entity);
        }
    }

} // namespace ice::ecs
