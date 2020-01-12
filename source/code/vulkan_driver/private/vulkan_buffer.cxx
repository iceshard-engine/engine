#include "vulkan_buffer.hxx"
#include <core/debug/assert.hxx>

namespace render::vulkan
{

    VulkanBuffer::VulkanBuffer(VkBuffer buffer_handle, VulkanDeviceMemoryManager& memory_manager, VulkanMemoryInfo memory_info) noexcept
        : _buffer_handle{ buffer_handle }
        , _device_memory{ &memory_manager }
        , _memory_info{ std::move(memory_info) }
    {
    }

    VulkanBuffer::~VulkanBuffer() noexcept
    {
        vkDestroyBuffer(_device_memory->graphics_device(), _buffer_handle, nullptr);
    }

    void VulkanBuffer::map_memory(render::api::BufferDataView& data_view) noexcept
    {
        data_view.data_size = _memory_info.memory_size;

        auto api_result = vkMapMemory(
            _device_memory->graphics_device(),
            _memory_info.memory_handle,
            _memory_info.memory_offset,
            _memory_info.memory_size,
            0 /* empty flags */,
            &data_view.data_pointer
        );
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't map memory!");
    }

    void VulkanBuffer::unmap_memory() noexcept
    {
        vkUnmapMemory(_device_memory->graphics_device(), _memory_info.memory_handle);
    }

    auto create_uniform_buffer(
        core::allocator& alloc,
        VulkanDeviceMemoryManager& device_memory,
        uint32_t buffer_size
    ) noexcept -> core::memory::unique_pointer<VulkanBuffer>
    {
        auto graphics_device = device_memory.graphics_device();

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

        VulkanMemoryInfo memory_info;
        auto const memory_allocation_success = device_memory.allocate_memory(buffer_handle, memory_info);
        IS_ASSERT(memory_allocation_success, "Couldn't allocate memory for uniform buffer!");

        return core::memory::make_unique<VulkanBuffer>(alloc, buffer_handle, device_memory, std::move(memory_info));
    }

    auto create_vertex_buffer(
        core::allocator& alloc,
        VulkanDeviceMemoryManager& device_memory,
        uint32_t buffer_size
    ) noexcept -> core::memory::unique_pointer<VulkanBuffer>
    {
        auto graphics_device = device_memory.graphics_device();

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
        IS_ASSERT(api_result == VK_SUCCESS, "Couldn't create vulkan vertex buffer object.");

        VulkanMemoryInfo memory_info;
        auto const memory_allocation_success = device_memory.allocate_memory(buffer_handle, memory_info);
        IS_ASSERT(memory_allocation_success, "Couldn't allocate memory for vertex buffer!");

        return core::memory::make_unique<VulkanBuffer>(alloc, buffer_handle, device_memory, std::move(memory_info));
    }

} // namespace render::vulkan
