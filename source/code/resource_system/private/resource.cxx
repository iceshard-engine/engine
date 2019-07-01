#include <resource/resource.hxx>
#include <core/string.hxx>

namespace resource
{

Resource::Resource(core::allocator& alloc, const URI& uri) noexcept
    : _path{ alloc, uri.path._data }
    , _uri{ uri.scheme, _path, uri.fragment }
{ }

Resource::~Resource() noexcept
{ }

auto Resource::location() const noexcept -> const URI&
{
    return _uri;
}

} // namespace resource
