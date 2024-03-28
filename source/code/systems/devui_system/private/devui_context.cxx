#include <ice/devui_context.hxx>
#include <ice/devui_module.hxx>
#include <ice/module_query.hxx>
#include <ice/log.hxx>
#include <imgui/imgui.h>

namespace ice
{

    static ice::api::devui::v1::DevUI_API::FnContextRegisterWidget global_context_register_widget = nullptr;
    static ice::api::devui::v1::DevUI_API::FnContextTraitName global_context_trait_name = nullptr;

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
            global_context_trait_name = api.fn_context_trait_name;
            global_context_register_widget = api.fn_context_register_widget;
            return true;
        }
        return false;
    }

    bool devui_oncreate_context_setup(ice::StringID_Arg name, void* context, void*) noexcept
    {
        if (name == "devui-context/imgui"_sid)
        {
            ICE_ASSERT_CORE(ImGui::GetCurrentContext() == nullptr);
            ImGui::SetCurrentContext((ImGuiContext*) context);
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

                devui_setup_context(query, devui_oncreate_context_setup);
            }
        }

        return result;
    }

} // namespace ice
