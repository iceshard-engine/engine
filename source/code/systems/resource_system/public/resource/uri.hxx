#pragma once
#include <core/cexpr/stringid.hxx>
#include <core/string_view.hxx>

namespace resource
{

    //! \brief Invalid scheme.
    static constexpr core::stringid_type scheme_invalid = "invalid"_sid;

    //! \brief File scheme.
    static constexpr core::stringid_type scheme_file = "file"_sid;

    //! \brief Directory scheme.
    static constexpr core::stringid_type scheme_directory = "dir"_sid;

    //! \brief ResourcePack scheme.
    static constexpr core::stringid_type scheme_pack = "pack"_sid;

    //! \brief Dynamic library scheme.
    static constexpr core::stringid_type scheme_dynlib = "dynlib"_sid;

    //! \brief Resource Name scheme.
    static constexpr core::stringid_type scheme_resource = "res"_sid;


    //! \brief Uniform Resource Name.
    struct URN
    {
        //! \brief Creates a new resource name from the given string value.
        constexpr URN(core::StringView name) noexcept;

        //! \brief Creates a new resource name from the given stringid value.
        constexpr URN(core::stringid_arg_type name) noexcept;

        //! \brief The resource name.
        core::stringid_type name = core::stringid_invalid;
    };

    static_assert(std::is_trivially_copyable_v<URN>, "The 'URN' type requires to be trivially copyable!");

    //! \brief Uniform Resource Identifier.
    struct URI
    {
        //! \brief Creates a new URI for the given scheme and path.
        URI(core::stringid_arg_type scheme, core::StringView path) noexcept;

        //! \brief Creates a new URI for the given scheme, path and fragment.
        URI(core::stringid_arg_type scheme, core::StringView path, core::stringid_arg_type fragment) noexcept;

        //! \brief Creates a new URI for the given scheme, path and fragment.
        URI(core::stringid_arg_type scheme, core::StringView path, URN name) noexcept;

        //! \brief The resource scheme.
        core::stringid_type scheme = resource::scheme_invalid;

        //! \brief The resource fragment.
        core::stringid_type fragment;

        //! \brief The resource location.
        core::StringView path;
    };

    static const resource::URI uri_invalid{ resource::scheme_invalid, "" };

    static_assert(std::is_trivially_copyable_v<URI>, "The 'URI' type requires to be trivially copyable!");

    //! \brief Returns the URN from the given URI.
    auto get_name(const URI& uri) noexcept -> URN;

} // namespace resource

constexpr resource::URN::URN(core::StringView name) noexcept
    : name{ core::stringid(name) }
{
}

constexpr resource::URN::URN(core::stringid_arg_type name) noexcept
    : name{ name }
{
}

//////////////////////////////////////////////////////////////////////////

namespace fmt
{

    template<>
    struct formatter<resource::URN>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext& ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(resource::URN const& urn, FormatContext& ctx)
        {
            if (urn.name == core::stringid_invalid)
            {
                return fmt::format_to(ctx.out(), "urn:{}:{}", resource::scheme_resource, urn.name);
            }
            else
            {
                if constexpr (core::cexpr::has_debug_fields<decltype(urn.name)>())
                {
                    return fmt::format_to(ctx.out(), "{}:{}", core::origin(resource::scheme_resource), core::origin(urn.name));
                }
                else
                {
                    return fmt::format_to(ctx.out(), "{}:{}", resource::scheme_resource, urn.name);
                }
            }
        }
    };

    template<>
    struct formatter<resource::URI>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext& ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(resource::URI const& uri, FormatContext& ctx)
        {
            if (uri.scheme == core::stringid_invalid)
            {
                return fmt::format_to(ctx.out(), "{}:{}", uri.scheme, uri.path);
            }
            else
            {
                std::string_view format_string = uri.fragment == core::stringid_invalid ? "{0}:{2}" : "{0}:{2}#{1}";

                if constexpr (core::cexpr::has_debug_fields<decltype(uri.scheme)>())
                {
                    auto format_args = fmt::make_format_args(core::origin(uri.scheme), core::origin(uri.fragment), uri.path);
                    return fmt::vformat_to(ctx.out(), format_string, format_args);
                }
                else
                {
                    auto format_args = fmt::make_format_args(uri.scheme, uri.fragment, uri.path);
                    return fmt::vformat_to(ctx.out(), format_string, format_args);
                }
            }
        }
    };

} // namespace fmt
