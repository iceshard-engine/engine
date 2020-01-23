#include "vulkan_renderpass.hxx"
#include <core/allocators/stack_allocator.hxx>
#include <core/debug/assert.hxx>
#include <core/pod/array.hxx>

namespace iceshard::renderer::vulkan
{

    //namespace detail
    //{

    //    enum class VulkanAttachmentType
    //    {
    //        ColorTexture = 0x0,
    //        ColorPresent = 0x1,
    //        DepthStencil = 0x2,
    //    };

    //    struct VulkanAttachmentInfo
    //    {
    //        VkFormat format = VkFormat::VK_FORMAT_UNDEFINED;
    //        VkImageLayout image_layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    //        VkImageLayout result_layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    //        VkAttachmentStoreOp store_op = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
    //    };

    //    template<VulkanAttachmentType AttachmentType>
    //    inline constexpr auto attachment_info() noexcept -> VulkanAttachmentInfo;

    //    template<>
    //    inline constexpr auto attachment_info<VulkanAttachmentType::ColorPresent>() noexcept -> VulkanAttachmentInfo
    //    {
    //        return {
    //            .format = VK_FORMAT_R8G8B8A8_UNORM,
    //            .image_layout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    //            .result_layout = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    //            .store_op = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE
    //        };
    //    }

    //    template<>
    //    inline constexpr auto attachment_info<VulkanAttachmentType::ColorTexture>() noexcept -> VulkanAttachmentInfo
    //    {
    //        return {
    //            .format = VK_FORMAT_R8G8B8A8_UNORM,
    //            .image_layout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    //            .result_layout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    //            .store_op = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE
    //        };
    //    }

    //    template<>
    //    inline constexpr auto attachment_info<VulkanAttachmentType::DepthStencil>() noexcept -> VulkanAttachmentInfo
    //    {
    //        return {
    //            .format = VK_FORMAT_R8G8B8A8_UNORM,
    //            .image_layout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    //            .result_layout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    //            .store_op = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE
    //        };
    //    }

    //    inline auto attachment_info(VulkanAttachmentType type) noexcept -> VulkanAttachmentInfo
    //    {
    //        switch (type)
    //        {
    //        case iceshard::renderer::vulkan::detail::VulkanAttachmentType::ColorTexture:
    //            return attachment_info<VulkanAttachmentType::ColorTexture>();
    //        case iceshard::renderer::vulkan::detail::VulkanAttachmentType::ColorPresent:
    //            return attachment_info<VulkanAttachmentType::ColorPresent>();
    //        case iceshard::renderer::vulkan::detail::VulkanAttachmentType::DepthStencil:
    //            return attachment_info<VulkanAttachmentType::DepthStencil>();
    //        default:
    //            break;
    //        }
    //        IS_ASSERT(false, "Unknown attachment type!");
    //        std::abort();
    //    }

    //    auto create_attachment_description(VulkanAttachmentInfo const& info) noexcept
    //    {
    //        VkAttachmentDescription description{ };
    //        description.flags = 0;
    //        description.format = info.format;
    //        description.samples = VK_SAMPLE_COUNT_1_BIT;
    //        description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    //        description.storeOp = info.store_op;
    //        description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    //        description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    //        description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //        description.finalLayout = info.result_layout;
    //        return description;
    //    }

    //    void build_render_pass_attachments(
    //        core::pod::Array<VulkanAttachmentInfo> const& attachment_info,
    //        core::pod::Array<VkAttachmentDescription>& attachment_description
    //    ) noexcept
    //    {
    //        IS_ASSERT(
    //            core::pod::array::size(attachment_info) == core::pod::array::size(attachment_description),
    //            "Argument sizes do not match!"
    //        );

    //        for (uint32_t idx = 0; idx < core::pod::array::size(attachment_info); ++idx)
    //        {
    //            VulkanAttachmentInfo const& info = attachment_info[idx];
    //            VkAttachmentDescription& description = attachment_description[idx];

    //            description.flags = 0;
    //            description.format = info.format;
    //            description.samples = VK_SAMPLE_COUNT_1_BIT;
    //            description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    //            description.storeOp = info.store_op;
    //            description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    //            description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    //            description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //            description.finalLayout = info.result_layout;
    //        }
    //    }

    //    void build_render_pass_subpasses(
    //        core::pod::Array<VulkanSubpassInfo> const& subpass_info,
    //        core::pod::Array<VkSubpassDescription>& subpass_description,
    //        core::pod::Array<VkSubpassDependency>& subpass_dependency
    //    ) noexcept
    //    {
    //        IS_ASSERT(
    //            core::pod::array::size(subpass_info) == core::pod::array::size(subpass_description),
    //            "Not enough capacity for subpass descriptions!"
    //        );

    //        for (uint32_t idx = 0; idx < core::pod::array::size(subpass_info); ++idx)
    //        {
    //            VulkanSubpassInfo const& info = subpass_info[idx];
    //            VkSubpassDescription& description = subpass_description[idx];

    //            // Build subpass description
    //            description.flags = 0;
    //            description.inputAttachmentCount = 0;
    //            description.pInputAttachments = nullptr;
    //            description.colorAttachmentCount = info.color_attachment_count;
    //            description.pColorAttachments = info.color_attachments;
    //            description.pResolveAttachments = nullptr;
    //            description.pDepthStencilAttachment = &info.depthstencil_attachment;
    //            description.preserveAttachmentCount = 0;
    //            description.pPreserveAttachments = nullptr;
    //        }
    //    }

    //    auto create_simple_renderpass(core::pod::Array<VulkanAttachmentType> const& types) noexcept -> VkRenderPass
    //    {
    //        core::memory::stack_allocator_1024 temp_alloc;

    //        // Build attachment sub-passes.
    //        core::pod::Array<VkAttachmentDescription> attachment_description{ temp_alloc };
    //        core::pod::Array<VkAttachmentDescription> attachment_reference{ temp_alloc };
    //        core::pod::Array<VkSubpassDescription> subpass_description{ temp_alloc };
    //        core::pod::Array<VkSubpassDependency> subpass_dependency{ temp_alloc };

    //        uint32_t const attachment_count = core::pod::array::size(types);
    //        core::pod::array::reserve(attachment_description, attachment_count);
    //        core::pod::array::reserve(attachment_reference, attachment_count * 3);
    //        core::pod::array::reserve(subpass_description, attachment_count);
    //        core::pod::array::reserve(subpass_dependency, attachment_count);


    //        // Fill render-pass info
    //        VkRenderPassCreateInfo render_pass_info{ };
    //        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    //        render_pass_info.pNext = nullptr;
    //        render_pass_info.flags = 0;
    //        render_pass_info.attachmentCount = core::pod::array::size(attachment_description);
    //        render_pass_info.pAttachments = core::pod::array::begin(attachment_description);
    //        render_pass_info.pSubpasses

    //        VkRenderPass vk_render_pass = nullptr;
    //        auto api_result = vkCreateRenderPass(nullptr, &render_pass_info, nullptr, &vk_render_pass);
    //        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create render pass object!");

    //        return vk_render_pass;
    //    }

    //} // namespace detail

    VulkanRenderPass::VulkanRenderPass() noexcept
    {
    }

    VulkanRenderPass::~VulkanRenderPass() noexcept
    {
    }

    auto create_render_pass(core::allocator& alloc) noexcept -> core::memory::unique_pointer<VulkanRenderPass>
    {
        return core::memory::make_unique<VulkanRenderPass>(alloc);
    }

} // namespace iceshard::renderer::vulkan
