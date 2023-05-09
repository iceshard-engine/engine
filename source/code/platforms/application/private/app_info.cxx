#include <ice/app_info.hxx>

namespace ice::app
{

    auto version() noexcept -> ice::app::Version
    {
        return { .major = 0, .minor = 0, .patch = 0, .build = 0, .commit = { 0, 0, 0, 0, 0 } };
    }

    auto name() noexcept -> ice::String
    {
        return { "iceshard-application" };
    }

} // namespace ice::app

