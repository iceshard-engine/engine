/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "vk_swapchain.hxx"
#include "vk_utility.hxx"
#include <ice/assert.hxx>

namespace ice::render::vk
{

    VulkanSwapchain::VulkanSwapchain(
        VkSwapchainKHR vk_swapchain,
        VkFormat vk_format,
        VkExtent2D vk_extent,
        VkDevice vk_device
    ) noexcept
        : _vk_swapchain{ vk_swapchain }
        , _vk_format{ vk_format }
        , _vk_extent{ vk_extent }
        , _vk_device{ vk_device }
    {
        VkResult result = vkGetSwapchainImagesKHR(
            _vk_device,
            _vk_swapchain,
            &_image_count,
            nullptr
        );
        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Couldn't query swapchain image count!"
        );

        VkImage vk_images[3];
        result = vkGetSwapchainImagesKHR(
            _vk_device,
            _vk_swapchain,
            &_image_count,
            vk_images
        );
        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Couldn't query swapchain images!"
        );

        for (ice::u32 idx = 0; idx < _image_count; ++idx)
        {
            VkImageViewCreateInfo color_image_view = {};
            color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            color_image_view.pNext = NULL;
            color_image_view.flags = 0;
            color_image_view.image = vk_images[idx];
            color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
            color_image_view.format = _vk_format;
            color_image_view.components.r = VK_COMPONENT_SWIZZLE_R;
            color_image_view.components.g = VK_COMPONENT_SWIZZLE_G;
            color_image_view.components.b = VK_COMPONENT_SWIZZLE_B;
            color_image_view.components.a = VK_COMPONENT_SWIZZLE_A;
            color_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            color_image_view.subresourceRange.baseMipLevel = 0;
            color_image_view.subresourceRange.levelCount = 1;
            color_image_view.subresourceRange.baseArrayLayer = 0;
            color_image_view.subresourceRange.layerCount = 1;

            VkImageView vk_image_view;
            result = vkCreateImageView(_vk_device, &color_image_view, nullptr, &vk_image_view);
            ICE_ASSERT(
                result == VkResult::VK_SUCCESS,
                "Couldn't create view for swapchain image!"
            );

            _vk_images[idx].vk_image = vk_images[idx];
            _vk_images[idx].vk_image_view = vk_image_view;
        }


        VkFenceCreateInfo fence_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        vkCreateFence(_vk_device, &fence_info, nullptr, &_vk_aquire_fence);
    }

    VulkanSwapchain::~VulkanSwapchain() noexcept
    {
        vkDestroyFence(_vk_device, _vk_aquire_fence, nullptr);
        for (ice::u32 idx = 0; idx < _image_count; ++idx)
        {
            vkDestroyImageView(_vk_device, _vk_images[idx].vk_image_view, nullptr);
        }
        vkDestroySwapchainKHR(_vk_device, _vk_swapchain, nullptr);
    }

    auto VulkanSwapchain::handle() const noexcept -> VkSwapchainKHR
    {
        return _vk_swapchain;
    }

    auto VulkanSwapchain::extent() const noexcept -> ice::vec2u
    {
        return { _vk_extent.width, _vk_extent.height };
    }

    auto VulkanSwapchain::image_format() const noexcept -> ice::render::ImageFormat
    {
        return api_enum_value(_vk_format);
    }

    auto VulkanSwapchain::image_extent() const noexcept -> VkExtent2D
    {
        return _vk_extent;
    }

    auto VulkanSwapchain::aquire_image() noexcept -> ice::u32
    {
        //VkAcquireNextImageInfoKHR aquire_info{ VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR };
        //aquire_info.swapchain = _vk_swapchain;
        //aquire_info.deviceMask = 0;
        //aquire_info.timeout = UINT64_MAX;

        VkResult result = vkAcquireNextImageKHR(
            _vk_device,
            _vk_swapchain,
            UINT64_MAX,
            nullptr,
            _vk_aquire_fence,
            &_current_image
        );
        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Failed to aquire next image for rendering!"
        );

        // #todo remove this fence here!
        do
        {
            constexpr auto FENCE_TIMEOUT = 100'000'000; // in ns
            result = vkWaitForFences(_vk_device, 1, &_vk_aquire_fence, VK_TRUE, FENCE_TIMEOUT);
        } while (result == VK_TIMEOUT);

        vkResetFences(_vk_device, 1, &_vk_aquire_fence);

        return _current_image;
    }

    auto VulkanSwapchain::current_image_index() const noexcept -> ice::u32
    {
        return _current_image;
    }

    auto VulkanSwapchain::image_count() const noexcept -> ice::u32
    {
        return _image_count;
    }

    auto VulkanSwapchain::image(ice::u32 index) const noexcept -> ice::render::Image
    {
        ICE_ASSERT(
            index < _image_count,
            "Swapchain image acces: Index {} out of bounds [0, {}]!",
            index, _image_count
        );
        return static_cast<Image>(reinterpret_cast<ice::uptr>(_vk_images + index));
    }

} // namespace ice::render::vk
