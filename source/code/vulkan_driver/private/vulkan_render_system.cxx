#include <core/memory.hxx>
#include <core/pod/array.hxx>
#include <core/message/buffer.hxx>
#include <core/message/operations.hxx>
#include <core/allocators/proxy_allocator.hxx>

#include <render_system/render_system.hxx>
#include <render_system/render_commands.hxx>

#include "vulkan_allocator.hxx"
#include "device/vulkan_physical_device.hxx"

#include <SDL.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <SDL_syswm.h>

#include <vulkan/vulkan.h>

#include <numeric>

namespace render
{
    namespace detail
    {

        auto vk_iceshard_allocate(void* userdata, size_t size, size_t alignment, [[maybe_unused]] VkSystemAllocationScope scope) noexcept -> void*
        {
            auto* const allocator = reinterpret_cast<render::vulkan::VulkanAllocator*>(userdata);
            return allocator->allocate(static_cast<uint32_t>(size), static_cast<uint32_t>(alignment));
        }

        auto vk_iceshard_reallocate(void* userdata, void* original, size_t size, size_t alignment, [[maybe_unused]] VkSystemAllocationScope scope) noexcept -> void*
        {
            auto* const allocator = reinterpret_cast<render::vulkan::VulkanAllocator*>(userdata);
            return allocator->reallocate(original, static_cast<uint32_t>(size), static_cast<uint32_t>(alignment));
        }

        void vk_iceshard_free(void* userdata, void* memory) noexcept
        {
            auto* const allocator = reinterpret_cast<render::vulkan::VulkanAllocator*>(userdata);
            allocator->deallocate(memory);
        }

        //void vk_iceshard_internal_allocate_event(void* userdata, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope) noexcept
        //{
        //    PFN_vkInternalAllocationNotification f;
        //}

        //void vk_iceshard_internal_free_event(void* userdata, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope) noexcept
        //{
        //    PFN_vkInternalFreeNotification f;
        //}

    } // namespace detail

    class VulkanRenderSystem : public render::RenderSystem
    {
    public:
        VulkanRenderSystem(core::allocator& alloc) noexcept
            : render::RenderSystem{}
            , _driver_allocator{ "vulkan-driver", alloc }
            , _vulkan_allocator{ alloc }
            , _render_window{ nullptr }
            , _command_buffer{ alloc }
            , _vulkan_devices{ _driver_allocator }
        {
            const bool sdl2_init_video = SDL_InitSubSystem(SDL_INIT_VIDEO) == 0;
            if (sdl2_init_video == false)
            {
                IS_ASSERT(sdl2_init_video == true, "Initialization od SDL2 Video subsystem failed! Error: '{}'", SDL_GetError());
            }

            _render_window = SDL_CreateWindow(
                "IceShard - Test window",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                1280, 720,
                SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);

            initialize();
        }

        void initialize() noexcept
        {
            const char* instanceExtensionNames[] = {
                VK_KHR_SURFACE_EXTENSION_NAME,
                VK_KHR_WIN32_SURFACE_EXTENSION_NAME
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
            instance_create_info.enabledExtensionCount = 0;
            instance_create_info.ppEnabledExtensionNames = nullptr;

            VkAllocationCallbacks alloc_callbacks = {};
            alloc_callbacks.pUserData = &_vulkan_allocator;
            alloc_callbacks.pfnAllocation = detail::vk_iceshard_allocate;
            alloc_callbacks.pfnReallocation = detail::vk_iceshard_reallocate;
            alloc_callbacks.pfnFree = detail::vk_iceshard_free;
            alloc_callbacks.pfnInternalAllocation = nullptr;
            alloc_callbacks.pfnInternalFree = nullptr;

            auto vk_create_result = vkCreateInstance(&instance_create_info, &alloc_callbacks, &_vulkan_instance);
            IS_ASSERT(vk_create_result == VkResult::VK_SUCCESS, "Creation of Vulkan instance failed!");

            enumerate_devices();

            if (core::pod::array::any(_vulkan_devices))
            {
                auto* first_device = core::pod::array::front(_vulkan_devices);

                [[maybe_unused]]
                auto* graphics_device = first_device->create_device(render::vulkan::VulkanDeviceQueueType::GraphicsQueue);

                core::pod::Array<render::vulkan::VulkanCommandBuffer*> cmd_buffers{ _driver_allocator };
                graphics_device->create_command_buffers(cmd_buffers, 1);
            }
        }

        void enumerate_devices() noexcept
        {
            uint32_t device_count;
            VkResult res = vkEnumeratePhysicalDevices(_vulkan_instance, &device_count, nullptr);
            IS_ASSERT(res == VK_SUCCESS, "Couldn't properly query the number of available vulkan devices!");

            core::pod::Array<VkPhysicalDevice> devices_handles{ _driver_allocator };
            core::pod::array::resize(devices_handles, device_count);

            vkEnumeratePhysicalDevices(_vulkan_instance, &device_count, &devices_handles[0]);
            IS_ASSERT(res == VK_SUCCESS, "Couldn't properly query available vulkan devices!");

            // Create VulkanPhysicalDevice objects from the handles.
            fmt::print("Available Vulkan devices: {}\n", device_count);
            for (const auto& handle : devices_handles)
            {
                core::pod::array::push_back(_vulkan_devices, _driver_allocator.make<vulkan::VulkanPhysicalDevice>(_driver_allocator, handle));
            }
        }

        void release_devices() noexcept
        {
            for (auto* vulkan_device : _vulkan_devices)
            {
                _driver_allocator.destroy(vulkan_device);
            }
            core::pod::array::clear(_vulkan_devices);
        }

        void shutdown() noexcept
        {
            release_devices();

            VkAllocationCallbacks alloc_callbacks = {};
            alloc_callbacks.pUserData = &_vulkan_allocator;
            alloc_callbacks.pfnAllocation = detail::vk_iceshard_allocate;
            alloc_callbacks.pfnReallocation = detail::vk_iceshard_reallocate;
            alloc_callbacks.pfnFree = detail::vk_iceshard_free;
            alloc_callbacks.pfnInternalAllocation = nullptr;
            alloc_callbacks.pfnInternalFree = nullptr;

            // We need to provide the callbacks when destrying the instance.
            vkDestroyInstance(_vulkan_instance, &alloc_callbacks);
        }

        auto command_buffer() noexcept -> render::RenderCommandBuffer&
        {
            return _command_buffer;
        }

        void swap() noexcept
        {
            _command_buffer.visit([](const render::CommandName& name, core::data_view data) {
                if (name.identifier == render::data::Clear::command_name.identifier)
                {
                    IS_ASSERT(data.size() == sizeof(render::data::Clear), "Data size does not match expected type size.");

                    [[maybe_unused]] auto* clear_data = reinterpret_cast<const render::data::Clear*>(data.data());
                }
            });
        }

        ~VulkanRenderSystem() noexcept override
        {
            shutdown();

            SDL_DestroyWindow(_render_window);
        }

    private:
        // Allocator for driver allocations.
        core::memory::proxy_allocator _driver_allocator;

        // Special allocator for vulkan render system.
        render::vulkan::VulkanAllocator _vulkan_allocator;

        SDL_Window* _render_window;

        render::RenderCommandBuffer _command_buffer;

        // The Vulkan instance handle.
        VkInstance _vulkan_instance{};

        // Array vulkan devices.
        core::pod::Array<render::vulkan::VulkanPhysicalDevice*> _vulkan_devices;
    };

} // namespace render

extern "C"
{
    __declspec(dllexport) auto create_render_system(core::allocator& alloc) -> render::RenderSystem*
    {
        return alloc.make<render::VulkanRenderSystem>(alloc);
    }

    __declspec(dllexport) void release_render_system(core::allocator& alloc, render::RenderSystem* driver)
    {
        alloc.destroy(driver);
    }
}
