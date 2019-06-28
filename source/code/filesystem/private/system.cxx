#include <resource/system.hxx>
#include <core/memory.hxx>

namespace resource
{
namespace detail
{

class EmptyResourceSystem : public ResourceSystem
{
public:
    auto find(const URN& /*urn*/) noexcept -> Resource* override { return nullptr; }
    auto find(const URI& /*uri*/) noexcept -> Resource* override { return nullptr; }
    auto mount(const URI& /*uri*/) noexcept -> uint32_t override { return 0; }
};

auto default_resource_system() noexcept -> ResourceSystem*
{
    static EmptyResourceSystem empty_resource_system;
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

void set_system(ResourceSystem& system) noexcept
{
    detail::current_resource_system = &system;
}

} // namespace resource
