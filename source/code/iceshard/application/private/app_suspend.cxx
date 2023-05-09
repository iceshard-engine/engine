#include <ice/app.hxx>

auto ice_suspend(
    ice::app::Config const& config,
    ice::app::State& state,
    ice::app::Runtime& runtime
) noexcept -> ice::Result
{
    return ice::app::S_ApplicationExit;
}
