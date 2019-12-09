#pragma once
#include <core/cexpr/stringid.hxx>
#include <render_system/render_vertex_descriptor.hxx>

namespace render
{

    template<uint32_t Size>
    struct PipelineVertexDescriptors
    {
        static_assert(Size >= 1, "At least one descriptor name needs to be used.");

        core::cexpr::stringid_type descriptors[Size];
    };

    template<uint32_t DescriptorCount>
    struct Pipeline
    {
        PipelineVertexDescriptors<DescriptorCount> descriptors;
    };

    namespace pipeline
    {

        // clang-format off
        static constexpr auto DefaultPieline = Pipeline<1>
        {
            .descriptors = { render::descriptor_set::Color.name }
        };
        // clang-format on

    } // namespace pipeline

} // namespace render
