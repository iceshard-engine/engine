/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/render/render_module.hxx>
#include <ice/log_module.hxx>
#include <ice/assert.hxx>

#include "vk_driver.hxx"
#include "vk_utility.hxx"

namespace ice::render::vk
{

#if ISP_WINDOWS
    static constexpr char const* instanceExtensionNames[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    };
#elif ISP_ANDROID
    static constexpr char const* instanceExtensionNames[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
    };
#endif

    auto create_vulkan_driver(ice::Allocator& alloc) noexcept -> ice::render::RenderDriver*
    {
        ice::UniquePtr<VulkanAllocator> vk_alloc = ice::make_unique<VulkanAllocator>(alloc, alloc);

        VkApplicationInfo app_info{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
        app_info.pApplicationName = "IceShard";
        app_info.applicationVersion = 1;
        app_info.pEngineName = "IceShard (alpha)";
        app_info.engineVersion = 1;
        app_info.apiVersion = VK_API_VERSION_1_3;
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

    struct VulkanModule : ice::Module<VulkanModule>
    {
        static void v1_driver_api(ice::render::detail::v1::RenderAPI& api) noexcept
        {
            api.create_driver_fn = create_vulkan_driver;
            api.destroy_driver_fn = destroy_vulkan_driver;
        }

        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator const& negotiator) noexcept
        {
            ice::LogModule::init(alloc, negotiator);
            ice::log_tag_register(log_tag);
            return negotiator.register_api(v1_driver_api);
        }

        IS_WORKAROUND_MODULE_INITIALIZATION(VulkanModule);
    };

} // ice::render::vk
