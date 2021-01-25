#pragma once
#include <ice/uri.hxx>
#include <ice/data.hxx>

namespace ice
{

    struct Metadata;

    class Resource
    {
    public:
        virtual ~Resource() noexcept = default;

        virtual auto name() const noexcept -> ice::String = 0;

        virtual auto location() const noexcept -> ice::URI const& = 0;

        virtual auto metadata() const noexcept -> ice::Metadata const& = 0;

        virtual auto data() noexcept -> ice::Data = 0;
    };


} // namespace ice
