#include "system.hxx"

namespace resource
{
namespace detail
{

auto default_resource_system() noexcept -> ResourceSystem*
{
    static ResourceSystem empty_resource_system;
    return &empty_resource_system;
}

static ResourceSystem* current_resource_system{ default_resource_system() };

} // namespace detail

auto default_resource_system() noexcept -> ResourceSystem&
{
    return *detail::default_resource_system();
}

auto get_system() noexcept -> ResourceSystem&
{
    return *detail::current_resource_system;
}

void set_system(ResourceSystem system) noexcept
{
    detail::current_resource_system = &system;
}

} // namespace resource
