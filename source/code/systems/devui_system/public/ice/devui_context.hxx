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

    auto devui_trait_name() noexcept -> ice::StringID;

    bool devui_register_widget(
        ice::DevUIWidget* widget
    ) noexcept;

    bool devui_setup_context(
        ice::ModuleQuery const& query,
        ice::FnDevUIContextSetupCallback callback,
        void* userdata = nullptr
    ) noexcept;

    auto create_devui_context(
        ice::Allocator& alloc,
        ice::ModuleQuery& query
    ) noexcept -> ice::UniquePtr<ice::DevUIContext>;

} // namespace ice
