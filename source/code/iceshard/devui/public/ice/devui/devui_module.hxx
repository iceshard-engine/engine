/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/module_register.hxx>

namespace ice::devui
{

    class DevUISystem;

    auto create_devui_system(
        ice::Allocator& alloc,
        ice::ModuleRegister& registry
    ) noexcept -> ice::UniquePtr<ice::devui::DevUISystem>;

    namespace detail::v1
    {

        using CreateFn = auto (ice::Allocator&) noexcept -> ice::devui::DevUISystem*;
        using DestroyFn = void (ice::devui::DevUISystem*) noexcept;

        struct DevUI_API
        {
            static constexpr ice::StringID Constant_APIName = "ice.devui_module"_sid;
            static constexpr ice::u32 Constant_APIVersion = 1;

            CreateFn* create_system_fn;
            DestroyFn* destroy_system_fn;
        };

    } // namespace api

} // namespace ice::devui
