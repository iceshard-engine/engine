#include "vulkan_shader.hxx"

namespace render::vulkan
{

    VulkanShader::VulkanShader(VkDevice device, VkShaderStageFlagBits shader_stage, VkShaderModule shader_module) noexcept
        : _device_handle{ device }
        , _shader_stage{ shader_stage }
        , _native_handle{ shader_module }
    {
    }

    VulkanShader::~VulkanShader() noexcept
    {
        vkDestroyShaderModule(_device_handle, _native_handle, nullptr);
    }

    auto create_shared(
        core::allocator& alloc,
        VkDevice device,
        VkShaderStageFlagBits shader_stage,
        core::data_view shader_data) noexcept -> core::memory::unique_pointer<VulkanShader>
    {
        VkShaderModuleCreateInfo shader_module_info;
        shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_module_info.pNext = nullptr;
        shader_module_info.flags = 0;
        shader_module_info.codeSize = shader_data.size();
        shader_module_info.pCode = reinterpret_cast<uint32_t const*>(shader_data.data());

        VkShaderModule shader_module;
        auto api_result = vkCreateShaderModule(device, &shader_module_info, nullptr, &shader_module);

        return core::memory::make_unique<VulkanShader>(alloc, device, shader_stage, shader_module);
    }

} // namespace render::vulkan
