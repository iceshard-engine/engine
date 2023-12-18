/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container/array.hxx>
#include <ice/gfx/gfx_object.hxx>
#include <ice/gfx/gfx_render_graph.hxx>
#include <ice/mem_allocator_proxy.hxx>

#include <ice/render/render_pass.hxx>

namespace ice::gfx::v2
{

    enum InternalHandleTypes : ice::u8
    {
        IHT_Invalid,
        IHT_Image,
        IHT_DepthStencil,
        IHT_Buffer,
    };

    using InternalHandleRef = GfxStageIO;

    static constexpr bool sort(GfxStageIO left, GfxStageIO right) noexcept
    {
        return left.type < right.type && left.idx < right.idx;
    }

    class SimpleGfxRenderGraph : public ice::gfx::v2::GfxRenderGraph
    {
    public:
        SimpleGfxRenderGraph(
            ice::Allocator& alloc,
            ice::Array<ice::gfx::v2::GfxStageDefinition>&& definitions,
            ice::ucount count_setup_stages
        ) noexcept;

        ~SimpleGfxRenderGraph() noexcept override = default;

        auto definitions() const noexcept -> ice::Span<ice::gfx::v2::GfxStageDefinition const> override;
        auto setup_stages() const noexcept -> ice::Span<ice::StringID const> override;
        auto runtime_stages() const noexcept -> ice::Span<ice::StringID const> override;

        auto create_runtime(
            ice::gfx::GfxDevice& device
        ) noexcept -> ice::UniquePtr<ice::gfx::v2::GfxRenderGraphRuntime>;

    protected:
        ice::ProxyAllocator _allocator;
        ice::Array<ice::gfx::v2::GfxStageDefinition> _graph_stage_definitions;
        ice::Array<ice::StringID> _graph_stage_names;
        ice::ucount const _count_setup_stages;
    };

    class BuilderGfxRenderGraph final : public SimpleGfxRenderGraph
    {
    public:
        BuilderGfxRenderGraph(
            ice::Allocator& alloc,
            ice::ucount count_setup_stages,
            ice::Array<ice::gfx::v2::GfxStageDefinition>&& definitions,
            ice::Memory definitions_data
        ) noexcept;

        ~BuilderGfxRenderGraph() noexcept override;

        auto create_runtime(
            ice::gfx::GfxDevice& device
        ) noexcept -> ice::UniquePtr<ice::gfx::v2::GfxRenderGraphRuntime>;

    private:
        ice::Memory _data;
    };

    struct SimpleGfxRenderStageBuilder : public ice::gfx::v2::GfxRenderStageBuilder
    {
        SimpleGfxRenderStageBuilder(
            ice::Allocator& alloc,
            ice::StringID_Arg name
        ) noexcept
            : _name{ name }
            , _inputs{ alloc }
            , _outputs{ alloc }
        {
        }

        auto builtin(GfxBuildInStage stage_type) noexcept -> GfxRenderStageBuilder& override
        {
            _builtin = stage_type;
            return *this;
        }

        auto depthstencil(GfxBuilderHandle handle) noexcept -> GfxRenderStageBuilder& override
        {
            ice::array::push_back(
                _inputs,
                InternalHandleRef{
                    .id = handle.id(),
                    .type = handle.type(),
                    .idx = 0
                }
            );
            return *this;
        }

        auto inputs(ice::Span<GfxBuilderHandle const> inputs, ice::u16 starting_idx) noexcept -> GfxRenderStageBuilder& override
        {
            ICE_ASSERT(GfxBuilderHandle::same_handles(inputs), "Only same type handles can be batched!");
            for (auto const handle : inputs)
            {
                ice::array::push_back(
                    _inputs,
                    InternalHandleRef{
                        .id = handle.id(),
                        .type = handle.type(),
                        .idx = starting_idx++
                    }
                );
            }
            return *this;
        }

        auto outputs(ice::Span<GfxBuilderHandle const> outputs, ice::u16 starting_idx) noexcept -> GfxRenderStageBuilder& override
        {
            ICE_ASSERT(GfxBuilderHandle::same_handles(outputs), "Only same type handles can be batched!");
            for (auto const handle : outputs)
            {
                ice::array::push_back(
                    _outputs,
                    InternalHandleRef{
                        .id = handle.id(),
                        .type = handle.type(),
                        .idx = starting_idx++
                    }
                );
            }
            return *this;
        };

        ice::StringID _name;
        GfxBuildInStage _builtin;
        ice::Array<InternalHandleRef> _inputs;
        ice::Array<InternalHandleRef> _outputs;
    };

    class SimpleGfxRenderPassBuilder : public ice::gfx::v2::GfxRenderPassBuilder
    {
    public:
        SimpleGfxRenderPassBuilder(
            ice::Allocator& alloc,
            ice::gfx::v2::GfxRenderGraphBuilder& graph_builder
        ) noexcept;

        auto clear_color(ice::vec4f color) noexcept -> GfxRenderPassBuilder& override { return *this; }
        auto stage(ice::StringID_Arg name) noexcept -> GfxRenderStageBuilder& override;

        auto stages() const noexcept -> ice::Span<ice::UniquePtr<SimpleGfxRenderStageBuilder> const> { return _stages; }
        //auto stage_count() const noexcept { return ice::count(_stages); }

    private:
        ice::Allocator& _allocator;
        ice::gfx::v2::GfxRenderGraphBuilder& _graph_builder;
        ice::Array<ice::UniquePtr<SimpleGfxRenderStageBuilder>, ContainerLogic::Complex> _stages;
    };

    class SimpleGfxRenderGraphBuilder : public ice::gfx::v2::GfxRenderGraphBuilder
    {
    public:
        SimpleGfxRenderGraphBuilder(ice::Allocator& alloc) noexcept;
        ~SimpleGfxRenderGraphBuilder() noexcept override;

        auto import_image(ice::StringID_Arg name) noexcept -> Handle override;
        auto create_image(ice::StringID_Arg name) noexcept -> Handle override;
        auto renderpass() noexcept -> GfxRenderPassBuilder& override;
    //    void renderpass_begin(ice::StringID_Arg name) noexcept override;
    //    void renderpass_subpass_begin() noexcept override;
    //    void read_texture(ice::StringID_Arg name, ice::render::ImageFormat format) noexcept override;
    //    void write_texture(ice::StringID_Arg name, ice::render::ImageFormat format) noexcept override;
    //    void read_depthstencil(ice::StringID_Arg name, ice::render::ImageFormat format) noexcept override;
    //    void write_depthstencil(ice::StringID_Arg name, ice::render::ImageFormat format) noexcept override;
    //    void renderpass_end() noexcept override;

        void build() noexcept;

        auto rendergraph() noexcept -> ice::UniquePtr<ice::gfx::v2::GfxRenderGraph> override;

    //public:
    //    enum class BuildContext : ice::u8 {
    //        Global = 0x00,
    //        RenderPass = 0x01,
    //        Pipeline = 0x02,
    //        Object = 0x04,
    //    };

    //    enum class BuildDataType : ice::u16 {
    //        None,
    //        RenderPass,
    //        RenderPass_SubPass,
    //        RenderPass_RenderAttachment,
    //        RenderPass_AttachmentReference,
    //        RenderPass_InputImage,
    //        RenderPass_OutputIMage,
    //    };

    //    struct BuildEntry
    //    {
    //        BuildContext context;
    //        GfxObjectType type;
    //        GfxStageDefinition definition;

    //        struct BuildData
    //        {
    //            BuildDataType type;
    //            BuildData* next;
    //            BuildData* child;
    //            ice::StringID name;

    //            union
    //            {
    //                ice::render::AttachmentReference renderpass_attachref;
    //                ice::render::RenderpassInfo renderpass_info;
    //                ice::render::RenderSubPass renderpass_subpass;
    //                ice::render::RenderAttachment renderpass_attachment;
    //            } value;
    //        } data;
    //        //void* data_memory;
    //    };

    private:
        ice::Allocator& _allocator;
        ice::Array<ice::Shard> _references;
        ice::HashMap<ice::ucount> _handles;

        ice::Array<void*> _builder_reference_params;

        ice::Array<ice::UniquePtr<SimpleGfxRenderPassBuilder>, ContainerLogic::Complex> _passes;
    //    ice::Array<BuildEntry> _build_stack;
    //    ice::Array<BuildEntry> _build_definitions;
        ice::Memory _final_data;
    };

    class ConstantGfxRenderGraph final : public ice::gfx::v2::GfxRenderGraph
    {
    public:
        ConstantGfxRenderGraph(
            ice::Allocator& alloc,
            ice::Array<GfxStageDefinition> stages,
            ice::gfx::v2::GfxRenderGraphData const& rgdata,
            ice::Memory data
        ) noexcept
            : _allocator{ alloc }
            , _stages{ ice::move(stages) }
            , _stage_names{ alloc }
            , _graph{ rgdata }
            , _data{ data }
        {
            ice::array::reserve(_stage_names, ice::count(_stages));
            for (GfxStageDefinition const& def : _stages)
            {
                ice::array::push_back(_stage_names, def.name);
            }
        }

        ~ConstantGfxRenderGraph() noexcept
        {
            _allocator.deallocate(_data);
        }

        auto names() const noexcept -> ice::Span<ice::StringID const> { return _stage_names; }
        auto definitions() const noexcept -> ice::Span<ice::gfx::v2::GfxStageDefinition const> override { return _stages; }

        auto setup_stages() const noexcept -> ice::Span<ice::StringID const> override
        {
            return ice::span::subspan(names(), 0, _graph.num_references_managed);
        }

        auto runtime_stages() const noexcept -> ice::Span<ice::StringID const> override
        {
            return ice::span::subspan(names(), _graph.num_references_managed);
        }

        //auto create_runtime(
        //    ice::gfx::GfxDevice& device
        //) noexcept -> ice::UniquePtr<ice::gfx::v2::GfxRenderGraphRuntime> override { return {}; }

        auto create_runtime(
            ice::Allocator& alloc,
            ice::gfx::GfxDevice& device
        ) const noexcept -> ice::UniquePtr<ice::gfx::v2::GfxRenderGraphRuntime> override;

    private:
        ice::Allocator& _allocator;
        ice::Array<GfxStageDefinition> _stages;
        ice::Array<ice::StringID> _stage_names;
        ice::gfx::v2::GfxRenderGraphData const& _graph;
        ice::Memory _data;
    };

} // namespace ice::gfx::v2
