/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/app.hxx>

auto ice_update(
    ice::app::Config const& config,
    ice::app::State const& state,
    ice::app::Runtime& runtime
) noexcept -> ice::Result
{
    return ice::app::S_ApplicationExit;
}
