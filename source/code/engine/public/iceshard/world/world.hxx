#pragma once
#include <iceshard/component/service_provider.hxx>
#include <core/allocator.hxx>

namespace iceshard::world
{


    //! \brief A single world container which is allowed to hold component managers.
    class World
    {
    public:
        virtual ~World() noexcept = default;

        virtual auto service_provider() noexcept -> component::ServiceProvider* = 0;
    };


} // namespace iceshard
