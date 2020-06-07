#include <iceshard/renderer/render_api.hxx>

namespace iceshard::renderer::api::v1_1
{

    void api_v1_not_initialized() noexcept
    {
        fmt::print("Render API v1 was not properly initialized in this module!\n");
        std::abort();
    }

    void assert_render_api() noexcept
    {
        render_module_api->check_func();
    }

    static RenderModuleInterface render_api_instance_uninitialized{ &api_v1_not_initialized };

    RenderModuleInterface* render_module_api{ &render_api_instance_uninitialized };

} // namespace api::v1
