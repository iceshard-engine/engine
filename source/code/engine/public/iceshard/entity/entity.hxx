#pragma once
#include <core/base.hxx>

namespace iceshard
{


    //! \brief A handle for a single entity.
    enum class entity_handle_type : uint64_t { };

    //! \brief An invalid entity handle object.
    static constexpr entity_handle_type invalid_entity_handle{ 0 };


    //! \brief Checks if the entity handle is valid.
    bool valid(entity_handle_type handle) noexcept;


    //! \brief Entity handle equality operator.
    bool operator==(entity_handle_type left, entity_handle_type right) noexcept;

    //! \brief Entity handle in-equality operator.
    bool operator!=(entity_handle_type left, entity_handle_type right) noexcept;


} // namespace iceshard::entity
