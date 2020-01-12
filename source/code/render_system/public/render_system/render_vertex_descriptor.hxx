#pragma once
#include <core/cexpr/stringid.hxx>

namespace render
{

    enum class VertexBindingRate : uint32_t
    {
        PerVertex = 0x01,
        PerInstance = 0x02,
    };

    enum class VertexBindingStride : uint32_t
    {
        Automatic = 0x0,
    };

    enum class VertexBindingLocation : uint32_t
    {
        Automatic = std::numeric_limits<std::underlying_type_t<VertexBindingLocation>>::max(),
    };

    struct VertexBinding
    {
        VertexBindingRate binding_rate;
        VertexBindingStride binding_stride = VertexBindingStride::Automatic;
        VertexBindingLocation binding_location = VertexBindingLocation::Automatic;
    };

    enum class VertexDescriptorType : uint32_t
    {
        Float,
        FloatVec2,
        FloatVec3,
        FloatVec4,
    };

    enum class VertexDescriptorOffset : uint32_t
    {
        Automatic = 0x0,
    };

    enum class VertexDescriptorLocation : uint32_t
    {
        Automatic = std::numeric_limits<std::underlying_type_t<VertexDescriptorLocation>>::max(),
    };

    struct VertexDescriptor
    {
        VertexDescriptorType descriptor_type;
        VertexDescriptorOffset descriptor_offset = VertexDescriptorOffset::Automatic;
        VertexDescriptorLocation descriptor_location = VertexDescriptorLocation::Automatic;
    };

    template<uint32_t Size>
    struct VertexDescriptorSet
    {
        static_assert(Size >= 1, "At least one descriptor needs to be defined.");

        core::cexpr::stringid_type name;
        VertexBinding binding;
        VertexDescriptor descriptors[Size];
    };

    namespace descriptor_set
    {

        static constexpr auto Color = VertexDescriptorSet<2>{
            .name = core::cexpr::stringid_cexpr("Color"),
            .binding = VertexBinding{
                .binding_rate = VertexBindingRate::PerVertex,
            },
            .descriptors = {
                VertexDescriptor{ // position xyzw
                    .descriptor_type = VertexDescriptorType::FloatVec4,
                },
                VertexDescriptor{ // color rgba
                    .descriptor_type = VertexDescriptorType::FloatVec4,
                },
            }
        };

        static constexpr auto Model = VertexDescriptorSet<4>{
            .name = core::cexpr::stringid_cexpr("Model"),
            .binding = VertexBinding{
                .binding_rate = VertexBindingRate::PerInstance,
            },
            .descriptors = {
                VertexDescriptor{
                    .descriptor_type = VertexDescriptorType::FloatVec4,
                },
                VertexDescriptor{
                    .descriptor_type = VertexDescriptorType::FloatVec4,
                },
                VertexDescriptor{
                    .descriptor_type = VertexDescriptorType::FloatVec4,
                },
                VertexDescriptor{
                    .descriptor_type = VertexDescriptorType::FloatVec4,
                },
            }
        };

    } // namespace descriptor_set

} // namespace render
