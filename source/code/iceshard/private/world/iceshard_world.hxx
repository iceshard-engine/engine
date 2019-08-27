#pragma once
#include <iceshard/world/world.hxx>
#include <core/pointer.hxx>

namespace iceshard
{

    class IceshardWorld : public World
    {
    public:
        IceshardWorld(
            core::allocator& alloc,
            core::cexpr::stringid_argument_type world_name,
            iceshard::entity_handle_type world_entity,
            iceshard::ServiceProvider* engine_service_provider
        ) noexcept;

        ~IceshardWorld() noexcept override = default;

        //! \brief The worlds service provider.
        auto service_provider() noexcept -> iceshard::ServiceProvider* override;

    private:
        core::allocator& _allocator;

        core::memory::unique_pointer<iceshard::ServiceProvider> _service_provider;
    };

} // namespace iceshard::world