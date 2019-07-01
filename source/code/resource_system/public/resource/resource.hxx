#pragma once
#include <resource/uri.hxx>
#include <core/string.hxx>

namespace resource
{


//! \brief Describes a single resource which can be fetched for data.
class Resource final
{
public:
    Resource(core::allocator& alloc, const URI& path) noexcept;
    ~Resource() noexcept;

    auto location() const noexcept -> const URI&;

private:
    core::String<> _path;
    URI _uri;
};


} // namespace resource
