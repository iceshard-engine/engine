#pragma once
#include <ice/pod/array.hxx>
#include <ice/log_tag.hxx>
#include <ice/log.hxx>

#include <ice/render/render_pass.hxx>
#include <ice/render/render_image.hxx>
#include "vk_include.hxx"


namespace ice::render
{

    enum class ImageFormat : ice::u32;

    enum class ImageLayout : ice::u32;

    enum class AttachmentType : ice::u32;

    enum class AttachmentOperation : ice::u32;

    enum class PipelineStage : ice::u32;

    enum class AccessFlags : ice::u32;

} // namespace ice::render

namespace ice::render::vk
{

    constexpr ice::LogTagDefinition log_tag = ice::create_log_tag(ice::LogTag::Module, "Vulkan");

    template<typename T, typename Fn, typename... Args>
    bool enumerate_objects(ice::pod::Array<T>& objects_out, Fn&& fn, Args&&... args) noexcept;

    inline auto native_enum_value(ImageType type) noexcept -> VkImageType;

    inline auto native_enum_value(ImageFormat image_format) noexcept -> VkFormat;

    inline auto native_enum_value(ImageLayout attachment_type) noexcept -> VkImageLayout;

    inline void native_enum_value(AttachmentOperation attachment_op, VkAttachmentStoreOp& store_op) noexcept;

    inline void native_enum_value(AttachmentOperation attachment_op, VkAttachmentLoadOp& load_op) noexcept;

    inline auto native_enum_value(PipelineStage stage) noexcept -> VkPipelineStageFlags;

    inline auto native_enum_value(AccessFlags flags) noexcept -> VkAccessFlags;

    inline auto native_enum_flags(ImageUsageFlags flags) noexcept -> VkImageUsageFlags;


    template<typename T, typename Fn, typename... Args>
    bool enumerate_objects(ice::pod::Array<T>& objects_out, Fn&& fn, Args&&... args) noexcept
    {
        using result_type = decltype(fn(args..., nullptr, nullptr));

        if constexpr (std::is_same_v<result_type, void>)
        {
            ice::u32 obj_count = 0;
            fn(args..., &obj_count, nullptr);
            if (obj_count > 0)
            {
                ice::pod::array::resize(objects_out, obj_count);
                fn(args..., &obj_count, ice::pod::array::begin(objects_out));
            }
            return true;
        }
        else
        {
            ice::u32 obj_count = 0;
            VkResult result = fn(args..., &obj_count, nullptr);
            if (result == VkResult::VK_SUCCESS && obj_count > 0)
            {
                ice::pod::array::resize(objects_out, obj_count);

                result = fn(args..., &obj_count, ice::pod::array::begin(objects_out));
            }
            return result == VK_SUCCESS;
        }
    }

    inline auto native_enum_value(ImageType type) noexcept -> VkImageType
    {
        switch (type)
        {
        case ImageType::Image2D:
            return VK_IMAGE_TYPE_2D;
        }
    }

    inline auto native_enum_value(ImageFormat image_format) noexcept -> VkFormat
    {
        switch (image_format)
        {
        case ImageFormat::I32_RGBA:
            return VK_FORMAT_R8G8B8A8_SINT;
        case ImageFormat::UNORM_RGBA:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case ImageFormat::UNORM_BGRA:
            return VK_FORMAT_B8G8R8A8_UNORM;
        case ImageFormat::SRGB_RGBA:
            return VK_FORMAT_R8G8B8A8_SRGB;
        case ImageFormat::SRGB_BGRA:
            return VK_FORMAT_B8G8R8A8_SRGB;
        case ImageFormat::UNORM_D24_UINT_S8:
            return VK_FORMAT_D24_UNORM_S8_UINT;
        default:
            return VK_FORMAT_UNDEFINED;
        }
    }

    inline auto api_enum_value(VkFormat image_format) noexcept -> ImageFormat
    {
        switch (image_format)
        {
        case VK_FORMAT_R8G8B8A8_SINT:
            return ImageFormat::I32_RGBA;
        case VK_FORMAT_R8G8B8A8_UNORM:
            return ImageFormat::UNORM_RGBA;
        case VK_FORMAT_B8G8R8A8_UNORM:
            return ImageFormat::UNORM_BGRA;
        case VK_FORMAT_R8G8B8A8_SRGB:
            return ImageFormat::SRGB_RGBA;
        case VK_FORMAT_B8G8R8A8_SRGB:
            return ImageFormat::SRGB_BGRA;
        case VK_FORMAT_D24_UNORM_S8_UINT:
            return ImageFormat::UNORM_D24_UINT_S8;
        default:
            return ImageFormat::Invalid;
        }
    }

    inline auto native_enum_value(ImageLayout attachment_layout) noexcept -> VkImageLayout
    {
        switch (attachment_layout)
        {
        case ImageLayout::Color:
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case ImageLayout::Present:
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        case ImageLayout::ShaderReadOnly:
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case ImageLayout::DepthStencil:
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        default:
            return VK_IMAGE_LAYOUT_UNDEFINED;
        }
    }

    inline void native_enum_value(AttachmentOperation attachment_op, VkAttachmentStoreOp& store_op) noexcept
    {
        switch (attachment_op)
        {
        case AttachmentOperation::Store_Store:
            store_op = VK_ATTACHMENT_STORE_OP_STORE;
            break;
        case AttachmentOperation::Store_DontCare:
            store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            break;
        }
    }

    inline void native_enum_value(AttachmentOperation attachment_op, VkAttachmentLoadOp& load_op) noexcept
    {
        switch (attachment_op)
        {
        case AttachmentOperation::Load_Clear:
            load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
            break;
        case AttachmentOperation::Load_DontCare:
            load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            break;
        }
    }

    inline auto native_enum_value(PipelineStage stage) noexcept -> VkPipelineStageFlags
    {
        switch (stage)
        {
        case PipelineStage::ColorAttachmentOutput:
            return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case PipelineStage::FramentShader:
            return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        default:
            return VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;
        }
    }

    inline auto native_enum_value(AccessFlags flags) noexcept -> VkAccessFlags
    {
        switch (flags)
        {
        case AccessFlags::ColorAttachmentWrite:
            return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case AccessFlags::InputAttachmentRead:
            return VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        default:
            return VK_ACCESS_FLAG_BITS_MAX_ENUM;
        }
    }

    template<typename T>
    bool has_flag(T flags, T flag) noexcept
    {
        using ValueType = std::underlying_type_t<T>;
        ValueType const flags_value = static_cast<ValueType>(flags);
        ValueType const flag_value = static_cast<ValueType>(flag);
        return (flags_value & flag_value) == flag_value;
    }

    auto native_enum_flags(ImageUsageFlags flags) noexcept -> VkImageUsageFlags
    {
        VkImageUsageFlags usage_flags = 0;
        if (has_flag(flags, ImageUsageFlags::Sampled))
        {
            usage_flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if (has_flag(flags, ImageUsageFlags::ColorAttachment))
        {
            usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
        if (has_flag(flags, ImageUsageFlags::DepthStencilAttachment))
        {
            usage_flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        return usage_flags;
    }

} // namespace ice::render::vk

#define VK_LOG(severity, message, ...) \
    ICE_LOG(severity, ice::render::vk::log_tag, message, __VA_ARGS__)
