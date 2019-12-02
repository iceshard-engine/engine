#pragma once
#include <core/allocator.hxx>
#include <core/pod/array.hxx>
#include <core/pod/queue.hxx>
#include <iceshard/entity/entity.hxx>

namespace iceshard
{


    //! \brief Special manager class handling entity lifetimes in the engine.
    class EntityManager final
    {
    public:
        EntityManager(core::allocator& alloc) noexcept;
        ~EntityManager() noexcept = default;

        //! \brief Total count of living entities.
        [[nodiscard]]
        auto count() noexcept -> uint32_t;

        //! \brief Creates a entity and returns it's handle.
        [[nodiscard]]
        auto create() noexcept -> entity_handle_type;

        //! \brief Creates a entity and returns it's handle.
        //! \param[in] owner The owner of the entity.
        [[nodiscard]]
        auto create(const void* owner) noexcept -> entity_handle_type;

        //! \brief Creates the requested number of entities and pushes them into the given array.
        void create_many(
            uint32_t count,
            core::pod::Array<entity_handle_type>& results
        ) noexcept;

        //! \brief Creates the requested number of entities and pushes them into the given array.
        //! \param[in] owner The owner of each created entity.
        void create_many(
            uint32_t count,
            core::pod::Array<entity_handle_type>& results,
            const void* owner
        ) noexcept;

        //! \brief Destroys the given entity.
        void destroy(entity_handle_type entity_handle) noexcept;

        //! \brief Checks if the given entity is alive.
        [[nodiscard]]
        bool is_alive(entity_handle_type entity_handle) noexcept;

        //! \brief Counts all owned entities.
        [[nodiscard]]
        auto count_owned(const void* owner) noexcept -> uint32_t;

        //! \brief Destroys all owned entities.
        void destroy_owned(const void* owner) noexcept;

        //! \brief Appends all entities of the given owner to the given array.
        void get_owned(
            const void* owner,
            core::pod::Array<entity_handle_type>& results
        ) noexcept;

    private:
        core::allocator& _allocator;

        //! \brief List of free entity indices.
        core::pod::Queue<uint32_t> _free_indices;

        //! \brief List of generations in use.
        core::pod::Array<uint16_t> _generation;

        //! \brief List of owners.
        core::pod::Array<const void*> _owner;
    };


} // namespace iceshard::entity
