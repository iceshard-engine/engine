/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "ice_gfx_object_storage.hxx"
#include <ice/render/render_device.hxx>

namespace ice::gfx
{

    SimpleGfxObjectStorage::SimpleGfxObjectStorage(ice::Allocator& alloc) noexcept
        : _objects{ alloc }
    {
        ice::hashmap::reserve(_objects, 100);
    }

    bool SimpleGfxObjectStorage::set(ice::StringID_Arg name, ice::gfx::GfxObject object) noexcept
    {
        ice::u64 const hashname = ice::hash(name);
        bool const has_object = ice::hashmap::has(_objects, hashname);

        if (has_object)
        {
            if (object.valid())
            {
                return false;
            }
            else
            {
                ice::hashmap::remove(_objects, hashname);
                return true;
            }
        }

        ice::hashmap::set(_objects, hashname, object);
        return true;
    }

    auto SimpleGfxObjectStorage::get(ice::StringID_Arg name) const noexcept -> ice::gfx::GfxObject
    {
        static GfxObject unknown_object{ GfxObjectType::Unknown, GfxObjectFlags::None, nullptr };
        return ice::hashmap::get(_objects, ice::hash(name), unknown_object);
    }

    static void destroy_object(ice::render::RenderDevice& device, GfxObject object) noexcept
    {
        using namespace ice::render;
        switch (object._type)
        {
        case GfxObjectType::Renderpass: device.destroy_renderpass(object.get<Renderpass>()); break;
        case GfxObjectType::Framebuffer: device.destroy_framebuffer(object.get<Framebuffer>()); break;
        case GfxObjectType::Pipeline: device.destroy_pipeline(object.get<Pipeline>()); break;
        case GfxObjectType::PipelineLayout: device.destroy_pipeline_layout(object.get<PipelineLayout>()); break;
        case GfxObjectType::ResourceSet:
        {
            auto rs = object.get<ResourceSet>();
            device.destroy_resourcesets({ &rs, 1 });
            break;
        }
        case GfxObjectType::ResourceSetLayout: device.destroy_resourceset_layout(object.get<ResourceSetLayout>()); break;
        case GfxObjectType::Shader: device.destroy_shader(object.get<Shader>()); break;
        case GfxObjectType::Image: device.destroy_image(object.get<Image>()); break;
        case GfxObjectType::Buffer: device.destroy_buffer(object.get<render::Buffer>()); break;
        }
    }

    void SimpleGfxObjectStorage::destroy_all(ice::render::RenderDevice& device) noexcept
    {
        for (auto entry : _objects)
        {
            if (has_none(entry._flags, GfxObjectFlags::Imported))
            {
                destroy_object(device, entry);
            }
        }
        ice::hashmap::clear(_objects);
    }

} // namespace ice::gfx
