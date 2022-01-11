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

namespace ice
{

    enum class ResourceStatus_v2 : ice::u32
    {
        Unknown,
        Available,
        Loaded,
        Updated,
        Unloaded,
        Released,
        Failure,
    };

    class Resource_v2
    {
    public:
        virtual ~Resource_v2() noexcept = default;

        //virtual auto urn() const noexcept -> ice::URN = 0;
        virtual auto uri() const noexcept -> ice::URI_v2 const& = 0;

        virtual auto name() const noexcept -> ice::Utf8String = 0;
        virtual auto origin() const noexcept -> ice::Utf8String = 0;

        virtual auto metadata() const noexcept -> ice::Metadata const& = 0;
    };

} // ice::res_v2
