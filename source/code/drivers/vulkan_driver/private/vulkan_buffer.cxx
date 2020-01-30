#include "vulkan_buffer.hxx"
#include <core/debug/assert.hxx>

namespace iceshard::renderer::vulkan
{

    namespace detail
    {

        auto type_to_string(api::BufferType type) noexcept -> std::string_view
        {
            switch (type)
            {
            case api::BufferType::IndexBuffer:
                return "Index Buffer";
            case api::BufferType::VertexBuffer:
                return "Vertex Buffer";
            case api::BufferType::UniformBuffer:
                return "Uniform Buffer";
            }
            return "<invalid_buffer_type>";
        }

        auto type_to_usage(api::BufferType type) noexcept -> VkBufferUsageFlags
        {
            switch (type)
            {
            case api::BufferType::IndexBuffer:
                return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            case api::BufferType::VertexBuffer:
                return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            case api::BufferType::UniformBuffer:
                return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            }

            IS_ASSERT(false, "Unknown buffer type!");
            std::abort();
        }

    } // namespace detail

    VulkanBuffer::VulkanBuffer(VkBuffer buffer_handle, VulkanDeviceMemoryManager& memory_manager, VulkanMemoryInfo memory_info) noexcept
        : _buffer_handle{ buffer_handle }
        , _device_memory{ &memory_manager }
        , _memory_info{ std::move(memory_info) }
    {
    }

    VulkanBuffer::~VulkanBuffer() noexcept
    {
        _device_memory->deallocate_memory(_buffer_handle, _memory_info);
        vkDestroyBuffer(_device_memory->graphics_device(), _buffer_handle, nullptr);
    }

    void VulkanBuffer::map_memory(api::DataView& data_view) noexcept
    {
        data_view.size = _memory_info.memory_size;

        auto api_result = vkMapMemory(
            _device_memory->graphics_device(),
            _memory_info.memory_handle,
            _memory_info.memory_offset,
            _memory_info.memory_size,
            0 /* empty flags */,
            &data_view.data
        );
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't map memory!");
    }

    void VulkanBuffer::unmap_memory() noexcept
    {
        vkUnmapMemory(_device_memory->graphics_device(), _memory_info.memory_handle);
    }

    auto create_staging_buffer(core::allocator& alloc, VulkanDeviceMemoryManager& device_memory) noexcept -> core::memory::unique_pointer<VulkanBuffer>
    {
        auto graphics_device = device_memory.graphics_device();

        VkBufferCreateInfo buf_info = {};
        buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buf_info.pNext = nullptr;
        buf_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buf_info.size = 1920 * 1080 * 16; // rgba fullhd staging buffer
        buf_info.queueFamilyIndexCount = 0;
        buf_info.pQueueFamilyIndices = nullptr;
        buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buf_info.flags = 0;

        VkBuffer buffer_handle;
        auto api_result = vkCreateBuffer(graphics_device, &buf_info, nullptr, &buffer_handle);
        IS_ASSERT(api_result == VK_SUCCESS, "Couldn't create vulkan image staging buffer object.");

        VulkanMemoryInfo memory_info;
        auto const memory_allocation_success = device_memory.allocate_memory(
            buffer_handle,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            memory_info
        );
        IS_ASSERT(memory_allocation_success, "Couldn't allocate memory for image staging buffer!");

        return core::memory::make_unique<VulkanBuffer>(alloc, buffer_handle, device_memory, std::move(memory_info));
    }

    auto create_buffer(
        core::allocator& alloc,
        api::BufferType type,
        uint32_t buffer_size,
        VulkanDeviceMemoryManager& device_memory
    ) noexcept -> core::memory::unique_pointer<VulkanBuffer>
    {
        auto graphics_device = device_memory.graphics_device();

        VkBufferCreateInfo buf_info = {};
        buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buf_info.pNext = nullptr;
        buf_info.usage = detail::type_to_usage(type);
        buf_info.size = buffer_size;
        buf_info.queueFamilyIndexCount = 0;
        buf_info.pQueueFamilyIndices = nullptr;
        buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buf_info.flags = 0;

        VkBuffer buffer_handle;
        auto api_result = vkCreateBuffer(graphics_device, &buf_info, nullptr, &buffer_handle);
        IS_ASSERT(api_result == VK_SUCCESS, "Couldn't create vulkan {} object.", detail::type_to_string(type));

        VulkanMemoryInfo memory_info;
        auto const memory_allocation_success = device_memory.allocate_memory(
            buffer_handle,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            memory_info
        );
        IS_ASSERT(memory_allocation_success, "Couldn't allocate memory for {}!", detail::type_to_string(type));

        return core::memory::make_unique<VulkanBuffer>(alloc, buffer_handle, device_memory, std::move(memory_info));
    }

    auto create_uniform_buffer(
        core::allocator& alloc,
        VulkanDeviceMemoryManager& device_memory,
        uint32_t buffer_size
    ) noexcept -> core::memory::unique_pointer<VulkanBuffer>
    {
        return create_buffer(alloc, api::BufferType::UniformBuffer, buffer_size, device_memory);
    }

    auto create_vertex_buffer(
        core::allocator& alloc,
        VulkanDeviceMemoryManager& device_memory,
        uint32_t buffer_size
    ) noexcept -> core::memory::unique_pointer<VulkanBuffer>
    {
        return create_buffer(alloc, api::BufferType::VertexBuffer, buffer_size, device_memory);
    }

} // namespace iceshard::renderer::vulkan
