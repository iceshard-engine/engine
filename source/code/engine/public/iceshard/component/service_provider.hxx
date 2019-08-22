#pragma once
#include <core/allocator.hxx>

namespace iceshard::component
{


    //! \brief Special class that allows provides access to component managers.
    class ServiceProvider final
    {
    public:
        ServiceProvider(core::allocator&) noexcept;
        ~ServiceProvider() noexcept = default;

    private:
        core::allocator& _allocator;
    };


} // namespace iceshard::component
