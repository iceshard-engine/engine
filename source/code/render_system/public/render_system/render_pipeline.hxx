#pragma once
#include <core/cexpr/stringid.hxx>
#include <render_system/render_vertex_descriptor.hxx>

namespace render
{

    template<uint32_t Size>
    struct PipelineVertexDescriptors
    {
        static_assert(Size >= 1, "At least one descriptor name needs to be used.");

        template<typename... Args>
        constexpr PipelineVertexDescriptors(Args&&... args) noexcept
            : descriptors{ std::forward<Args>(args).name... }
        {
            static_assert(sizeof...(args) == Size, "Number of provided argument count does not match!");
        }

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
        static constexpr auto DefaultPieline = Pipeline<2>
        {
            .descriptors = {
                render::descriptor_set::Color,
                render::descriptor_set::Model,
            }
        };
        // clang-format on

    } // namespace pipeline

} // namespace render
