#pragma once
#include <ice/stringid.hxx>
#include <ice/gfx/gfx_object.hxx>

namespace ice::gfx::v2
{

    struct GfxObjectStorage
    {
        virtual ~GfxObjectStorage() noexcept = default;

        virtual bool set(
            ice::StringID_Arg name,
            ice::gfx::v2::GfxObject object
        ) noexcept = 0;

        virtual auto get(
            ice::StringID_Arg name
        ) const noexcept -> ice::gfx::v2::GfxObject = 0;

        template<typename RenderHandle>
        bool try_get(ice::StringID_Arg name, RenderHandle& out_handle) const noexcept;

        template<typename RenderHandle>
        bool try_set(ice::StringID_Arg name, RenderHandle handle) noexcept;

        template<typename RenderHandle>
        auto get(ice::StringID_Arg name) const noexcept -> RenderHandle;

        template<typename RenderHandle>
        void set(ice::StringID_Arg name, RenderHandle handle) noexcept;

        template<typename RenderHandle>
        bool get_by_index(ice::ucount idx, RenderHandle& out_handle) const noexcept { return false; }
    };

    template<typename RenderHandle>
    bool GfxObjectStorage::try_get(ice::StringID_Arg name, RenderHandle& out_handle) const noexcept
    {
        return get(name).try_get(out_handle);
    }

    template<typename RenderHandle>
    bool GfxObjectStorage::try_set(ice::StringID_Arg name, RenderHandle handle) noexcept
    {
        GfxObject object{ Constant_GfxObjectType<RenderHandle> };
        object.set(handle);
        return set(name, object);
    }

    template<typename RenderHandle>
    auto GfxObjectStorage::get(ice::StringID_Arg name) const noexcept -> RenderHandle
    {
        RenderHandle result;
        try_get(name, result);
        return result;
    }

    template<typename RenderHandle>
    void GfxObjectStorage::set(ice::StringID_Arg name, RenderHandle handle) noexcept
    {
        bool const success = try_set(name, handle);
        ICE_ASSERT(success, "Failed to store render handle in storage with name: {}", name);
    }

} // namespace ice::gfx::v2
