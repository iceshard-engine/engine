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

    struct VertexBinding
    {
        uint32_t binding_location;
        VertexBindingRate binding_rate;
        VertexBindingStride binding_stride = VertexBindingStride::Automatic;
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

    struct VertexDescriptor
    {
        uint32_t descriptor_location;
        VertexDescriptorType descriptor_type;
        VertexDescriptorOffset descriptor_offset = VertexDescriptorOffset::Automatic;
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

        // clang-format off
        static constexpr auto Color = VertexDescriptorSet<2>{
            .name = { core::cexpr::stringid_cexpr("Color").hash_value, "Color" },
            .binding = VertexBinding{
                .binding_location = 0,
                .binding_rate = VertexBindingRate::PerVertex,
            },
            .descriptors = {
                VertexDescriptor{ // position xyzw
                    .descriptor_location = 0,
                    .descriptor_type = VertexDescriptorType::FloatVec4,
                },
                VertexDescriptor{ // color rgba
                    .descriptor_location = 1,
                    .descriptor_type = VertexDescriptorType::FloatVec4,
                },
            }
        };
        // clang-format on

    } // namespace descriptor_set

} // namespace render
