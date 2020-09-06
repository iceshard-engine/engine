#pragma once
#include <iceshard/renderer/vulkan/vulkan_texture.hxx>
#include "../vulkan_device_memory_manager.hxx"
#include "../vulkan_image.hxx"

namespace iceshard::renderer::vulkan
{

    namespace detail
    {

        auto format_to_vulkan_format(api::TextureFormat format) noexcept
        {
            switch (format)
            {
            case iceshard::renderer::api::v1_1::types::TextureFormat::UintRGBA:
                return VkFormat::VK_FORMAT_R8G8B8_UNORM;
            case iceshard::renderer::api::v1_1::types::TextureFormat::UnormRGBA:
                return VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
            case iceshard::renderer::api::v1_1::types::TextureFormat::UnormSRGB:
                return VkFormat::VK_FORMAT_R8G8B8_SRGB;
            default:
                return VkFormat::VK_FORMAT_UNDEFINED;
            }
        }

    } // namespace detail

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

    auto VulkanTextureStorage::allocate_texture(
        core::stringid_arg_type name,
        api::TextureFormat format,
        VkExtent2D extent
    ) noexcept -> api::Texture
    {
        auto texture = create_texture_2d(
            _allocator,
            _device_memory,
            detail::format_to_vulkan_format(format),
            extent
        );

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
