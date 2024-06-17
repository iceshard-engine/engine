/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/app.hxx>

auto ice_setup(
    ice::Allocator& alloc,
    ice::app::Config& config,
    ice::app::State& state
) noexcept -> ice::Result
{
    return ice::app::S_ApplicationResume;
}
