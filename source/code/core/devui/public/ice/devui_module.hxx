#pragma once
#include <ice/stringid.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/module_types.hxx>
#include <ice/devui_types.hxx>

namespace ice::api
{

    namespace devui::v1
    {

        struct DevUI_API
        {
            static constexpr ice::StringID Constant_APIName = "iceshard/api/devui-v1"_sid;
            static constexpr ice::u32 Constant_APIVersion = 1;

            using FnCreateContext = auto(*)(ice::Allocator& alloc) noexcept -> ice::DevUIContext*;
            using FnDestroyContext = void(*)(ice::DevUIContext* context) noexcept;

            using FnContextSetupCallback = ice::FnDevUIContextSetupCallback;
            using FnContextSetup = void(*)(FnContextSetupCallback callback, void* userdata) noexcept;
            using FnContextRegisterWidget = void(*)(ice::DevUIWidget* widget) noexcept;
            using FnContextRemoveWidget = void(*)(ice::DevUIWidget* widget) noexcept;
            using FnContextTraitName = auto(*)() noexcept -> ice::StringID;

            FnCreateContext fn_create_context;
            FnDestroyContext fn_destry_context;
            FnContextSetup fn_context_setup;
            FnContextRegisterWidget fn_context_register_widget;
            FnContextRemoveWidget fn_context_remove_widget;
            FnContextTraitName fn_context_trait_name;
        };

    } // inline namespace devui::v1

    // The default API namespace
    using namespace devui::v1;

} // namespace ice::api
