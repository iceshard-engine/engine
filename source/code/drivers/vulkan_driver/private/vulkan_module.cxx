#include <core/memory.hxx>
#include <core/pointer.hxx>
#include <core/pod/array.hxx>
#include <core/pod/hash.hxx>
#include <core/message/buffer.hxx>
#include <core/message/operations.hxx>
#include <core/allocators/proxy_allocator.hxx>

#include <iceshard/renderer/render_commands.hxx>
#include <iceshard/renderer/render_module.hxx>
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>

#include "api/v1/vulkan_render_api.hxx"

#include "vulkan_allocator.hxx"
#include "vulkan_device_memory_manager.hxx"
#include "vulkan_image.hxx"
#include "vulkan_buffer.hxx"

#include <iceshard/renderer/render_pass.hxx>
#include <iceshard/renderer/vulkan/vulkan_system.hxx>

#include <vulkan/vulkan.h>

namespace iceshard::renderer::vulkan
{

    class VulkanModule : public RenderModule
    {
    public:
        VulkanModule(core::allocator& alloc) noexcept
            : _driver_allocator{ "vulkan-driver", alloc }
            , _vulkan_allocator{ alloc }
        {
            const char* instanceExtensionNames[] = {
                VK_KHR_SURFACE_EXTENSION_NAME,
                VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
                VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
            };

            VkApplicationInfo app_info = {};
            app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            app_info.pNext = nullptr;
            app_info.pApplicationName = "IceShard (vulkan)";
            app_info.applicationVersion = 1;
            app_info.pEngineName = "IceShard";
            app_info.engineVersion = 1;
            app_info.apiVersion = VK_API_VERSION_1_0;

            VkInstanceCreateInfo instance_create_info = {};
            instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            instance_create_info.pNext = nullptr;
            instance_create_info.flags = 0;
            instance_create_info.pApplicationInfo = &app_info;
            instance_create_info.enabledLayerCount = 0;
            instance_create_info.ppEnabledLayerNames = nullptr;
            instance_create_info.enabledExtensionCount = static_cast<uint32_t>(std::size(instanceExtensionNames));
            instance_create_info.ppEnabledExtensionNames = &instanceExtensionNames[0];

            auto vk_create_result = vkCreateInstance(&instance_create_info, _vulkan_allocator.vulkan_callbacks(), &_vulkan_instance);
            IS_ASSERT(vk_create_result == VkResult::VK_SUCCESS, "Creation of Vulkan instance failed!");

            _vk_render_system = iceshard::renderer::vulkan::create_render_system(_driver_allocator, _vulkan_instance);
        }

        ~VulkanModule() noexcept
        {
            iceshard::renderer::vulkan::destroy_render_system(_driver_allocator, _vk_render_system);

            // We need to provide the callbacks when destroying the instance.
            vkDestroyInstance(_vulkan_instance, _vulkan_allocator.vulkan_callbacks());
        }

        auto render_module_interface() const noexcept -> api::RenderModuleInterface* override
        {
            return iceshard::renderer::api::v1_1::render_module_api;
        }

    private:
        // Allocator for driver allocations.
        core::memory::proxy_allocator _driver_allocator;

        // Special allocator for vulkan render system.
        render::vulkan::VulkanAllocator _vulkan_allocator;

        // The Vulkan instance handle.
        VkInstance _vulkan_instance{};

        // Array vulkan devices.
        core::memory::unique_pointer<iceshard::renderer::vulkan::VulkanDeviceMemoryManager> _vulkan_device_memory{ nullptr, { core::memory::globals::null_allocator() } };

    public:
        ////////////////////////////////////////////////////////////////
        // New RenderSystem object
        iceshard::renderer::vulkan::VulkanRenderSystem* _vk_render_system = nullptr;
    };

} // namespace render

extern "C"
{
    __declspec(dllexport) auto create_render_module(
        core::allocator& alloc,
        core::cexpr::stringid_hash_type api_version
    ) -> iceshard::renderer::RenderModule*
    {
        using namespace iceshard::renderer;

        vulkan::VulkanModule* result = nullptr;
        if (api_version == api::v1_1::version_name.hash_value)
        {
            api::v1_1::vulkan::init_api(api::v1_1::render_module_api);
            result = alloc.make<vulkan::VulkanModule>(alloc);

            api::v1_1::render_module_api->internal_data = result->_vk_render_system;
        }
        return result;
    }

    __declspec(dllexport) void release_render_module(
        core::allocator& alloc,
        iceshard::renderer::RenderModule* module
    )
    {
        alloc.destroy(module);
    }
}
