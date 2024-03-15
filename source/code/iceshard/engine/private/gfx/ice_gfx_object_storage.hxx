/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_object_storage.hxx>
#include <ice/container/hashmap.hxx>

namespace ice::gfx
{

#if 0 // DEPRECATED
    class SimpleGfxObjectStorage : public ice::gfx::GfxObjectStorage
    {
    public:
        SimpleGfxObjectStorage(ice::Allocator& alloc) noexcept;

        bool set(ice::StringID_Arg name, ice::gfx::GfxObject object) noexcept override;
        auto get(ice::StringID_Arg name) const noexcept -> ice::gfx::GfxObject override;
        void destroy_all(ice::render::RenderDevice& device) noexcept;

    private:
        ice::HashMap<ice::gfx::GfxObject> _objects;
    };
#endif

} // namespace ice::gfx
