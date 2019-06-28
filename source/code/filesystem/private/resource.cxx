#include <resource\resource.hxx>

namespace resource
{

Resource::Resource(const URI& uri) noexcept
    : _location{ uri }
{ }

Resource::~Resource() noexcept
{
}

auto Resource::location() const noexcept -> const URI&
{
    return _location;
}

} // namespace resource
