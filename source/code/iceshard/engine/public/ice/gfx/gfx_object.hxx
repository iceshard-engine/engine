#pragma once
#include <ice/render/render_declarations.hxx>
#include <ice/assert.hxx>

namespace ice::gfx::v2
{

    enum class GfxObjectType : ice::u8
    {
        Unknown,
        Renderpass,
        Framebuffer,
        Pipeline,
        PipelineLayout,
        ResourceSet,
        ResourceSetLayout,
        Shader,
        Image,
        Buffer,
    };

    enum class GfxObjectFlags : ice::u8
    {
        None,
        Imported,
    };

    template<typename RenderHandle>
    constexpr GfxObjectType Constant_GfxObjectType = GfxObjectType::Unknown;


    using GfxObjectHandle = struct GfxObjectHandleBase*;

    struct GfxObject
    {
        ice::gfx::v2::GfxObjectType _type;
        ice::gfx::v2::GfxObjectFlags _flags;
        ice::gfx::v2::GfxObjectHandle _handle;

        inline bool valid() const noexcept;

        template<typename RenderHandle> requires (Constant_GfxObjectType<RenderHandle> != GfxObjectType::Unknown)
        inline bool try_get(RenderHandle& out_handle) const noexcept;

        template<typename RenderHandle> requires (Constant_GfxObjectType<RenderHandle> != GfxObjectType::Unknown)
        inline bool try_set(RenderHandle handle) noexcept;

        template<typename RenderHandle> requires (Constant_GfxObjectType<RenderHandle> != GfxObjectType::Unknown)
        inline auto get() const noexcept -> RenderHandle;

        template<typename RenderHandle> requires (Constant_GfxObjectType<RenderHandle> != GfxObjectType::Unknown)
        inline void set(RenderHandle handle) noexcept;

        template<typename RenderHandle> requires (Constant_GfxObjectType<RenderHandle> != GfxObjectType::Unknown)
        inline auto direct_reference() noexcept -> RenderHandle*;
    };

    inline bool GfxObject::valid() const noexcept
    {
        return _type != GfxObjectType::Unknown && _handle != nullptr;
    }

    template<typename RenderHandle> requires (Constant_GfxObjectType<RenderHandle> != GfxObjectType::Unknown)
    inline bool GfxObject::try_get(RenderHandle& out_handle) const noexcept
    {
        if (_type == Constant_GfxObjectType<RenderHandle>)
        {
            out_handle = ice::bit_cast<RenderHandle>(_handle);
            return true;
        }
        return false;
    }

    template<typename RenderHandle> requires (Constant_GfxObjectType<RenderHandle> != GfxObjectType::Unknown)
    inline bool GfxObject::try_set(RenderHandle handle) noexcept
    {
        if (_type == Constant_GfxObjectType<RenderHandle>)
        {
            _handle = ice::bit_cast<GfxObjectHandle>(handle);
            return true;
        }
        return false;
    }

    template<typename RenderHandle> requires (Constant_GfxObjectType<RenderHandle> != GfxObjectType::Unknown)
    inline auto GfxObject::get() const noexcept -> RenderHandle
    {
        RenderHandle result;
        bool const success = try_get(result);
        ICE_ASSERT(success, "Failed to read requested type from handle!");
        return result;
    }

    template<typename RenderHandle> requires (Constant_GfxObjectType<RenderHandle> != GfxObjectType::Unknown)
    inline void GfxObject::set(RenderHandle handle) noexcept
    {
        bool const success = try_set(handle);
        ICE_ASSERT(success, "Failed to store requested type as handle!");
    }

    template<typename RenderHandle> requires (Constant_GfxObjectType<RenderHandle> != GfxObjectType::Unknown)
    inline auto GfxObject::direct_reference() noexcept -> RenderHandle*
    {
        if (_type == Constant_GfxObjectType<RenderHandle>)
        {
            return reinterpret_cast<RenderHandle*>(_handle);
        }
        return nullptr;
    }


    template<>
    constexpr GfxObjectType Constant_GfxObjectType<ice::render::Renderpass> = GfxObjectType::Renderpass;
    template<>
    constexpr GfxObjectType Constant_GfxObjectType<ice::render::Framebuffer> = GfxObjectType::Framebuffer;
    template<>
    constexpr GfxObjectType Constant_GfxObjectType<ice::render::Pipeline> = GfxObjectType::Pipeline;
    template<>
    constexpr GfxObjectType Constant_GfxObjectType<ice::render::PipelineLayout> = GfxObjectType::PipelineLayout;
    template<>
    constexpr GfxObjectType Constant_GfxObjectType<ice::render::ResourceSet> = GfxObjectType::ResourceSet;
    template<>
    constexpr GfxObjectType Constant_GfxObjectType<ice::render::ResourceSetLayout> = GfxObjectType::ResourceSetLayout;
    template<>
    constexpr GfxObjectType Constant_GfxObjectType<ice::render::Shader> = GfxObjectType::Shader;
    template<>
    constexpr GfxObjectType Constant_GfxObjectType<ice::render::Image> = GfxObjectType::Image;
    template<>
    constexpr GfxObjectType Constant_GfxObjectType<ice::render::Buffer> = GfxObjectType::Buffer;

} // namespace ice::gfx::v2
