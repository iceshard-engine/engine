#pragma once
#include <core/cexpr/stringid.hxx>
#include <core/string.hxx>

namespace resource
{


//! \brief Uniform Resource Name.
struct URN
{
    //! \brief The resource scheme.
    core::cexpr::stringid_type scheme{ core::cexpr::stringid_invalid };

    //! \brief The resource name.
    core::cexpr::stringid_type name{ core::cexpr::stringid_invalid };
};


//! \brief Uniform Resource Identifier.
struct URI
{
    //! \brief Creates a new URI for the given scheme and path.
    URI(core::cexpr::stringid_argument_type scheme, core::String<> path) noexcept;

    //! \brief Creates a new URI for the given scheme, path and fragment.
    URI(core::cexpr::stringid_argument_type scheme, core::String<> path, core::cexpr::stringid_argument_type fragment) noexcept;

    //! \brief Releases the path memory if allocated.
    ~URI() noexcept;

    //! \brief The resource scheme.
    core::cexpr::stringid_type scheme;

    //! \brief The resource fragment.
    core::cexpr::stringid_type fragment;

    //! \brief The resource location.
    core::String<> path;
};


//! \brief Returns the URN from the given URI.
auto get_name(const URI& uri) noexcept -> URN;


} // namespace resource


//////////////////////////////////////////////////////////////////////////


namespace fmt
{

template<>
struct formatter<resource::URN>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const resource::URN& urn, FormatContext& ctx)
    {
        if (urn.scheme == core::cexpr::stringid_invalid || urn.name == core::cexpr::stringid_invalid)
        {
            return fmt::format_to(ctx.begin(), "urn:{}:{}", urn.scheme, urn.name);
        }
        else
        {
#if CFG_RELEASE == 0
            return fmt::format_to(ctx.begin(), "urn:{}:{}", urn.scheme.hash_origin, urn.name.hash_origin);
#else
            return fmt::format_to(ctx.begin(), "urn:{}:{}", urn.scheme, urn.name);
#endif
        }
    }
};


template<>
struct formatter<resource::URI>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const resource::URI& uri, FormatContext& ctx)
    {
        if (uri.scheme == core::cexpr::stringid_invalid)
        {
            return fmt::format_to(ctx.begin(), "{}:{}", uri.scheme, uri.path);
        }
        else
        {
            auto format_string = std::string_view{ uri.fragment == core::cexpr::stringid_invalid ? "{0}:{2}" : "{0}:{2}#{1}" };
#if CFG_RELEASE == 0
            auto format_args = fmt::make_format_args(
                uri.scheme.hash_origin
                , uri.fragment.hash_origin
                , uri.path
            );
#else
            auto format_args = fmt::make_format_args(
                uri.scheme
                , uri.fragment
                , uri.path
            );
#endif
            return fmt::vformat_to(ctx.begin(), format_string, format_args);
        }
    }
};

} // namespace fmt
