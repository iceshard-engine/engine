#pragma once
#include <ice/stringid.hxx>
#include <ice/allocator.hxx>
#include <ice/span.hxx>
#include <ice/pod/array.hxx>
#include <ice/render/render_device.hxx>
#include <ice/render/render_resource.hxx>

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

    struct SurfaceInfo;

    class RenderSurface;

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
            ice::pod::Array<ice::render::QueueFamilyInfo>& queue_info
        ) noexcept = 0;

        virtual auto create_device(
            ice::Span<ice::render::QueueInfo const> queue_info
        ) noexcept -> ice::render::RenderDevice* = 0;

        virtual void destroy_device(
            ice::render::RenderDevice* device
        ) noexcept = 0;

        virtual void create_buffer() noexcept = 0;
        virtual void release_buffer() noexcept = 0;

        virtual void create_resource_set() noexcept = 0;
        virtual void release_resource_set() noexcept = 0;
    };

    //namespace detail::v1
    //{

    //    using CreateFn = auto(ice::Allocator&) noexcept -> ice::render::RenderDriver*;
    //    using DestroyFn = void(ice::Allocator&, ice::render::RenderDriver*) noexcept;

    //    struct RenderAPI
    //    {
    //        CreateFn* create_driver_fn;
    //        DestroyFn* destroy_driver_fn;
    //    };

    //} // namespace detail::v1


    //namespace api
    //{

    //    struct API_Driver;

    //    using DriverInstance = struct DriverInstanceHandle*;
    //    struct DriverInstanceCreateInfo;

    //} // namespace api


    //class RenderDriver
    //{
    //public:
    //    explicit RenderDriver(
    //        ice::Allocator& alloc,
    //        ice::render::api::API_Driver* driver_api
    //    ) noexcept;
    //    ~RenderDriver() noexcept;

    //    RenderDriver(RenderDriver&& other) noexcept;
    //    auto operator=(RenderDriver&& other) noexcept -> RenderDriver&;

    //    RenderDriver(RenderDriver const&) noexcept = delete;
    //    auto operator=(RenderDriver const&) noexcept -> RenderDriver& = delete;

    //private:
    //    ice::Allocator& _allocator;
    //    ice::render::api::API_Driver* _api;
    //    ice::render::api::DriverInstance _instance;
    //};

    //namespace api
    //{

    //    struct DriverInstanceCreateInfo
    //    {
    //    };

    //    struct API_Driver
    //    {
    //        bool(*fn_get_api)(ice::StringID_Hash, ice::u32, void**) noexcept;

    //        bool(*fn_create_driver_instance)(ice::Allocator*, DriverInstanceCreateInfo const*, DriverInstance*) noexcept;
    //        auto(*fn_driver_instance_api_version)() noexcept -> ice::u32;
    //        void(*fn_destroy_driver_instance)(DriverInstance) noexcept;

    //        bool(*fn_create_resource_set)(void*, ResourceSetCreateInfo const*, ResourceSet*);
    //        bool(*fn_destroy_resource_set)(void*, ResourceSet);

    //        void* reserved[32];
    //    };

    //} // namespace api

} // namespace ice::render