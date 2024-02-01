/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "imgui_system.hxx"
#include <ice/devui/devui_module.hxx>

#include <ice/mem_allocator.hxx>
#include <ice/log_module.hxx>

namespace ice::devui
{

    auto create_imgui_devui(ice::Allocator& alloc) noexcept -> ice::devui::DevUISystem*
    {
        return alloc.create<ImGuiSystem>(alloc);
    }

    auto destroy_imgui_devui(ice::devui::DevUISystem* system) noexcept
    {
        ice::Allocator& alloc = static_cast<ImGuiSystem*>(system)->allocator();
        alloc.destroy(system);
    }

    struct ImGuiModule : ice::Module<ImGuiModule>
    {
        static void v1_devui_system(ice::devui::detail::v1::DevUI_API& api) noexcept
        {
            api.create_system_fn = create_imgui_devui;
            api.destroy_system_fn = destroy_imgui_devui;
        }

        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator const& negotiator) noexcept
        {
            ice::LogModule::init(alloc, negotiator);
            return negotiator.register_api(v1_devui_system);
        }
    };

} // namespace ice
