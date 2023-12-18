/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/mem_allocator_proxy.hxx>
#include <ice/task_utils.hxx>
#include <ice/sort.hxx>

#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_pass.hxx>
#include <ice/gfx/gfx_queue.hxx>
#include <ice/render/render_buffer.hxx>
#include <ice/render/render_device.hxx>
#include <ice/render/render_fence.hxx>
#include <ice/render/render_framebuffer.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/render/render_swapchain.hxx>

#include "gfx_dynamic_pass.hxx"

namespace ice::gfx
{

    struct IceGfxSubPass
    {

    };

    class IceGfxStaticPass : public GfxPass
    {
    public:
        IceGfxStaticPass() noexcept;
        ~IceGfxStaticPass() noexcept override;
    };

    auto create_static_pass(
        ice::Allocator& alloc,
        ice::render::RenderDevice& render_device,
        ice::gfx::GfxPassInfo const& pass_description
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxPass>
    {
        return { };
    }

    auto create_dynamic_pass(
        ice::Allocator& allocator
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxDynamicPass>
    {
        return ice::make_unique<ice::gfx::IceGfxDynamicPass>(allocator, allocator);
    }


    namespace v2
    {


        //union RenderObject
        //{
        //    ice::render::Renderpass renderpass;
        //    ice::render::Framebuffer framebuffer;
        //    ice::render::Pipeline pipeline;
        //    ice::render::PipelineLayout pipeline_layout;
        //    ice::render::ResourceSet resourceset;
        //    ice::render::ResourceSetLayout resourceset_layout;
        //    ice::render::Shader shader;
        //    ice::render::Image image;
        //    ice::render::Buffer buffer;
        //};

        //static_assert(sizeof(RenderObject) == 8);

        //struct RenderObjectEntry
        //{
        //    RenderObject object;
        //    ice::u8 type;
        //    bool imported;
        //};

        //static_assert(sizeof(RenderObjectEntry) == 16);

        //struct RenderObjects final : ice::gfx::v2::GfxResources
        //{
        //    RenderObjects(ice::Allocator& alloc) noexcept;

        //    ~RenderObjects() noexcept override
        //    {
        //        ICE_ASSERT(ice::hashmap::count(_objects) == 0, "Objects improperly cleared!");
        //    }

        //    void set(ice::StringID_Arg name, RenderObject object, ice::u8 type, bool imported) noexcept
        //    {
        //        ICE_ASSERT(find(name, type).renderpass == ice::render::Renderpass::Invalid, "Object with type and name already exists!");
        //        ice::multi_hashmap::insert(_objects, ice::hash(name), { object, type, imported });
        //    }

        //    template<ice::u8 TypeID, typename Enum, Enum RenderObject::* FieldPtr> requires (std::is_enum_v<Enum>)
        //        void set_internal(ice::StringID_Arg name, Enum value) noexcept
        //    {
        //        RenderObject object;
        //        (object.*FieldPtr) = value;
        //        set(name, object, TypeID, false);
        //    }

        //    void track(ice::StringID_Arg name, ice::render::Renderpass renderpass) noexcept override
        //    {
        //        set_internal<0, ice::render::Renderpass, &RenderObject::renderpass>(name, renderpass);
        //    }

        //    void track(ice::StringID_Arg name, ice::render::Framebuffer framebuffer) noexcept override
        //    {
        //        set_internal<1, ice::render::Framebuffer, &RenderObject::framebuffer>(name, framebuffer);
        //    }

        //    void track(ice::StringID_Arg name, ice::render::Image image) noexcept override
        //    {
        //        set_internal<7, ice::render::Image, &RenderObject::image>(name, image);
        //    }

        //    static void destroy_object(ice::render::RenderDevice& device, RenderObject object, ice::u8 type) noexcept
        //    {
        //        switch (type)
        //        {
        //        case 0:
        //            device.destroy_renderpass(object.renderpass);
        //            break;
        //        case 1:
        //            device.destroy_framebuffer(object.framebuffer);
        //            break;
        //        case 2:
        //            device.destroy_pipeline(object.pipeline);
        //            break;
        //        case 3:
        //            device.destroy_pipeline_layout(object.pipeline_layout);
        //            break;
        //        case 4:
        //            device.destroy_resourcesets({ &object.resourceset, 1 });
        //            break;
        //        case 5:
        //            device.destroy_resourceset_layout(object.resourceset_layout);
        //            break;
        //        case 6:
        //            device.destroy_shader(object.shader);
        //            break;
        //        case 7:
        //            device.destroy_image(object.image);
        //            break;
        //        case 8:
        //            device.destroy_buffer(object.buffer);
        //            break;
        //        }
        //    }

        //    void remove(ice::render::RenderDevice& device, ice::StringID_Arg name, ice::u8 type) noexcept
        //    {
        //        RenderObject result = { ice::render::Renderpass::Invalid };
        //        auto it = ice::multi_hashmap::find_first(_objects, ice::hash(name));
        //        while (it != nullptr)
        //        {
        //            if (it.value().type == type)
        //            {
        //                result = it.value().object;
        //                break;
        //            }

        //            it = ice::multi_hashmap::find_next(_objects, it);
        //        }

        //        if (result.renderpass != ice::render::Renderpass::Invalid)
        //        {
        //            if (it.value().imported == false)
        //            {
        //                destroy_object(device, result, type);
        //            }
        //            ice::multi_hashmap::remove(_objects, it);
        //        }
        //    }

        //    void remove_all(ice::render::RenderDevice& device) noexcept
        //    {
        //        for (auto entry : _objects)
        //        {
        //            if (entry.imported == false)
        //            {
        //                destroy_object(device, entry.object, entry.type);
        //            }
        //        }
        //        ice::hashmap::clear(_objects);
        //    }

        //    auto find(ice::StringID_Arg name, ice::u8 type) const noexcept -> RenderObject
        //    {
        //        RenderObject result = { ice::render::Renderpass::Invalid };
        //        auto it = ice::multi_hashmap::find_first(_objects, ice::hash(name));
        //        while (it != nullptr)
        //        {
        //            if (it.value().type == type)
        //            {
        //                result = it.value().object;
        //                break;
        //            }

        //            it = ice::multi_hashmap::find_next(_objects, it);
        //        }
        //        return result;
        //    }

        //    template<ice::u8 TypeID, typename Enum, Enum RenderObject::* FieldPtr> requires (std::is_enum_v<Enum>)
        //        bool find_internal(ice::StringID_Arg name, Enum& out_object) const noexcept
        //    {
        //        RenderObject const result = find(name, TypeID);
        //        if ((result.*FieldPtr) != Enum::Invalid)
        //        {
        //            out_object = (result.*FieldPtr);
        //            return true;
        //        }
        //        return false;
        //    }

        //    bool find(ice::StringID_Arg name, ice::render::Renderpass& out_renderpass) const noexcept override
        //    {
        //        return find_internal<0, ice::render::Renderpass, &RenderObject::renderpass>(name, out_renderpass);
        //    }

        //    bool find(ice::StringID_Arg name, ice::render::Framebuffer& out_framebuffer) const noexcept override
        //    {
        //        return find_internal<1, ice::render::Framebuffer, &RenderObject::framebuffer>(name, out_framebuffer);
        //    }

        //    bool find(ice::StringID_Arg name, ice::render::Image& out_image) const noexcept override
        //    {
        //        return find_internal<7, ice::render::Image, &RenderObject::image>(name, out_image);
        //    }

        //    ice::HashMap<RenderObjectEntry> _objects;
        //};

    } // namespace v2

} // namespace ice::gfx
