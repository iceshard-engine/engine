#include "imgui_system.hxx"
#include <ice/devui/devui_module.hxx>

#include <ice/allocator.hxx>
#include <ice/module_register.hxx>
#include <ice/log_module.hxx>

namespace ice::devui
{

    auto create_imgui_devui(ice::Allocator& alloc) noexcept -> ice::devui::DevUISystem*
    {
        return alloc.make<ImGuiSystem>(alloc);
    }

    auto destroy_imgui_devui(ice::Allocator& alloc, ice::devui::DevUISystem* system) noexcept
    {
        alloc.destroy(static_cast<ImGuiSystem*>(system));
    }

    bool iceshard_mesh_pipeline_api(
        ice::StringID_Hash name,
        ice::u32 version,
        void** api_ptr
    ) noexcept
    {
        static ice::devui::detail::v1::DevUI_API devui_api{
            .create_system_fn = create_imgui_devui,
            .destroy_system_fn = destroy_imgui_devui,
        };

        if (name == "ice.devui_module"_sid_hash && version == 1)
        {
            *api_ptr = &devui_api;
            return true;
        }
        return false;
    }

    bool ice_module_load(
        ice::Allocator* alloc,
        ice::ModuleNegotiatorContext* ctx,
        ice::ModuleNegotiator* negotiator
    ) noexcept
    {
        ice::initialize_log_module(ctx, negotiator);

        negotiator->fn_register_module(ctx, "ice.devui_module"_sid_hash, iceshard_mesh_pipeline_api);
        return true;
    }

} // namespace ice

extern "C"
{

    // #TODO: https://github.com/iceshard-engine/engine/issues/92
#if ISP_WINDOWS
    __declspec(dllexport) bool ice_module_load(
        ice::Allocator* alloc,
        ice::ModuleNegotiatorContext* ctx,
        ice::ModuleNegotiator* negotiator
    )
    {
        return ice::devui::ice_module_load(alloc, ctx, negotiator);
    }

    __declspec(dllexport) bool ice_module_unload(
        ice::Allocator* alloc
    )
    {
        return true;
    }
#endif 

} // extern "C"
