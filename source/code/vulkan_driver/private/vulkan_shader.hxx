#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <core/data/view.hxx>
#include <vulkan/vulkan.h>

namespace render::vulkan
{

    class VulkanShader
    {
    public:
        VulkanShader(VkDevice device, VkShaderStageFlagBits shader_stage, VkShaderModule shader_module) noexcept;
        ~VulkanShader() noexcept;

        auto stage() const noexcept -> VkShaderStageFlagBits { return _shader_stage; }

    private:
        VkDevice _device_handle;
        VkShaderStageFlagBits _shader_stage;
        VkShaderModule _native_handle;
    };

    auto create_shader(
        core::allocator& alloc,
        VkDevice device,
        VkShaderStageFlagBits shader_stage,
        core::data_view shader_data) noexcept -> core::memory::unique_pointer<VulkanShader>;

} // namespace render::vulkan
