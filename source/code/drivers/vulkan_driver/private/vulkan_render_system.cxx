#include <core/memory.hxx>
#include <core/pointer.hxx>
#include <core/pod/array.hxx>
#include <core/pod/hash.hxx>
#include <core/message/buffer.hxx>
#include <core/message/operations.hxx>
#include <core/allocators/proxy_allocator.hxx>

#include <render_system/render_system.hxx>
#include <render_system/render_commands.hxx>
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>

#include "api/v1/vulkan_render_api.hxx"

#include "vulkan_allocator.hxx"
#include "vulkan_device_memory_manager.hxx"
#include "vulkan_image.hxx"
#include "vulkan_buffer.hxx"

#include <iceshard/renderer/render_pass.hxx>
#include <iceshard/renderer/vulkan/vulkan_system.hxx>

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace render
{

    class VulkanRenderSystem : public render::RenderSystem
    {
    public:
        VulkanRenderSystem(core::allocator& alloc) noexcept
            : render::RenderSystem{}
            , _driver_allocator{ "vulkan-driver", alloc }
            , _vulkan_allocator{ alloc }
            , _vulkan_buffers{ _driver_allocator }
            , _vulkan_images{ _driver_allocator }
        {
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
            instance_create_info.enabledExtensionCount = static_cast<uint32_t>(std::size(instanceExtensionNames));
            instance_create_info.ppEnabledExtensionNames = &instanceExtensionNames[0];

            auto vk_create_result = vkCreateInstance(&instance_create_info, _vulkan_allocator.vulkan_callbacks(), &_vulkan_instance);
            IS_ASSERT(vk_create_result == VkResult::VK_SUCCESS, "Creation of Vulkan instance failed!");

            _vk_render_system = iceshard::renderer::vulkan::create_render_system(_driver_allocator, _vulkan_instance);

            _vulkan_device_memory = core::memory::make_unique<iceshard::renderer::vulkan::VulkanDeviceMemoryManager>(
                _driver_allocator,
                _driver_allocator,
                _vk_render_system->devices()
            );

            _vulkan_staging_buffer = iceshard::renderer::vulkan::create_staging_buffer(_driver_allocator, *_vulkan_device_memory);


        }

        void begin_frame() noexcept override
        {
            _vk_render_system->begin_frame();
        }

        void shutdown() noexcept
        {
            _vulkan_images.clear();

            _vulkan_staging_buffer = nullptr;
            _vulkan_buffers.clear();

            _vulkan_device_memory = nullptr;

            iceshard::renderer::vulkan::destroy_render_system(_driver_allocator, _vk_render_system);

            // We need to provide the callbacks when destroying the instance.
            vkDestroyInstance(_vulkan_instance, _vulkan_allocator.vulkan_callbacks());
        }

        auto create_buffer(iceshard::renderer::api::v1_1::BufferType type, uint32_t size) noexcept -> iceshard::renderer::api::v1_1::Buffer override
        {
            auto vulkan_buffer = iceshard::renderer::vulkan::create_buffer(
                _driver_allocator,
                type,
                size,
                *_vulkan_device_memory
            );

            auto result = iceshard::renderer::api::v1_1::Buffer{ reinterpret_cast<uintptr_t>(vulkan_buffer.get()) };
            _vulkan_buffers.emplace_back(std::move(vulkan_buffer));

            return result;
        }

        auto create_data_buffer(
            iceshard::renderer::api::BufferType type,
            uint32_t size
        ) noexcept -> iceshard::renderer::api::Buffer override
        {
            return _vk_render_system->create_data_buffer(type, size);
        }

        auto create_resource_set(
            core::stringid_arg_type name,
            iceshard::renderer::RenderPipelineLayout layout,
            core::pod::Array<iceshard::renderer::RenderResource> const& resources
        ) noexcept -> iceshard::renderer::ResourceSet override
        {
            return _vk_render_system->create_resource_set(name, layout, resources);
        }

        auto get_resource_set(
            core::stringid_arg_type name
        ) noexcept -> iceshard::renderer::ResourceSet override
        {
            return _vk_render_system->get_resource_set(name);
        }

        void update_resource_set(
            core::stringid_arg_type name,
            core::pod::Array<iceshard::renderer::RenderResource> const& resources
        ) noexcept override
        {
            return _vk_render_system->update_resource_set(name, resources);
        }

        void destroy_resource_set(
            core::stringid_arg_type name
        ) noexcept override
        {
            return _vk_render_system->destroy_resource_set(name);
        }

        auto create_pipeline(
            core::stringid_arg_type name,
            iceshard::renderer::RenderPipelineLayout layout,
            core::pod::Array<asset::AssetData> const& shader_assets
        ) noexcept -> iceshard::renderer::Pipeline override
        {
            return _vk_render_system->create_pipeline(name, layout, shader_assets);
        }

        void destroy_pipeline(
            core::stringid_arg_type name
        ) noexcept override
        {
            return _vk_render_system->destroy_pipeline(name);
        }

        auto load_texture(asset::AssetData texture_data) noexcept -> iceshard::renderer::api::v1_1::Texture override
        {
            int32_t width = resource::get_meta_int32(texture_data.metadata, "texture.extents.width"_sid);
            int32_t height = resource::get_meta_int32(texture_data.metadata, "texture.extents.height"_sid);

            iceshard::renderer::api::v1_1::DataView data_view;
            _vulkan_staging_buffer->map_memory(data_view);
            std::memcpy(data_view.data, texture_data.content._data, texture_data.content._size);
            _vulkan_staging_buffer->unmap_memory();

            VkExtent2D image_extent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

            auto texture = iceshard::renderer::vulkan::create_texture_2d(
                _driver_allocator,
                *_vulkan_device_memory,
                image_extent
            );

            VkCommandBuffer staging_cmds = _vk_render_system->v1_transfer_cmd_buffer();

            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(staging_cmds, &beginInfo);

            VkImageMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = texture->native_handle();
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = 0; // TODO
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // TODO

            vkCmdPipelineBarrier(
                staging_cmds,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            VkBufferImageCopy region = {};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = { 0, 0, 0 };
            region.imageExtent.width = image_extent.width;
            region.imageExtent.height = image_extent.height;
            region.imageExtent.depth = 1;
            vkCmdCopyBufferToImage(
                staging_cmds,
                _vulkan_staging_buffer->native_handle(),
                texture->native_handle(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region
            );


            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(
                staging_cmds,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            vkEndCommandBuffer(staging_cmds);
            _staging_cmds = true;

            auto result_handle = iceshard::renderer::api::v1_1::Texture{ reinterpret_cast<uintptr_t>(texture->native_view()) };
            _vulkan_images.emplace_back(std::move(texture));

            return result_handle;
        }

        auto acquire_command_buffer(iceshard::renderer::RenderPassStage stage) noexcept -> iceshard::renderer::CommandBuffer override
        {
            return _vk_render_system->acquire_command_buffer(stage);
        }

        void submit_command_buffer(iceshard::renderer::CommandBuffer cb) noexcept override
        {
            _vk_render_system->submit_command_buffer(cb);
        }

        void end_frame() noexcept override
        {
            _vk_render_system->end_frame();

            auto graphics_queue = _vk_render_system->v1_graphics_queue();

            if (_staging_cmds)
            {
                _staging_cmds = false;
                const VkCommandBuffer cmd_bufs[] = { _vk_render_system->v1_transfer_cmd_buffer() };

                VkSubmitInfo submit_info[1] = {};
                submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submit_info[0].commandBufferCount = 1;
                submit_info[0].pCommandBuffers = cmd_bufs;

                vkQueueSubmit(graphics_queue, 1, submit_info, VK_NULL_HANDLE);
                vkQueueWaitIdle(graphics_queue);
            }


            _vk_render_system->v1_submit_command_buffers();

            _vk_render_system->v1_present();
        }

        void initialize_render_interface(iceshard::renderer::api::v1_1::RenderInterface** render_interface) noexcept
        {
            *render_interface = iceshard::renderer::api::v1_1::render_api_instance;
        }

        ~VulkanRenderSystem() noexcept override
        {
            shutdown();
        }

    private:
        // Allocator for driver allocations.
        core::memory::proxy_allocator _driver_allocator;

        // Special allocator for vulkan render system.
        vulkan::VulkanAllocator _vulkan_allocator;

        // The Vulkan instance handle.
        VkInstance _vulkan_instance{};

        // Data buffers
        core::memory::unique_pointer<iceshard::renderer::vulkan::VulkanBuffer> _vulkan_staging_buffer{ nullptr, { core::memory::globals::null_allocator() } };
        core::Vector<core::memory::unique_pointer<iceshard::renderer::vulkan::VulkanBuffer>> _vulkan_buffers;

        // Array vulkan devices.
        core::memory::unique_pointer<iceshard::renderer::vulkan::VulkanDeviceMemoryManager> _vulkan_device_memory{ nullptr, { core::memory::globals::null_allocator() } };

        // Shader stages
        core::Vector<core::memory::unique_pointer<iceshard::renderer::vulkan::VulkanImage>> _vulkan_images;

        bool _staging_cmds;

        ////////////////////////////////////////////////////////////////
        // New RenderSystem object
        iceshard::renderer::vulkan::VulkanRenderSystem* _vk_render_system = nullptr;
    };

} // namespace render

extern "C"
{
    __declspec(dllexport) auto create_render_system(
        core::allocator& alloc,
        core::cexpr::stringid_hash_type api_version,
        void* api_instance) -> render::RenderSystem*
    {
        render::RenderSystem* result = nullptr;
        if (api_version == iceshard::renderer::api::v1_1::version_name.hash_value)
        {
            iceshard::renderer::api::v1_1::vulkan::init_api(iceshard::renderer::api::v1_1::render_api_instance);
            iceshard::renderer::api::v1_1::vulkan::init_api(api_instance);
            result = alloc.make<render::VulkanRenderSystem>(alloc);
        }
        return result;
    }

    __declspec(dllexport) void release_render_system(
        core::allocator& alloc,
        render::RenderSystem* driver)
    {
        alloc.destroy(driver);
    }
}
