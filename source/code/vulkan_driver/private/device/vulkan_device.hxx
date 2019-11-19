#pragma once
#include <core/allocator.hxx>
#include <core/pod/array.hxx>
#include <vulkan/vulkan.h>

#include "vulkan_command_buffer.hxx"

namespace render::vulkan
{

    //! \brief Defines available vulkan device queues.
    enum class VulkanDeviceQueueType
    {
        GraphicsQueue,
        ComputeQueue,
        TransferQueue,
        SparseQueue
    };

    //! \brief Vulkand queue family index.
    enum class VulkanQueueFamilyIndex : uint32_t;

    //! \brief Representas a logical vulkan device created to handle a specific queue.
    class VulkanDevice final
    {
    public:
        VulkanDevice(
            core::allocator& alloc,
            VulkanDeviceQueueType queue_type,
            VulkanQueueFamilyIndex queue_family,
            VkDevice device_handle) noexcept;

        ~VulkanDevice() noexcept;

        //! \brief Creates the specified number of command buffers.
        void create_command_buffers(core::pod::Array<VulkanCommandBuffer*>& output_array, uint32_t num) noexcept;

    protected:
        void initialize() noexcept;
        void shutdown() noexcept;

    private:
        core::allocator& _allocator;
        VulkanDeviceQueueType const _queue_type;
        VulkanQueueFamilyIndex const _queue_family;

        VkDevice _device_handle;
        VkCommandPool _command_pool;

        core::pod::Array<VulkanCommandBuffer*> _command_buffers;
    };

} // namespace render::vulkan
