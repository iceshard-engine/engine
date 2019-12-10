#include "vulkan_buffer.hxx"
#include <core/debug/assert.hxx>

namespace render::vulkan
{

    VulkanBuffer::VulkanBuffer(VkDevice device_handle, VkBuffer buffer_handle, VkDeviceMemory memory_handle) noexcept
        : _device_handle{ device_handle }
        , _buffer_handle{ buffer_handle }
        , _memory_handle{ memory_handle }
    {
    }

    VulkanBuffer::~VulkanBuffer() noexcept
    {
        vkDestroyBuffer(_device_handle, _buffer_handle, nullptr);
        vkFreeMemory(_device_handle, _memory_handle, nullptr);
    }

    void VulkanBuffer::map_memory(render::api::BufferDataView& data_view) noexcept
    {
        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(_device_handle, _buffer_handle, &mem_requirements);

        data_view.data_size = static_cast<uint32_t>(mem_requirements.size);

        auto api_result = vkMapMemory(_device_handle, _memory_handle, 0, data_view.data_size, 0, &data_view.data_pointer);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't map memory !");
    }

    void VulkanBuffer::unmap_memory() noexcept
    {
        vkUnmapMemory(_device_handle, _memory_handle);
    }

    auto create_uniform_buffer(core::allocator& alloc, VulkanPhysicalDevice* physical_device, VkDeviceSize buffer_size) noexcept -> core::memory::unique_pointer<VulkanBuffer>
    {
        auto surface_capabilities = physical_device->surface_capabilities();
        auto device_memory_properties = physical_device->memory_properties();
        auto graphics_device = physical_device->graphics_device()->native_handle();

        VkBufferCreateInfo buf_info = {};
        buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buf_info.pNext = nullptr;
        buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buf_info.size = buffer_size;
        buf_info.queueFamilyIndexCount = 0;
        buf_info.pQueueFamilyIndices = nullptr;
        buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buf_info.flags = 0;

        VkBuffer buffer_handle;
        auto api_result = vkCreateBuffer(graphics_device, &buf_info, nullptr, &buffer_handle);
        IS_ASSERT(api_result == VK_SUCCESS, "Couldn't create vulkan uniform buffer object.");

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(graphics_device, buffer_handle, &mem_requirements);

        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = 0;

        bool memory_type_found = physical_device->find_memory_type_index(
            mem_requirements,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            alloc_info.memoryTypeIndex);
        IS_ASSERT(memory_type_found, "No memory type found for depth image.");

        VkDeviceMemory buffer_memory_handle;
        api_result = vkAllocateMemory(graphics_device, &alloc_info, nullptr, &buffer_memory_handle);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't allocate memory for depth image!");

        api_result = vkBindBufferMemory(graphics_device, buffer_handle, buffer_memory_handle, 0);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't bind memory to buffer!");

        return core::memory::make_unique<VulkanBuffer>(alloc, graphics_device, buffer_handle, buffer_memory_handle);
    }

    auto create_vertex_buffer(core::allocator& alloc, VulkanPhysicalDevice* physical_device, VkDeviceSize buffer_size) noexcept -> core::memory::unique_pointer<VulkanBuffer>
    {
        auto surface_capabilities = physical_device->surface_capabilities();
        auto device_memory_properties = physical_device->memory_properties();
        auto graphics_device = physical_device->graphics_device()->native_handle();

        VkBufferCreateInfo buf_info = {};
        buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buf_info.pNext = nullptr;
        buf_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        buf_info.size = buffer_size;
        buf_info.queueFamilyIndexCount = 0;
        buf_info.pQueueFamilyIndices = nullptr;
        buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buf_info.flags = 0;

        VkBuffer buffer_handle;
        auto api_result = vkCreateBuffer(graphics_device, &buf_info, nullptr, &buffer_handle);
        IS_ASSERT(api_result == VK_SUCCESS, "Couldn't create vulkan uniform buffer object.");

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(graphics_device, buffer_handle, &mem_requirements);

        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = 0;

        bool memory_type_found = physical_device->find_memory_type_index(
            mem_requirements,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            alloc_info.memoryTypeIndex);
        IS_ASSERT(memory_type_found, "No memory type found for depth image.");

        VkDeviceMemory buffer_memory_handle;
        api_result = vkAllocateMemory(graphics_device, &alloc_info, nullptr, &buffer_memory_handle);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't allocate memory for depth image!");

        api_result = vkBindBufferMemory(graphics_device, buffer_handle, buffer_memory_handle, 0);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't bind memory to buffer!");

        return core::memory::make_unique<VulkanBuffer>(alloc, graphics_device, buffer_handle, buffer_memory_handle);
    }

} // namespace render::vulkan
