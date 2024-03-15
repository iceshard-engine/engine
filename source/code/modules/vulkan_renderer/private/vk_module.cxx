/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/render/render_module.hxx>
#include <ice/log_module.hxx>
#include <ice/assert.hxx>

#include "vk_driver.hxx"
#include "vk_utility.hxx"
#include "vk_extensions.hxx"

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
        app_info.apiVersion = VK_API_VERSION_1_3;

        ice::ucount layer_count = 0;
        ice::Array<ExtensionName> names{ alloc };
        Extension extensions = extensions_gather_names(names, layer_count, ExtensionTarget::InstanceLayer);

        ice::ucount extension_count = 0;
        extensions |= extensions_gather_names(names, extension_count, ExtensionTarget::InstanceExtension);
        ICE_ASSERT_CORE(ice::has_all(extensions, Extension::VkI_Surface));
        ICE_ASSERT_CORE(ice::has_any(extensions, Extension::VkI_AndroidSurface | Extension::VkI_Win32Surface));

        VkInstanceCreateInfo instance_create_info{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        instance_create_info.flags = 0;
        instance_create_info.pApplicationInfo = &app_info;
        instance_create_info.enabledLayerCount = layer_count;
        instance_create_info.ppEnabledLayerNames = ice::array::begin(names);
        instance_create_info.enabledExtensionCount = extension_count;
        instance_create_info.ppEnabledExtensionNames = ice::array::begin(names) + layer_count;

        VkInstance vk_instance;
        VkResult const vk_create_result = vkCreateInstance(&instance_create_info, vk_alloc->vulkan_callbacks(), &vk_instance);
        ICE_ASSERT(vk_create_result == VkResult::VK_SUCCESS, "Creation of Vulkan instance failed!");

        // Release the array backing data
        ice::array::set_capacity(names, 0);

        return alloc.create<VulkanRenderDriver>(alloc, ice::move(vk_alloc), vk_instance, extensions);
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
