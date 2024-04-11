#pragma once
#include <ice/devui_types.hxx>
#include <ice/module_types.hxx>
#include <ice/mem_unique_ptr.hxx>

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

    auto devui_trait_name() noexcept -> ice::StringID;

    bool devui_register_widget(
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
