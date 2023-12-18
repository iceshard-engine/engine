/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine_types.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/span.hxx>
#include <ice/stringid.hxx>
#include <ice/math.hxx>
#include <ice/render/render_declarations.hxx>

namespace ice::gfx::v2
{

    enum class GfxStageFlags : ice::u32
    {
        None = 0x0000'0000,

        Import = 0x2000'0000,
        Setup = 0x4000'0000,
        BuiltIn = 0x8000'0000,

        BuiltInStages = BuiltIn,
        BuiltIn_Present,
        BuiltIn_Clear,

        ImportStages = BuiltIn | Import,
        Import_Image,

        CreateStages = BuiltIn | Setup,
        Create_Image,
        Create_Framebuffer,
        Create_Renderpass,
        Create_Pipeline,

        //Create_Renderpass = 0x0000'0001 | BuiltIn | Setup,
        //Create_Image = 0x0000'0004 | BuiltIn | Setup,
        //Create_Buffer = 0x0000'0008 | BuiltIn | Setup,
        //Create_Pipeline = 0x0000'0010 | BuiltIn | Setup,
        //Create_ResourceSet = 0x0000'0010 | BuiltIn | Setup,
    };

    struct GfxStageDescription
    {
        ice::gfx::v2::GfxStageFlags flags;
        //ice::Shard data_v2;




        //union Data
        //{
        //    union ConstantData
        //    {
        //        struct ClearData
        //        {
        //            ice::vec4f color;
        //        } clear;
        //    } constant;
        //    union CreateData
        //    {
        //        struct FrameBufferData
        //        {
        //            ice::vec2u extent;
        //        } framebuffer;
        //    } create;
        //    struct DynamicData
        //    {

        //    } dynamic;
        //} data;
    };

    struct GfxStageIO
    {
        ice::u32 id;
        ice::u16 type;
        ice::u16 idx;
    };

    struct GfxStageDefinition
    {
        ice::StringID name;
        ice::gfx::v2::GfxStageFlags flags;
        ice::ucount refidx = ice::ucount_max;
        ice::meminfo runtime_memory;
        ice::Data params;

        ice::Span<ice::gfx::v2::GfxStageIO const> autoio;

        ice::gfx::v2::GfxStageDescription desc;
    };

    struct GfxRenderGraphPassData
    {
        ice::ucount num_stages;
    };

    struct GfxRenderGraphReferenceData
    {
        ice::Shard reference;
        ice::usize offset_params;
    };

    struct GfxRenderGraphStageData
    {
        ice::StringID name;
        ice::gfx::v2::GfxStageFlags flags;
        ice::usize offset_params;
    };

    struct GfxRenderGraphData
    {
        ice::ucount num_renderpasses;
        ice::ucount num_references;
        ice::ucount num_references_managed;
        ice::ucount num_stages;
        ice::gfx::v2::GfxRenderGraphPassData const* renderpasses;
        /*ice::Shard const* references;
        ice::usize const* reference_params_offsets;*/
        ice::gfx::v2::GfxRenderGraphReferenceData const* references;
        ice::gfx::v2::GfxRenderGraphStageData const* stages;
        ice::Data params_data;
    };

    struct GfxRenderGraph
    {
        virtual ~GfxRenderGraph() noexcept = default;

        virtual auto definitions() const noexcept -> ice::Span<ice::gfx::v2::GfxStageDefinition const> = 0;
        virtual auto setup_stages() const noexcept -> ice::Span<ice::StringID const> = 0;
        virtual auto runtime_stages() const noexcept -> ice::Span<ice::StringID const> = 0;

        virtual auto create_runtime(
            ice::Allocator& alloc,
            ice::gfx::GfxDevice& device
        ) const noexcept -> ice::UniquePtr<ice::gfx::v2::GfxRenderGraphRuntime> = 0;
    };

    struct GfxBuilderHandle
    {
        void* internal;

        constexpr auto id() const noexcept -> ice::u32
        {
            return ice::u32(ice::bit_cast<ice::u64>(internal));
        }

        constexpr auto type() const noexcept -> ice::u16
        {
            return ice::u16(ice::bit_cast<ice::u64>(internal) >> 32);
        }

        constexpr bool same_handle_type(GfxBuilderHandle other) const noexcept
        {
            return type() == other.type();
        }

        static constexpr bool same_handles(ice::Span<GfxBuilderHandle const> handles) noexcept
        {
            bool all_same = true;
            auto const first = ice::span::front(handles);
            for (auto other : ice::span::subspan(handles, 1))
            {
                all_same &= first.same_handle_type(other);
            }
            return all_same;
        }

        static constexpr auto create(ice::u16 type, ice::u32 id) noexcept -> GfxBuilderHandle
        {
            return { ice::bit_cast<void*>((ice::u64(type) << 32) | ice::u64(id)) };
        }
    };

    enum class GfxBuildInStage : ice::u8
    {
        None,
        CreateRenderpass,
        CreateFramebuffer,
    };

    struct GfxRenderStageBuilder
    {
        virtual ~GfxRenderStageBuilder() noexcept = default;

        virtual auto builtin(GfxBuildInStage stage_type) noexcept -> GfxRenderStageBuilder& = 0;
        virtual auto depthstencil(GfxBuilderHandle handle) noexcept -> GfxRenderStageBuilder& = 0;
        virtual auto inputs(ice::Span<GfxBuilderHandle const> inputs, ice::u16 starting_index = 0) noexcept -> GfxRenderStageBuilder& = 0;
        virtual auto outputs(ice::Span<GfxBuilderHandle const> outputs, ice::u16 starting_index = 0) noexcept -> GfxRenderStageBuilder& = 0;

        //struct Data
        //{
        //    ice::ucount num_inputs;
        //    ice::ucount num_outputs;
        //    ice::ucount const* input_indices;
        //    ice::ucount const* output_indices;
        //};
    };

    struct GfxRenderPassBuilder
    {
        virtual ~GfxRenderPassBuilder() noexcept = default;

        virtual auto clear_color(ice::vec4f color) noexcept -> GfxRenderPassBuilder& = 0;

        virtual auto stage(ice::StringID_Arg name) noexcept -> GfxRenderStageBuilder& = 0;

        struct Data
        {
            ice::ucount num_stages;
            ice::ucount num_clear_colors;
            //ice::gfx::v2::GfxRenderStageBuilder::Data const* stages;
            ice::vec4f const* clear_colors;
        };
    };

    struct GfxRenderGraphBuilder
    {
        using Handle = GfxBuilderHandle;

        virtual ~GfxRenderGraphBuilder() noexcept = default;

        virtual auto import_image(ice::StringID_Arg name) noexcept -> GfxBuilderHandle = 0;
        virtual auto create_image(ice::StringID_Arg name) noexcept -> GfxBuilderHandle = 0;

        virtual auto renderpass() noexcept -> GfxRenderPassBuilder& = 0;
        //virtual bool query_definitions(ice::Array<ice::gfx::v2::GfxStageDefinition const>& definitions) noexcept = 0;

        //struct Handle { void* value; };

        //virtual auto create_image() noexcept -> Handle = 0;
        //virtual auto import_image(ice::StringID_Arg name) noexcept -> Handle = 0;

        //virtual void renderpass_begin(ice::StringID_Arg name) noexcept = 0;
        //virtual void renderpass_subpass_begin() noexcept = 0;
        //virtual void read_texture(ice::StringID_Arg name, ice::render::ImageFormat format) noexcept = 0;
        //virtual void write_texture(ice::StringID_Arg name, ice::render::ImageFormat format) noexcept = 0;
        //virtual void read_depthstencil(ice::StringID_Arg name, ice::render::ImageFormat format) noexcept = 0;
        //virtual void write_depthstencil(ice::StringID_Arg name, ice::render::ImageFormat format) noexcept = 0;
        //virtual void renderpass_end() noexcept = 0;

        virtual auto rendergraph() noexcept -> ice::UniquePtr<ice::gfx::v2::GfxRenderGraph> = 0;

        struct Data
        {
            ice::ucount num_renderpasses;
            ice::ucount num_references;
            ice::gfx::v2::GfxRenderPassBuilder::Data const* renderpasses;
            ice::Shard const* references;
            ice::usize const* reference_params_offsets;
            void const* reference_params;
        };
    };

    auto create_rendergraph(
        ice::Allocator& alloc,
        ice::Span<ice::gfx::v2::GfxStageDefinition const> definitions
    ) noexcept -> ice::UniquePtr<ice::gfx::v2::GfxRenderGraph>;

    auto create_rendergraph(
        ice::Allocator& alloc,
        ice::Memory data,
        ice::gfx::v2::GfxRenderGraphData const& render_graph_data
    ) noexcept -> ice::UniquePtr<ice::gfx::v2::GfxRenderGraph>;

    auto create_rendergraph_builder(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::gfx::v2::GfxRenderGraphBuilder>;

} // namespace ice::gfx::v2
