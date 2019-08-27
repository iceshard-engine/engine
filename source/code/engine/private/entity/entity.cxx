#include <iceshard/entity/entity.hxx>

namespace iceshard::entity
{

    bool valid(entity_handle_type handle) noexcept
    {
        return handle != invalid_entity_handle;
    }

    bool operator==(entity_handle_type left, entity_handle_type right) noexcept
    {
        return static_cast<uint64_t>(left) == static_cast<uint64_t>(right);
    }

    bool operator!=(entity_handle_type left, entity_handle_type right) noexcept
    {
        return !(left == right);
    }

} // namespace iceshard::entity
