#include <iceshard/entity/entity_manager.hxx>

namespace iceshard::entity
{
    namespace detail
    {

        //! \brief Internal entity representation.
        union entity_type
        {
            // The public handle.
            entity_handle_type handle;

            // The public handle as integet.
            uint64_t handle_numeric;

            // The internal handle representation
            struct
            {
                uint32_t index;
                uint16_t generation;
                uint16_t padding;
            } object;
        };


        //! \brief Returns an entity object from the handle.
        [[nodiscard]]
        auto entity_from_handle(entity_handle_type handle) noexcept -> entity_type
        {
            entity_type entity_object;
            entity_object.handle = handle;
            return entity_object;
        }

        //! \brief Returns the entity index from the handle.
        auto index_from_handle(entity_handle_type handle) noexcept -> uint32_t
        {
            entity_type entity_object = entity_from_handle(handle);
            return entity_object.object.index;
        }

        //! \brief Returns a handle from the given index and generation.
        [[nodiscard]]
        auto handle_from_parts(uint32_t index, uint16_t generation) noexcept -> entity_handle_type
        {
            entity_type entity_object;
            entity_object.object = { index, generation, uint16_t{ 0 } };
            return entity_object.handle;
        }


        static constexpr uint32_t minimum_free_indices = 1024;

    } // namespace detail


    EntityManager::EntityManager(core::allocator& alloc) noexcept
        : _allocator{ alloc }
        , _free_indices{ alloc }
        , _generation{ alloc }
        , _owner{ alloc }
    { }

    auto EntityManager::count() noexcept -> uint32_t
    {
        return core::pod::array::size(_generation) - core::pod::queue::size(_free_indices);
    }

    auto EntityManager::create() noexcept -> entity_handle_type
    {
        uint32_t entity_index;

        if (core::pod::queue::size(_free_indices) > detail::minimum_free_indices)
        {
            entity_index = *core::pod::queue::begin_front(_free_indices);
            core::pod::queue::pop_front(_free_indices);
        }
        else
        {
            entity_index = core::pod::array::size(_generation);
            core::pod::array::push_back<uint16_t>(_generation, 0);
            core::pod::array::push_back<const void*>(_owner, nullptr);
        }

        return detail::handle_from_parts(entity_index, _generation[entity_index]);
    }

    auto EntityManager::create(const void* owner) noexcept -> entity_handle_type
    {
        auto result_handle = this->create();
        _owner[detail::index_from_handle(result_handle)] = owner;
        return result_handle;
    }

    void EntityManager::create_many(uint32_t count, core::pod::Array<entity_handle_type>& results) noexcept
    {
        // For now when creating entities in bulk, we skip the check of _free_indices.
        uint32_t index = core::pod::array::size(_generation);
        uint32_t end_index = index + count;

        // Resize the generation and owner arrays to hold at least the `end_index`
        core::pod::array::resize(_generation, end_index);
        core::pod::array::resize(_owner, end_index);

        // Set the generation from 'start_index' to 'end_index' to 0.
        while (index < end_index)
        {
            _generation[index] = 0;
            _owner[index] = nullptr;

            // Add the resulting entity to the result array.
            core::pod::array::push_back(results, detail::handle_from_parts(index, _generation[index]));

            // Advance the index
            index++;
        }
    }

    void EntityManager::create_many(uint32_t count, core::pod::Array<entity_handle_type>& results, const void * owner) noexcept
    {
        // For now when creating entities in bulk, we skip the check of _free_indices.
        uint32_t index = core::pod::array::size(_generation);
        uint32_t end_index = index + count;

        // Resize the generation and owner arrays to hold at least the `end_index`
        core::pod::array::resize(_generation, end_index);
        core::pod::array::resize(_owner, end_index);

        // Set the generation from 'start_index' to 'end_index' to 0.
        while (index < end_index)
        {
            _generation[index] = 0;
            _owner[index] = owner;

            // Add the resulting entity to the result array.
            core::pod::array::push_back(results, detail::handle_from_parts(index, _generation[index]));

            // Advance the index
            index++;
        }
    }

    void EntityManager::destroy(entity_handle_type entity_handle) noexcept
    {
        const auto index = detail::index_from_handle(entity_handle);

        _owner[index] = nullptr;
        _generation[index] += 1;

        core::pod::queue::push_back(_free_indices, index);
    }

    bool EntityManager::is_alive(entity_handle_type entity_handle) noexcept
    {
        const auto entity_object = detail::entity_from_handle(entity_handle);
        return _generation[entity_object.object.index] == entity_object.object.generation;
    }

    auto EntityManager::count_owned(const void* owner) noexcept -> uint32_t
    {
        uint32_t result = 0;
        uint32_t size = core::pod::array::size(_owner);
        for (uint32_t index = 0; index < size; ++index)
        {
            if (_owner[index] == owner)
            {
                result += 1;
            }
        }
        return result;
    }

    void EntityManager::destroy_owned(const void* owner) noexcept
    {
        [[likely]]
        if (owner != nullptr)
        {
            uint32_t size = core::pod::array::size(_owner);
            for (uint32_t index = 0; index < size; ++index)
            {
                if (_owner[index] == owner)
                {
                    _owner[index] = nullptr;
                    _generation[index] += 1;

                    core::pod::queue::push_back(_free_indices, index);
                }
            }
        }
    }

    void EntityManager::get_owned(const void * owner, core::pod::Array<entity_handle_type>& results) noexcept
    {
        uint32_t size = core::pod::array::size(_owner);
        for (uint32_t index = 0; index < size; ++index)
        {
            if (_owner[index] == owner)
            {
                core::pod::array::push_back(results, detail::handle_from_parts(index, _generation[index]));
            }
        }
    }

} // namespace iceshard::entity
