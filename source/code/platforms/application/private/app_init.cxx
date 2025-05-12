/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/app.hxx>

void ice_init(
    ice::Allocator& alloc,
    ice::app::Factories& factories
) noexcept
{
    factories.factory_config = [](ice::Allocator&) noexcept -> ice::UniquePtr<ice::app::Config> { return {}; };
    factories.factory_state = [](ice::Allocator&) noexcept -> ice::UniquePtr<ice::app::State> { return {}; };
    factories.factory_runtime = [](ice::Allocator&) noexcept -> ice::UniquePtr<ice::app::Runtime> { return {}; };
}
