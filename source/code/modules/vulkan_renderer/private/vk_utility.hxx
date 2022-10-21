#pragma once
#include <ice/container/array.hxx>
#include <ice/log_tag.hxx>
#include <ice/log.hxx>

#include <ice/render/render_pass.hxx>
#include <ice/render/render_image.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_pipeline.hxx>
#include <ice/render/render_buffer.hxx>

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
    bool enumerate_objects(ice::Array<T>& objects_out, Fn&& fn, Args&&... args) noexcept;

    inline auto native_enum_value(ImageType type) noexcept -> VkImageType;

    inline auto native_enum_value(ImageFormat image_format) noexcept -> VkFormat;

    inline auto native_enum_value(ImageLayout attachment_type) noexcept -> VkImageLayout;

    inline void native_enum_value(AttachmentOperation attachment_op, VkAttachmentStoreOp& store_op) noexcept;

    inline void native_enum_value(AttachmentOperation attachment_op, VkAttachmentLoadOp& load_op) noexcept;

    inline auto native_enum_value(PipelineStage stage) noexcept -> VkPipelineStageFlags;

    inline auto native_enum_value(AccessFlags flags) noexcept -> VkAccessFlags;

    inline auto native_enum_value(ResourceType type) noexcept -> VkDescriptorType;

    inline auto native_enum_value(BufferType type) noexcept -> VkBufferUsageFlags;

    inline auto native_enum_value(ShaderStageFlags flags) noexcept -> VkShaderStageFlagBits;

    inline auto native_enum_value(ShaderAttribType type) noexcept -> VkFormat;

    inline auto native_enum_value(SamplerFilter filter) noexcept -> VkFilter;

    inline auto native_enum_value(PrimitiveTopology topology) noexcept -> VkPrimitiveTopology;

    inline auto native_enum_value(SamplerAddressMode address_mode) noexcept -> VkSamplerAddressMode;

    inline auto native_enum_value(SamplerMipMapMode mipmap_mode) noexcept -> VkSamplerMipmapMode;

    inline auto native_enum_flags(ImageUsageFlags flags) noexcept -> VkImageUsageFlags;

    inline auto native_enum_flags(ShaderStageFlags flags) noexcept -> VkShaderStageFlags;


    template<typename T, typename Fn, typename... Args>
    bool enumerate_objects(ice::Array<T>& objects_out, Fn&& fn, Args&&... args) noexcept
    {
        using result_type = decltype(fn(args..., nullptr, nullptr));

        if constexpr (std::is_same_v<result_type, void>)
        {
            ice::u32 obj_count = 0;
            fn(args..., &obj_count, nullptr);
            if (obj_count > 0)
            {
                ice::array::resize(objects_out, obj_count);
                fn(args..., &obj_count, ice::array::begin(objects_out));
            }
            return true;
        }
        else
        {
            ice::u32 obj_count = 0;
            VkResult result = fn(args..., &obj_count, nullptr);
            if (result == VkResult::VK_SUCCESS && obj_count > 0)
            {
                ice::array::resize(objects_out, obj_count);

                result = fn(args..., &obj_count, ice::array::begin(objects_out));
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
        case ImageFormat::SFLOAT_D32_UINT_S8:
            return VK_FORMAT_D32_SFLOAT_S8_UINT;
        case ImageFormat::SFLOAT_D32:
            return VK_FORMAT_D32_SFLOAT;
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
        case ImageLayout::TransferDstOptimal:
            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case ImageLayout::Undefined:
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
        case PipelineStage::TopOfPipe:
            return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        case PipelineStage::Transfer:
            return VK_PIPELINE_STAGE_TRANSFER_BIT;
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
        case AccessFlags::None:
            return VK_ACCESS_NONE_KHR;
        case AccessFlags::ShaderRead:
            return VK_ACCESS_SHADER_READ_BIT;
        case AccessFlags::InputAttachmentRead:
            return VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        case AccessFlags::TransferWrite:
            return VK_ACCESS_TRANSFER_WRITE_BIT;
        case AccessFlags::ColorAttachmentWrite:
            return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        default:
            return VK_ACCESS_FLAG_BITS_MAX_ENUM;
        }
    }

    auto native_enum_value(ResourceType type) noexcept -> VkDescriptorType
    {
        switch (type)
        {
        case ResourceType::Sampler:
        case ResourceType::SamplerImmutable:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
        case ResourceType::SampledImage:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case ResourceType::CombinedImageSampler:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case ResourceType::UniformBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case ResourceType::InputAttachment:
            return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        default:
            return VK_DESCRIPTOR_TYPE_MAX_ENUM;
        }
    }

    auto native_enum_value(ice::render::BufferType type) noexcept -> VkBufferUsageFlags
    {
        switch (type)
        {
        case BufferType::Index:
            return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        case BufferType::Vertex:
            return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        case BufferType::Uniform:
            return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        case BufferType::Transfer:
            return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        default:
            return VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
        }
    }

    auto native_enum_value(ShaderStageFlags value) noexcept -> VkShaderStageFlagBits
    {
        switch (value)
        {
        case ShaderStageFlags::VertexStage:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderStageFlags::FragmentStage:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderStageFlags::GeometryStage:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case ShaderStageFlags::TesselationControlStage:
            return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case ShaderStageFlags::TesselationEvaluationStage:
            return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        default:
            return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
        }
    }

    auto native_enum_value(ShaderAttribType type) noexcept -> VkFormat
    {
        switch (type)
        {
        case ShaderAttribType::Vec1f:
            return VK_FORMAT_R32_SFLOAT;
        case ShaderAttribType::Vec2f:
            return VK_FORMAT_R32G32_SFLOAT;
        case ShaderAttribType::Vec3f:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case ShaderAttribType::Vec4f:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case ShaderAttribType::Vec4f_Unorm8:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case ShaderAttribType::Vec1u:
            return VK_FORMAT_R32_UINT;
        case ShaderAttribType::Vec1i:
            return VK_FORMAT_R32_SINT;
        default:
            return VK_FORMAT_MAX_ENUM;
        }
    }

    auto native_enum_value(SamplerFilter filter) noexcept -> VkFilter
    {
        switch (filter)
        {
        case SamplerFilter::Nearest:
            return VkFilter::VK_FILTER_NEAREST;
        case SamplerFilter::Linear:
            return VkFilter::VK_FILTER_LINEAR;
        case SamplerFilter::CubicImg:
            return VkFilter::VK_FILTER_CUBIC_IMG;
        case SamplerFilter::CubicExt:
            return VkFilter::VK_FILTER_CUBIC_EXT;
        default:
            return VkFilter::VK_FILTER_MAX_ENUM;
        }
    }

    auto native_enum_value(PrimitiveTopology topology) noexcept -> VkPrimitiveTopology
    {
        switch (topology)
        {
        case PrimitiveTopology::LineStrip:
            return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case PrimitiveTopology::LineStripWithAdjency:
            return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
        case PrimitiveTopology::TriangleList:
            return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case PrimitiveTopology::TriangleStrip:
            return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case PrimitiveTopology::TriangleFan:
            return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
        case PrimitiveTopology::PatchList:
            return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
        default:
            return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
        }
    }

    auto native_enum_value(SamplerAddressMode address_mode) noexcept -> VkSamplerAddressMode
    {
        switch (address_mode)
        {
        case SamplerAddressMode::Repeat:
            return VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case SamplerAddressMode::RepeatMirrored:
            return VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case SamplerAddressMode::ClampToBorder:
            return VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case SamplerAddressMode::ClampToEdge:
            return VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case SamplerAddressMode::ClampToEdgeMirrored:
            return VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
        default:
            return VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
        }
    }

    auto native_enum_value(SamplerMipMapMode mipmap_mode) noexcept -> VkSamplerMipmapMode
    {
        switch (mipmap_mode)
        {
        case SamplerMipMapMode::Nearest:
            return VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case SamplerMipMapMode::Linear:
            return VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR;
        default:
            return VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_MAX_ENUM;
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
        if (has_flag(flags, ImageUsageFlags::TransferDst))
        {
            usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if (has_flag(flags, ImageUsageFlags::ColorAttachment))
        {
            usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
        if (has_flag(flags, ImageUsageFlags::InputAttachment))
        {
            usage_flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        }
        if (has_flag(flags, ImageUsageFlags::DepthStencilAttachment))
        {
            usage_flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        return usage_flags;
    }

    auto native_enum_flags(ShaderStageFlags flags) noexcept -> VkShaderStageFlags
    {
        VkShaderStageFlags usage_flags = 0;
        if (has_flag(flags, ShaderStageFlags::VertexStage))
        {
            usage_flags |= VK_SHADER_STAGE_VERTEX_BIT;
        }
        if (has_flag(flags, ShaderStageFlags::FragmentStage))
        {
            usage_flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        if (has_flag(flags, ShaderStageFlags::GeometryStage))
        {
            usage_flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
        }
        if (has_flag(flags, ShaderStageFlags::TesselationControlStage))
        {
            usage_flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        }
        if (has_flag(flags, ShaderStageFlags::TesselationEvaluationStage))
        {
            usage_flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        }
        return usage_flags;
    }

} // namespace ice::render::vk

#define VK_LOG(severity, message, ...) \
    ICE_LOG(severity, ice::render::vk::log_tag, message, __VA_ARGS__)
