#include <ice/app.hxx>

auto ice_setup(
    ice::Allocator& alloc,
    ice::app::Arguments const& args,
    ice::app::Config& config,
    ice::app::State& state
) noexcept -> ice::Result
{
    return ice::app::S_ApplicationResume;
}
