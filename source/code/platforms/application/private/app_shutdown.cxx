/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/app.hxx>

auto ice_shutdown(
    ice::Allocator& alloc,
    ice::app::Config const& config,
    ice::app::State& state
) noexcept -> ice::Result
{
    return ice::S_Success;
}
