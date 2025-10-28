/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/devui_types.hxx>
#include <ice/module_types.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/string/string.hxx>
#include <ice/span.hxx>

namespace ice
{

    class DevUIContext
    {
    public:
        virtual ~DevUIContext() noexcept = default;

        virtual void update_widgets() noexcept = 0;
    };

    struct DevUIContextSetupParams
    {
        void* native_context;
        FnDevUIAlloc fn_alloc;
        FnDevUIDealloc fn_dealloc;
        void* alloc_userdata;
    };

    bool devui_available() noexcept;

    void devui_setup_mainmenu(ice::Span<ice::String> categories) noexcept;

    auto devui_trait_name() noexcept -> ice::StringID;

    bool devui_register_widget(
        ice::DevUIWidget* widget,
        ice::DevUIWidget* owning_widget = nullptr
    ) noexcept;

    bool devui_remove_widget(
        ice::DevUIWidget* widget
    ) noexcept;

    bool devui_context_setup_default(
        ice::StringID_Arg context_name,
        ice::DevUIContextSetupParams const& params,
        void* userdata
    ) noexcept;

    bool devui_setup_context(
        ice::ModuleQuery const& query,
        ice::FnDevUIContextSetupCallback callback = ice::devui_context_setup_default,
        void* userdata = nullptr
    ) noexcept;

    auto create_devui_context(
        ice::Allocator& alloc,
        ice::ModuleQuery& query
    ) noexcept -> ice::UniquePtr<ice::DevUIContext>;

} // namespace ice
