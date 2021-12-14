#pragma once
#include <ice/stringid.hxx>
#include <ice/gfx/gfx_resource.hxx>

namespace ice::gfx
{

    class GfxResourceTracker;

    template<typename T>
    inline void track_resource(
        ice::gfx::GfxResourceTracker& tracker,
        ice::StringID_Arg name,
        T resource_handle
    ) noexcept;

    template<typename T>
    inline auto find_resource(
        ice::gfx::GfxResourceTracker& tracker,
        ice::StringID_Arg name
    ) noexcept -> T;

    class GfxResourceTracker
    {
    public:
        virtual ~GfxResourceTracker() noexcept = default;

        virtual void track_resource(
            ice::StringID_Arg name,
            ice::gfx::GfxResource resource
        ) noexcept = 0;

        virtual auto find_resource(
            ice::StringID_Arg name,
            ice::gfx::GfxResource::Type type
        ) noexcept -> ice::gfx::GfxResource = 0;
    };

    namespace detail
    {

        template<typename T>
        constexpr GfxResource::Type Constant_GfxResourceType = GfxResource::Type::Invalid;

        template<>
        constexpr GfxResource::Type Constant_GfxResourceType<ice::render::Framebuffer> = GfxResource::Type::Framebuffer;

        template<>
        constexpr GfxResource::Type Constant_GfxResourceType<ice::render::Renderpass> = GfxResource::Type::Renderpass;

        template<>
        constexpr GfxResource::Type Constant_GfxResourceType<ice::render::ResourceSetLayout> = GfxResource::Type::ResourceSetLayout;

        template<>
        constexpr GfxResource::Type Constant_GfxResourceType<ice::render::PipelineLayout> = GfxResource::Type::PipelineLayout;

        template<>
        constexpr GfxResource::Type Constant_GfxResourceType<ice::render::ResourceSet> = GfxResource::Type::ResourceSet;

        template<>
        constexpr GfxResource::Type Constant_GfxResourceType<ice::render::Buffer> = GfxResource::Type::Buffer;

        template<>
        constexpr GfxResource::Type Constant_GfxResourceType<ice::render::Image> = GfxResource::Type::Image;

        template<>
        constexpr GfxResource::Type Constant_GfxResourceType<ice::render::Sampler> = GfxResource::Type::Sampler;

        template<typename T>
        auto constexpr MemberPtr_GfxResourceValue() noexcept
        {
            return nullptr;
        }

        template<>
        auto constexpr MemberPtr_GfxResourceValue<ice::render::Framebuffer>() noexcept
        {
            return &GfxResource::Value::framebuffer;
        }

        template<>
        auto constexpr MemberPtr_GfxResourceValue<ice::render::Renderpass>() noexcept
        {
            return &GfxResource::Value::renderpass;
        }

        template<>
        auto constexpr MemberPtr_GfxResourceValue<ice::render::ResourceSetLayout>() noexcept
        {
            return &GfxResource::Value::resourceset_layout;
        }

        template<>
        auto constexpr MemberPtr_GfxResourceValue<ice::render::PipelineLayout>() noexcept
        {
            return &GfxResource::Value::pipeline_layout;
        }

        template<>
        auto constexpr MemberPtr_GfxResourceValue<ice::render::ResourceSet>() noexcept
        {
            return &GfxResource::Value::resourceset;
        }

        template<>
        auto constexpr MemberPtr_GfxResourceValue<ice::render::Buffer>() noexcept
        {
            return &GfxResource::Value::buffer;
        }

        template<>
        auto constexpr MemberPtr_GfxResourceValue<ice::render::Image>() noexcept
        {
            return &GfxResource::Value::image;
        }

        template<>
        auto constexpr MemberPtr_GfxResourceValue<ice::render::Sampler>() noexcept
        {
            return &GfxResource::Value::sampler;
        }

    } // namespace detail

    template<typename T>
    inline void track_resource(
        ice::gfx::GfxResourceTracker& tracker,
        ice::StringID_Arg name,
        T resource_handle
    ) noexcept
    {
        static_assert(
            detail::Constant_GfxResourceType<T> != GfxResource::Type::Invalid,
            "Trying to track unknown resource handle type!"
        );

        if constexpr (detail::Constant_GfxResourceType<T> != GfxResource::Type::Invalid)
        {
            GfxResource resource{ .type = detail::Constant_GfxResourceType<T> };
            (resource.value.*detail::MemberPtr_GfxResourceValue<T>()) = resource_handle;
            tracker.track_resource(name, resource);
        }
    }

    template<typename T>
    inline auto find_resource(
        ice::gfx::GfxResourceTracker& tracker,
        ice::StringID_Arg name
    ) noexcept -> T
    {
        static_assert(
            detail::Constant_GfxResourceType<T> != GfxResource::Type::Invalid,
            "Trying to find resource with unknown handle type!"
        );

        if constexpr (detail::Constant_GfxResourceType<T> != GfxResource::Type::Invalid)
        {
            GfxResource resource = tracker.find_resource(name, detail::Constant_GfxResourceType<T>);
            return (resource.value.*detail::MemberPtr_GfxResourceValue<T>());
        }
    }

} // namespace ice::gfx
