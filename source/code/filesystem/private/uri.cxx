#include <resource/uri.hxx>
#include <core/memory.hxx>

namespace resource
{

URI::URI(core::cexpr::stringid_argument_type scheme, core::String<> path) noexcept
    : URI{ scheme, std::move(path), core::cexpr::stringid_invalid }
{ }

URI::URI(core::cexpr::stringid_argument_type scheme, core::String<> path, core::cexpr::stringid_argument_type fragment) noexcept
    : scheme{ scheme }
    , fragment{ fragment }
    , path{ std::move(path) }
{ }

URI::~URI() noexcept
{
    core::string::set_capacity(path, 0);
}


auto get_name(const URI& uri) noexcept -> URN
{
    core::cexpr::stringid_type resource_name = uri.fragment;
    if (resource_name == core::cexpr::stringid_invalid)
    {
        auto* it = core::string::begin(uri.path);
        auto* end = core::string::end(uri.path);

        auto* filename = it;
        while (it != end)
        {
            if (*it == '/')
            {
                filename = it + 1;
            }
            it += 1;
        }

        // Get the resource name from the path
        resource_name = core::cexpr::stringid(filename);
    }

    return { core::cexpr::stringid("res"), resource_name };
}

} // namespace resource
