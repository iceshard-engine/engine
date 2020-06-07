#pragma once
#include <core/allocator.hxx>
#include <core/collections.hxx>
#include <core/pointer.hxx>
#include <iceshard/renderer/render_api.hxx>
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>

namespace iceshard::renderer::vulkan
{

    class VulkanImage;

    class VulkanDeviceMemoryManager;

    class VulkanTextureStorage
    {
    public:
        VulkanTextureStorage(
            core::allocator& alloc,
            VulkanDeviceMemoryManager& device_memory
        ) noexcept;
        ~VulkanTextureStorage() noexcept;

        auto allocate_texture(core::stringid_arg_type name, VkExtent2D size) noexcept -> api::Texture;
        void release_texture(core::stringid_arg_type name) noexcept;

    private:
        core::allocator& _allocator;
        VulkanDeviceMemoryManager& _device_memory;

        core::Map<core::stringid_hash_type, core::memory::unique_pointer<VulkanImage>> _textures;
    };

} // namespace iceshard::renderer::vulkan
