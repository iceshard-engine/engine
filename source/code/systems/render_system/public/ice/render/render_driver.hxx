#pragma once
#include <ice/span.hxx>
#include <ice/stringid.hxx>
#include <ice/mem_allocator.hxx>
#include <ice/container/array.hxx>
#include <ice/render/render_declarations.hxx>
#include <ice/render/render_queue.hxx>

namespace ice::render
{

    enum class RenderDriverAPI
    {
        None = 0x0,
        DirectX11,
        DirectX12,
        Vulkan,
        Metal,
        OpenGL,
    };

    class RenderDriver
    {
    public:
        virtual ~RenderDriver() noexcept = default;

        virtual auto render_api() const noexcept -> ice::render::RenderDriverAPI = 0;

        virtual auto create_surface(
            ice::render::SurfaceInfo const& surface_info
        ) noexcept -> ice::render::RenderSurface* = 0;

        virtual void destroy_surface(
            ice::render::RenderSurface* surface
        ) noexcept = 0;

        virtual void query_queue_infos(
            ice::Array<ice::render::QueueFamilyInfo>& queue_info
        ) noexcept = 0;

        virtual auto create_device(
            ice::Span<ice::render::QueueInfo const> queue_info
        ) noexcept -> ice::render::RenderDevice* = 0;

        virtual void destroy_device(
            ice::render::RenderDevice* device
        ) noexcept = 0;
    };

} // namespace ice::render
