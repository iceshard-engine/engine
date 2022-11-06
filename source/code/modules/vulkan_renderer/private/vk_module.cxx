/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/render/render_module.hxx>
#include <ice/log_module.hxx>
#include <ice/assert.hxx>

#include "vk_driver.hxx"
#include "vk_utility.hxx"

namespace ice::render::vk
{


    auto create_vulkan_driver(ice::Allocator& alloc) noexcept -> ice::render::RenderDriver*
    {
        ice::UniquePtr<VulkanAllocator> vk_alloc = ice::make_unique<VulkanAllocator>(alloc, alloc);

        VkApplicationInfo app_info{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
        app_info.pApplicationName = "IceShard";
        app_info.applicationVersion = 1;
        app_info.pEngineName = "IceShard (alpha)";
        app_info.engineVersion = 1;
        app_info.apiVersion = VK_API_VERSION_1_0;

        const char* instanceExtensionNames[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
            VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
        };

        VkInstanceCreateInfo instance_create_info{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        instance_create_info.flags = 0;
        instance_create_info.pApplicationInfo = &app_info;
        instance_create_info.enabledLayerCount = 0;
        instance_create_info.ppEnabledLayerNames = nullptr;
        instance_create_info.enabledExtensionCount = static_cast<uint32_t>(std::size(instanceExtensionNames));
        instance_create_info.ppEnabledExtensionNames = &instanceExtensionNames[0];

        VkInstance vk_instance;
        VkResult const vk_create_result = vkCreateInstance(&instance_create_info, vk_alloc->vulkan_callbacks(), &vk_instance);
        ICE_ASSERT(vk_create_result == VkResult::VK_SUCCESS, "Creation of Vulkan instance failed!");

        return alloc.create<VulkanRenderDriver>(alloc, ice::move(vk_alloc), vk_instance);
    }

    auto destroy_vulkan_driver(ice::render::RenderDriver* driver) noexcept
    {
        ice::Allocator& alloc = static_cast<VulkanRenderDriver*>(driver)->allocator();
        alloc.destroy(driver);
    }

    bool vulkan_driver_get_api_proc(ice::StringID_Hash name, ice::u32 version, void** api_ptr) noexcept
    {
        static ice::render::detail::v1::RenderAPI driver_api{
            .create_driver_fn = create_vulkan_driver,
            .destroy_driver_fn = destroy_vulkan_driver,
        };

        if (name == "ice.render-api"_sid_hash)
        {
            *api_ptr = &driver_api;
            return true;
        }

        return false;
    }

} // ice::render::vk


extern "C"
{

    __declspec(dllexport) void ice_module_load(
        ice::Allocator* alloc,
        ice::ModuleNegotiatorContext* ctx,
        ice::ModuleNegotiator* negotiator
    )
    {
        using ice::operator""_sid_hash;
        ice::initialize_log_module(ctx, negotiator);
        ice::register_log_tag(ice::render::vk::log_tag);

        negotiator->fn_register_module(ctx, "ice.render-api"_sid_hash, ice::render::vk::vulkan_driver_get_api_proc);
    }

    __declspec(dllexport) void ice_module_unload(
        ice::Allocator* alloc
    )
    {
    }

}
