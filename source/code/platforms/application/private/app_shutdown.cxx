#include <ice/app.hxx>

auto ice_shutdown(
    ice::Allocator& alloc,
    ice::app::Arguments const& args,
    ice::app::Config const& config,
    ice::app::State& state
) noexcept -> ice::Result
{
    return ice::Res::Success;
}
