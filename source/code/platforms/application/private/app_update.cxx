#include <ice/app.hxx>

auto ice_update(
    ice::app::Config const& config,
    ice::app::State const& state,
    ice::app::Runtime& runtime
) noexcept -> ice::Result
{
    return ice::app::S_ApplicationExit;
}
