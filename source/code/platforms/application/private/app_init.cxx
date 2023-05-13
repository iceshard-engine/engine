/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/app.hxx>

void ice_init(
    ice::Allocator& alloc,
    ice::app::Factories& factories
) noexcept
{
    factories.factory_config = [](ice::Allocator&) -> ice::UniquePtr<ice::app::Config> { return {}; };
    factories.factory_state = [](ice::Allocator&) -> ice::UniquePtr<ice::app::State> { return {}; };
    factories.factory_runtime = [](ice::Allocator&) -> ice::UniquePtr<ice::app::Runtime> { return {}; };
}
