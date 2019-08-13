#pragma once
#include <core/cexpr/stringid.hxx>
#include <core/string_view.hxx>

namespace resource
{

    //! \brief File scheme.
    static core::cexpr::stringid_type scheme_file{ core::cexpr::stringid("file") };

    //! \brief Directory scheme.
    static core::cexpr::stringid_type scheme_directory{ core::cexpr::stringid("dir") };

    //! \brief ResourcePack scheme.
    static core::cexpr::stringid_type scheme_pack{ core::cexpr::stringid("pack") };

    //! \brief Dynamic library scheme.
    static core::cexpr::stringid_type scheme_dynlib{ core::cexpr::stringid("dynlib") };

    //! \brief Resource Name scheme.
    static core::cexpr::stringid_type scheme_resource{ core::cexpr::stringid("res") };


    //! \brief Uniform Resource Name.
    struct URN
    {
        //! \brief Creates a new resource name from the given string value.
        URN(core::StringView<> name) noexcept;

        //! \brief Creates a new resource name from the given stringid value.
        URN(core::cexpr::stringid_argument_type name) noexcept;

        //! \brief The resource name.
        core::cexpr::stringid_type name{ core::cexpr::stringid_invalid };
    };

    static_assert(std::is_trivially_copyable_v<URN>, "The 'URN' type requires to be trivially copyable!");


    //! \brief Uniform Resource Identifier.
    struct URI
    {
        //! \brief Creates a new URI for the given scheme and path.
        URI(core::cexpr::stringid_argument_type scheme, core::StringView<> path) noexcept;

        //! \brief Creates a new URI for the given scheme, path and fragment.
        URI(core::cexpr::stringid_argument_type scheme, core::StringView<> path, core::cexpr::stringid_argument_type fragment) noexcept;

        //! \brief Creates a new URI for the given scheme, path and fragment.
        URI(core::cexpr::stringid_argument_type scheme, core::StringView<> path, URN name) noexcept;

        //! \brief The resource scheme.
        core::cexpr::stringid_type scheme;

        //! \brief The resource fragment.
        core::cexpr::stringid_type fragment;

        //! \brief The resource location.
        core::StringView<> path;
    };

    static_assert(std::is_trivially_copyable_v<URI>, "The 'URI' type requires to be trivially copyable!");


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
            if (urn.name == core::cexpr::stringid_invalid)
            {
                return fmt::format_to(ctx.begin(), "urn:{}:{}", resource::scheme_resource, urn.name);
            }
            else
            {
#if STRINGID_DEBUG == 1
                return fmt::format_to(ctx.begin(), "{}:{}", resource::scheme_resource.hash_origin, urn.name.hash_origin);
#else
                return fmt::format_to(ctx.begin(), "{}:{}", resource::scheme_resource, urn.name);
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
#if STRINGID_DEBUG == 1
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
