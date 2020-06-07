#pragma once
#include <iceshard/renderer/vulkan/vulkan_texture.hxx>
#include "../vulkan_device_memory_manager.hxx"
#include "../vulkan_image.hxx"

namespace iceshard::renderer::vulkan
{

    VulkanTextureStorage::VulkanTextureStorage(
        core::allocator& alloc,
        VulkanDeviceMemoryManager& device_memory
    ) noexcept
        : _allocator{ alloc }
        , _device_memory{ device_memory }
        , _textures{ _allocator }
    {
    }

    VulkanTextureStorage::~VulkanTextureStorage() noexcept
    {
    }

    auto VulkanTextureStorage::allocate_texture(core::stringid_arg_type name, VkExtent2D extent) noexcept -> api::Texture
    {
        auto texture = create_texture_2d(_allocator, _device_memory, extent);
        auto handle = api::Texture{ reinterpret_cast<uintptr_t>(texture.get()) };

        _textures.emplace(
            name.hash_value,
            std::move(texture)
        );

        return handle;
    }

    void VulkanTextureStorage::release_texture(core::stringid_arg_type name) noexcept
    {
        if (auto texture_entry = _textures.find(name.hash_value); texture_entry != _textures.end())
        {
            core::memory::unique_pointer<VulkanImage>& tex = texture_entry->second;

            _device_memory.deallocate_memory(
                tex->native_handle(), tex->memory_info()
            );

            _textures.erase(texture_entry);
        }
    }

} // namespace iceshard::renderer::vulkan
