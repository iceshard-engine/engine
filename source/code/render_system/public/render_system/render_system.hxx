#pragma once
#include <render_system/render_command_buffer.hxx>
#include <render_system/render_vertex_descriptor.hxx>
#include <render_system/render_pipeline.hxx>

namespace render
{

    //! \brief Render context interface.
    class RenderContext;

    //! \brief A render system interface.
    class RenderSystem
    {
    public:
        virtual ~RenderSystem() noexcept = default;

        virtual auto command_buffer() noexcept -> CommandBuffer = 0;

        //virtual auto current_frame_buffer() noexcept -> FrameBufferHandle = 0;


        virtual void add_named_descriptor_set(
            core::cexpr::stringid_argument_type name,
            VertexBinding const& binding,
            VertexDescriptor const* descriptors,
            uint32_t descriptor_count) noexcept = 0;

        template<uint32_t Size>
        void add_named_descriptor_set(
            VertexDescriptorSet<Size> const& binding_set) noexcept;


        // clang-format off
        virtual auto create_pipeline(
            core::cexpr::stringid_type* descriptor_names,
            uint32_t descriptor_name_count
        ) noexcept -> api::RenderPipeline = 0;

        template<uint32_t DescriptorCount>
        auto create_pipeline(Pipeline<DescriptorCount> pipeline) noexcept -> api::RenderPipeline;
        // clang-format on

        virtual void swap() noexcept = 0;
    };

    template<uint32_t Size>
    inline void RenderSystem::add_named_descriptor_set(
        VertexDescriptorSet<Size> const& binding_set) noexcept
    {
        add_named_descriptor_set(binding_set.name, binding_set.binding, binding_set.descriptors, Size);
    }

    template<uint32_t DescriptorCount>
    inline auto RenderSystem::create_pipeline(Pipeline<DescriptorCount> pipeline) noexcept -> api::RenderPipeline
    {
        return create_pipeline(pipeline.descriptors.descriptors, DescriptorCount);
    }

} // namespace render
