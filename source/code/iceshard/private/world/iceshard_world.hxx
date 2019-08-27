#include <iceshard/world/world.hxx>

namespace iceshard::world
{

    class IceshardWorld : public World
    {
    public:
        IceshardWorld(
            core::allocator& alloc,
            core::cexpr::stringid_argument_type world_name,
            iceshard::entity::entity_handle_type world_entity
        ) noexcept;

        ~IceshardWorld() noexcept override = default;

        //! \brief The worlds service provider.
        auto service_provider() noexcept -> component::ServiceProvider* override;

    private:
        core::allocator& _allocator;
    };

} // namespace iceshard::world