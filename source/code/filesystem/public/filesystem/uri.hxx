#pragma once
#include <core/string.hxx>
#include <core/cexpr/stringid.hxx>

namespace filesystem
{


//! \brief A simple Uniform Resource Name type.
struct URN
{
    //! \brief The resource schema.
    //! \details A schema in this example is the file content type.
    //!     So it might be an image, file, script, texture, mesh etc.
    core::cexpr::stringid_type schema;

    //! \brief The resource name.
    //! \details The name is a unique string identifier, commonly
    //!     made of the source file basename.
    core::cexpr::stringid_type name;
};


//! \brief A simple Uniform Resource Locator type.
struct URL
{
    //! \brief The resource schema.
    //! \details A schema in this example is the file content type.
    //!     So it might be an image, file, script, texture, mesh etc.
    core::cexpr::stringid_type schema;

    //! \brief The resource path.
    core::String<> path;
};


//! \brief A simple Uniform Resource Identifier type.
struct URI
{
    URI(core::cexpr::stringid_argument_type schema, core::cexpr::stringid_argument_type name, core::String<> path) noexcept
        : schema{ schema }
        , name{ name }
        , path{ std::move(path) }
    { }

    ~URI() noexcept
    {
        core::string::set_capacity(path, 0);
    }

    //! \brief The resource schema.
    //! \details A schema in this example is the file content type.
    //!     So it might be an image, file, script, texture, mesh etc.
    core::cexpr::stringid_type schema;

    //! \brief The resource name.
    //! \details The name is a unique string identifier, commonly
    //!     made of the source file basename.
    core::cexpr::stringid_type name;

    //! \brief The resource path.
    core::String<> path;
};


} // namespace filesystem


//////////////////////////////////////////////////////////////////////////


namespace fmt
{

template<>
struct formatter<filesystem::URI>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const filesystem::URI& uri, FormatContext& ctx)
    {
#if CFG_RELEASE == 0
        if (uri.schema == core::cexpr::stringid_invalid || uri.name == core::cexpr::stringid_invalid)
        {
            return fmt::format_to(ctx.begin(), "{}://{}#{}", uri.schema, uri.path, uri.name);
        }
        else
        {
            return fmt::format_to(ctx.begin(), "{}://{}#{}", uri.schema.hash_origin, uri.path, uri.name.hash_origin);
        }
#else
        return fmt::format_to(ctx.begin(), "{}://{}#{}", uri.schema, uri.path, uri.name);
#endif
    }
};

} // namespace fmt