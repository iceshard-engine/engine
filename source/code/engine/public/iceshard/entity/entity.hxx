#pragma once
#include <core/base.hxx>

namespace iceshard::entity
{


    //! \brief A handle for a single entity.
    enum class entity_handle : uint64_t { };

    //! \brief An invalid entity handle object.
    static constexpr entity_handle invalid_entity_handle{ 0 };


    //! \brief Checks if the entity handle is valid.
    bool valid(entity_handle handle) noexcept;


    //! \brief Entity handle equality operator.
    bool operator==(entity_handle left, entity_handle right) noexcept;

    //! \brief Entity handle in-equality operator.
    bool operator!=(entity_handle left, entity_handle right) noexcept;


} // namespace iceshard::entity
