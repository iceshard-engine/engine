#include <render_system/render_api.hxx>

namespace render::api::v1
{

    void api_v1_not_initialized() noexcept
    {
        fmt::print("Render API v1 was not properly initialized in this module!\n");
        std::abort();
    }

    void assert_render_api() noexcept
    {
        render::api::v1::render_api_instance->check_func();
    }

    static RenderInterface render_api_instance_uninitialized{ &api_v1_not_initialized };

    RenderInterface* render_api_instance{ &render_api_instance_uninitialized };

} // namespace api::v1
