/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/devui/devui_module.hxx>
#include <ice/devui/devui_system.hxx>

namespace ice::devui
{

    auto create_devui_system(
        ice::Allocator& alloc,
        ice::ModuleRegister& registry
    ) noexcept -> ice::UniquePtr<ice::devui::DevUISystem>
    {
        ice::UniquePtr<DevUISystem> result{ };

        ice::devui::detail::v1::DevUI_API devui_api;
        if (registry.query_api(devui_api))
        {
            ice::devui::DevUISystem* devui_system = devui_api.create_system_fn(alloc);
            result = ice::UniquePtr<ice::devui::DevUISystem>{ devui_api.destroy_system_fn, devui_system };
        }
        return result;
    }

} // namespace ice
