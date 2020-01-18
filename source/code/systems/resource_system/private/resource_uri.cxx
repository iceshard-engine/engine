#include <resource/uri.hxx>
#include <core/memory.hxx>
#include <core/string.hxx>
#include <core/path_utils.hxx>

namespace resource
{

    URI::URI(core::stringid_arg_type scheme, core::StringView path) noexcept
        : URI{ scheme, std::move(path), core::stringid_invalid }
    { }

    URI::URI(core::stringid_arg_type scheme, core::StringView path, core::stringid_arg_type fragment) noexcept
        : scheme{ scheme }
        , fragment{ fragment }
        , path{ std::move(path) }
    { }

    URI::URI(core::stringid_arg_type scheme, core::StringView path, URN name) noexcept
        : scheme{ scheme }
        , fragment{ name.name }
        , path{ std::move(path) }
    {
    }

    auto get_name(const URI& uri) noexcept -> URN
    {
        core::stringid_type resource_name = uri.fragment;
        if (resource_name == core::stringid_invalid)
        {
            // Get the resource name from the path
            resource_name = core::stringid(core::path::filename(uri.path));
        }

        return { resource_name };
    }

} // namespace resource
