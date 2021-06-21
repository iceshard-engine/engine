#include <ice/entity/entity_index.hxx>
#include <ice/pod/queue.hxx>
#include <ice/pod/array.hxx>
#include <ice/assert.hxx>

namespace ice
{

    namespace detail
    {

        static constexpr ice::u32 minimum_free_indices_before_reuse = 1024;

        union EntityHelper
        {
            Entity handle;
            EntityInfo info;
        };

        auto get_entity_info(ice::Entity entity) noexcept -> ice::EntityInfo
        {
            EntityHelper const helper{ .handle = entity };
            return helper.info;
        }

        auto make_entity(ice::EntityInfo info) noexcept -> ice::Entity
        {
            EntityHelper const helper{ .info = info };
            return helper.handle;
        }

    } // namespace detail

    EntityIndex::EntityIndex(
        ice::Allocator& alloc,
        ice::u32 estimated_entity_count,
        ice::u32 maximum_entity_count /*= std::numeric_limits<ice::u32>::max()*/
    ) noexcept
        : _allocator{ alloc }
        , _max_entity_count{ maximum_entity_count }
        , _free_indices{ _allocator }
        , _generation{ _allocator }
        , _owner{ _allocator }
    {
        ICE_ASSERT(
            estimated_entity_count <= _max_entity_count,
            "Estimated entity count is higher than the maximum allowed number of entities! [ estimated: {}, maximum: {} ]",
            estimated_entity_count,
            _max_entity_count
        );

        ice::pod::array::reserve(_generation, estimated_entity_count);
        ice::pod::array::reserve(_owner, estimated_entity_count);
    }

    auto EntityIndex::count() noexcept -> ice::u32
    {
        return ice::pod::array::size(_generation) - ice::pod::queue::size(_free_indices);
    }

    bool EntityIndex::is_alive(ice::Entity entity) noexcept
    {
        EntityInfo const info = detail::get_entity_info(entity);
        return _generation[info.index] == info.generation;
    }

    auto EntityIndex::create(void const* owner) noexcept -> ice::Entity
    {
        ice::u32 index = 0;

        if (ice::pod::queue::size(_free_indices) >= detail::minimum_free_indices_before_reuse)
        {
            index = ice::pod::queue::front(_free_indices);
            ice::pod::queue::pop_front(_free_indices);
        }
        else
        {
            index = ice::pod::array::size(_generation);
            ice::pod::array::push_back(_generation, ice::u16{ 0 });
            ice::pod::array::push_back(_owner, owner);
        }

        return detail::make_entity(
            EntityInfo{
                .index = index,
                .generation = _generation[index],
                .reserved = 0
            }
        );
    }

    bool EntityIndex::create_many(
        ice::Span<ice::Entity> values_out,
        void const* owner
    ) noexcept
    {
        // For now when creating entities in bulk, we skip the check of _free_indices.
        ice::u32 index = ice::pod::array::size(_generation);
        ice::u32 const final_index = index + static_cast<ice::u32>(values_out.size());

        // #todo Check for maximum allowed number of entities

        // Resize the generation and owner arrays to hold at least the `end_index`
        ice::pod::array::resize(_generation, final_index);
        ice::pod::array::resize(_owner, final_index);

        auto out_it = values_out.begin();

        // Set the generation from 'start_index' to 'end_index' to 0.
        while (index < final_index)
        {
            _generation[index] = ice::u16{ 0 };
            _owner[index] = owner;

            // Add the resulting entity to the result array.
            *out_it = detail::make_entity(
                EntityInfo{
                    .index = index,
                    .generation = 0,
                    .reserved = 0,
                }
            );

            // Advance the index and iterator
            index += 1;
            out_it += 1;
        }

        return true;
    }

    void EntityIndex::destroy(ice::Entity entity) noexcept
    {
        EntityInfo const info = detail::get_entity_info(entity);
        _owner[info.index] = nullptr;
        _generation[info.index] += 1;
        ice::pod::queue::push_back(_free_indices, info.index);
    }

    auto EntityIndex::count_owned(void const* owner) noexcept -> ice::u32
    {
        if (owner == nullptr) [[unlikely]]
        {
            return 0;
        }

        ice::u32 result = 0;
        for (void const* entity_owner : _owner)
        {
            if (entity_owner == owner)
            {
                result += 1;
            }
        }
        return result;
    }

    void EntityIndex::get_owned(
        void const* owner,
        ice::pod::Array<ice::Entity>& entities_out
    ) noexcept
    {
        ice::u32 const size = ice::pod::array::size(_owner);
        for (ice::u32 index = 0; index < size; ++index)
        {
            if (_owner[index] == owner)
            {
                ice::pod::array::push_back(
                    entities_out,
                    detail::make_entity(
                        EntityInfo{
                            .index = index,
                            .generation = _generation[index],
                            .reserved = 0,
                        }
                    )
                );
            }
        }
    }

    bool EntityIndex::destroy_owned(void const* owner) noexcept
    {
        if (owner == nullptr) [[unlikely]]
        {
            return false;
        }

        ice::u32 const size = ice::pod::array::size(_owner);
        for (ice::u32 index = 0; index < size; ++index)
        {
            if (_owner[index] == owner)
            {
                _owner[index] = nullptr;
                _generation[index] += 1;

                ice::pod::queue::push_back(_free_indices, index);
            }
        }
        return true;
    }

} // namespace ice
