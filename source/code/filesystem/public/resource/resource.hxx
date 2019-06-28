#pragma once
#include <resource/uri.hxx>

namespace resource
{


//! \brief Describes a single resource which can be fetched for data.
class Resource final
{
public:
    Resource(const URI& uri) noexcept;
    ~Resource() noexcept;

    auto location() const noexcept -> const URI&;

private:
    URI _location;
};


} // namespace resource
