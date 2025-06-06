/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/devui_context.hxx>
#include <ice/devui_module.hxx>
#include <ice/module_query.hxx>
#include <ice/log.hxx>
#include <imgui/imgui.h>

#if ISP_WINDOWS
#include <imguizmo/ImGuizmo.h>
#endif

namespace ice
{

    static ice::api::devui::v1::DevUI_API::FnContextSetupMenu global_context_setup_menu = nullptr;
    static ice::api::devui::v1::DevUI_API::FnContextRegisterWidget global_context_register_widget = nullptr;
    static ice::api::devui::v1::DevUI_API::FnContextRemoveWidget global_context_remove_widget = nullptr;
    static ice::api::devui::v1::DevUI_API::FnContextTraitName global_context_trait_name = nullptr;

    bool devui_available() noexcept
    {
        return global_context_trait_name != nullptr && global_context_register_widget != nullptr;
    }

    void devui_setup_mainmenu(ice::Span<ice::String> categories) noexcept
    {
        if (global_context_setup_menu != nullptr)
        {
            global_context_setup_menu(categories);
        }
    }

    auto devui_trait_name() noexcept -> ice::StringID
    {
        return global_context_trait_name ? global_context_trait_name() : StringID_Invalid;
    }

    bool devui_register_widget(
        ice::DevUIWidget* widget
    ) noexcept
    {
        if (global_context_register_widget == nullptr)
        {
            ICE_LOG(
                LogSeverity::Error, LogTag::System,
                "Trying to register DevUI widget before settin-up the context"
            );
            return false;
        }

        global_context_register_widget(widget);
        return true;
    }

    bool devui_remove_widget(
        ice::DevUIWidget* widget
    ) noexcept
    {
        if (global_context_remove_widget == nullptr)
        {
            // ICE_LOG(
            //     LogSeverity::Error, LogTag::System,
            //     "Trying to remove DevUI widget without a devui context"
            // );
            return false;
        }

        global_context_remove_widget(widget);
        return true;
    }

    bool devui_context_setup_default(
        ice::StringID_Arg context_name,
        ice::DevUIContextSetupParams const& params,
        void* userdata
    ) noexcept
    {
        if (context_name == "devui-context/imgui"_sid)
        {
            ICE_ASSERT_CORE(ImGui::GetCurrentContext() == nullptr || ImGui::GetCurrentContext() == params.native_context);
            // TODO: Try to move it away from here
            ImGui::SetAllocatorFunctions(params.fn_alloc, params.fn_dealloc, params.alloc_userdata);
            ImGui::SetCurrentContext((ImGuiContext*)params.native_context);
            return true;
        }
#if ISP_WINDOWS
        else if (context_name == "devui-context/imguizmo"_sid)
        {
            ImGuizmo::SetImGuizmoContext((ImGuizmo::ImGuizmoContext*)params.native_context);
            return true;
        }
#endif
        return false;
    }

    bool devui_setup_context(
        ice::ModuleQuery const& query,
        ice::FnDevUIContextSetupCallback callback,
        void* userdata
    ) noexcept
    {
        api::DevUI_API api;
        if (query.query_api(api))
        {
            ICE_ASSERT_CORE(api.fn_context_setup != nullptr);
            api.fn_context_setup(callback, userdata);

            // Store some of the API pointers
            global_context_setup_menu = api.fn_context_setup_menu;
            global_context_trait_name = api.fn_context_trait_name;
            global_context_register_widget = api.fn_context_register_widget;
            global_context_remove_widget = api.fn_context_remove_widget;
            return true;
        }
        return false;
    }

    auto create_devui_context(
        ice::Allocator& alloc,
        ice::ModuleQuery& query
    ) noexcept -> ice::UniquePtr<ice::DevUIContext>
    {
        ice::UniquePtr<ice::DevUIContext> result{};

        api::DevUI_API api;
        if (query.query_api(api))
        {
            ICE_ASSERT_CORE(api.fn_create_context != nullptr);
            ICE_ASSERT_CORE(api.fn_destry_context != nullptr);
            ICE_ASSERT_CORE(api.fn_context_setup != nullptr);

            if (ice::DevUIContext* context = api.fn_create_context(alloc); context != nullptr)
            {
                result = ice::make_unique<ice::DevUIContext>(api.fn_destry_context, context);

                ice::devui_setup_context(query, ice::devui_context_setup_default);
            }
        }

        return result;
    }

} // namespace ice
