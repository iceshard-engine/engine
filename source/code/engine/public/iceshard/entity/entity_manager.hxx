#pragma once
#include <core/allocator.hxx>
#include <core/pod/array.hxx>
#include <iceshard/entity/entity.hxx>

namespace iceshard::entity
{


    //! \brief Special manager class handling entity lifetimes in the engine.
    class EntityManager final
    {
    public:
        EntityManager(core::allocator& alloc) noexcept;
        ~EntityManager() noexcept = default;

    private:
        core::allocator& _allocator;

        //! \brief List of handles to be reused.
        core::pod::Array<entity_handle> _free_handles;

        //! \brief List of handles in use.
        core::pod::Array<entity_handle> _handles;
    };


} // namespace iceshard::entity
