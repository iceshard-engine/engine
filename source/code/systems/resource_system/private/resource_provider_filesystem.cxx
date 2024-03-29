/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_provider_filesystem.hxx"
#include "resource_provider_hailstorm.hxx"

auto ice::create_resource_provider_hailstorm(
    ice::Allocator& alloc,
    ice::String path
) noexcept -> ice::UniquePtr<ice::ResourceProvider>
{
    return ice::make_unique<ice::HailStormResourceProvider>(alloc, alloc, path);
}
