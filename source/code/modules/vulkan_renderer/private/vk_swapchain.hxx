/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_swapchain.hxx>
#include "vk_image.hxx"
#include "vk_include.hxx"

namespace ice::render::vk
{

    class VulkanSwapchain final : public ice::render::RenderSwapchain
    {
    public:
        VulkanSwapchain(
            VkSwapchainKHR vk_swapchain,
            VkFormat vk_format,
            VkExtent2D vk_extent,
            VkDevice vk_device
        ) noexcept;
        ~VulkanSwapchain() noexcept;

        auto handle() const noexcept -> VkSwapchainKHR;

        auto extent() const noexcept -> ice::vec2u override;

        auto image_format() const noexcept -> ice::render::ImageFormat override;

        auto image_count() const noexcept -> ice::u32 override;

        auto image(ice::u32 index) const noexcept -> ice::render::Image override;

        auto image_extent() const noexcept -> VkExtent2D;

        auto aquire_image() noexcept -> ice::u32 override;

        auto current_image_index() const noexcept -> ice::u32 override;

    private:
        VkSwapchainKHR _vk_swapchain;
        VkFormat _vk_format;
        VkExtent2D _vk_extent;
        VkDevice _vk_device;
        VkFence _vk_aquire_fence;

        ice::u32 _image_count;
        VulkanImage _vk_images[3];
        ice::u32 _current_image = 0;
    };

} // namespace ice::render::vk
