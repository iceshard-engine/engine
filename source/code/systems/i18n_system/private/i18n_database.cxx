/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/i18n_database.hxx>
#include "i18n_resource_database.hxx"

namespace ice
{

    auto create_i18n_database(
        ice::Allocator& allocator,
        ice::ResourceTracker& resources
    ) noexcept -> ice::UniquePtr<I18NDatabase>
    {
        return ice::make_unique<I18NResourceDatabase>(allocator, allocator, resources);
    }

} // namespace ice
