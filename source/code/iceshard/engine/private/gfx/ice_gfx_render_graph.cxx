/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "ice_gfx_render_graph.hxx"
#include "stages/builtin_create_stages.hxx"

#include <ice/render/render_swapchain.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/sort.hxx>

namespace ice::gfx::v2
{

    //namespace detail
    //{

    //    using BuildData = SimpleGfxRenderGraphBuilder::BuildEntry::BuildData;
    //    using BuildDataType = SimpleGfxRenderGraphBuilder::BuildDataType;

    //    constexpr auto list_length(BuildData* data, ice::ucount initial = 0) noexcept -> ice::ucount
    //    {
    //        if (data == nullptr) return initial;
    //        return list_length(data->next, initial + 1);
    //    }

    //    constexpr auto list_length_type(BuildData* data, BuildDataType type, ice::ucount initial = 0) noexcept -> ice::ucount
    //    {
    //        if (data == nullptr) return initial;
    //        return list_length_type(data->next, type, initial + (data->type == type));
    //    }

    //    constexpr auto list_children_count(BuildData* data, ice::ucount depth = 1, ice::ucount initial = 0) noexcept -> ice::ucount
    //    {
    //        if (data == nullptr) return initial;
    //        if (depth == 0) return list_length(data, initial);
    //        return list_children_count(data->next, depth, list_children_count(data->child, depth - 1, initial));
    //    }

    //    constexpr auto list_children_count_type(BuildData* data, BuildDataType type, ice::ucount depth = 1, ice::ucount initial = 0) noexcept -> ice::ucount
    //    {
    //        if (data == nullptr) return initial;
    //        if (depth == 0) return list_length_type(data, type, initial);
    //        return list_children_count_type(data->next, type, depth, list_children_count_type(data->child, type, depth - 1, initial));
    //    }

    //    void tree_destroy(ice::Allocator& alloc, BuildData* data) noexcept
    //    {
    //        if (data == nullptr) return;
    //        if (data->child != nullptr) tree_destroy(alloc, data->child);
    //        if (data->next != nullptr) tree_destroy(alloc, data->next);
    //        alloc.destroy(data);
    //    }

    //    auto renderpass_memory_requirements(BuildData& data) noexcept -> ice::meminfo
    //    {
    //        ice::ucount const subpass_count = list_length(data.child);

    //        ice::meminfo rpmem = ice::meminfo_of<render::RenderpassInfo>;
    //        ice::usize const sub_offset = rpmem += ice::meminfo_of<render::RenderSubPass> * subpass_count;
    //        ice::usize const dep_offset = rpmem += ice::meminfo_of<render::SubpassDependency> * (subpass_count - 1);
    //        ice::usize const att_offset = rpmem += ice::meminfo_of<render::RenderAttachment> * list_children_count_type(data.child, BuildDataType::RenderPass_RenderAttachment);
    //        return rpmem;
    //    }

    //    //auto count_children(BuildData* data, ice::ucount initial = 0) noexcept -> ice::ucount
    //    //{
    //    //    if (data->child == nullptr) return initial;
    //    //    return list_length(data->child, initial + 1);
    //    //}

    //} // namespace detail


    void GfxStage::update(
        ice::gfx::v2::GfxStageParams const& params,
        ice::gfx::v2::GfxStageDefinition const& def,
        ice::gfx::v2::GfxObjectStorage& resources
    ) noexcept
    {
        ice::render::RenderDevice& device = params.device.device();

        // Image resource set
        ice::render::ResourceSet rs_img, rs_buf;
        resources.get_by_index(IHT_Image, rs_img);
        resources.get_by_index(IHT_Buffer, rs_buf);

        for (GfxStageIO io : def.autoio)
        {
            switch (io.type)
            {
            case IHT_Buffer: break;
            case IHT_Image:
            {
                //ice::render::Handl
                ice::render::Image img;
                resources.get_by_index(io.id, img);
                ice::render::ResourceUpdateInfo const rui{ .image = img };
                ice::render::ResourceSetUpdateInfo const ui{
                    .resource_set = rs_img,
                    .resource_type = ice::render::ResourceType::SampledImage,
                    .binding_index = io.idx,
                    .resources = { &rui, 1 }
                };
                device.update_resourceset({ &ui, 1 });
            }
            case IHT_DepthStencil: break;
            default:
                break;
            }

        }
    }

    SimpleGfxRenderGraph::SimpleGfxRenderGraph(
        ice::Allocator& alloc,
        ice::Array<ice::gfx::v2::GfxStageDefinition>&& definitions,
        ice::ucount count_setup_stages
    ) noexcept
        : _allocator{ alloc, "render-graph" }
        , _graph_stage_definitions{ ice::move(definitions) }
        , _graph_stage_names{ _allocator }
        , _count_setup_stages{ count_setup_stages }
    {
        ice::array::reserve(_graph_stage_names, ice::count(_graph_stage_definitions));
        for (ice::gfx::v2::GfxStageDefinition const& def : _graph_stage_definitions)
        {
            ice::array::push_back(_graph_stage_names, def.name);
        }
    }

    auto SimpleGfxRenderGraph::definitions() const noexcept -> ice::Span<ice::gfx::v2::GfxStageDefinition const>
    {
        return ice::array::slice(_graph_stage_definitions);
    }

    auto SimpleGfxRenderGraph::setup_stages() const noexcept -> ice::Span<ice::StringID const>
    {
        return ice::array::slice(_graph_stage_names, 0, _count_setup_stages);
    }

    auto SimpleGfxRenderGraph::runtime_stages() const noexcept -> ice::Span<ice::StringID const>
    {
        return ice::array::slice(_graph_stage_names, _count_setup_stages);
    }

    auto SimpleGfxRenderGraph::create_runtime(ice::gfx::GfxDevice& device) noexcept -> ice::UniquePtr<ice::gfx::v2::GfxRenderGraphRuntime>
    {
        return {};
    }

    auto create_rendergraph(
        ice::Allocator& alloc,
        ice::Span<ice::gfx::v2::GfxStageDefinition const> user_definitions
    ) noexcept -> ice::UniquePtr<ice::gfx::v2::GfxRenderGraph>
    {
        ice::ucount setup_stages = 0;
        for (GfxStageDefinition const& definition : user_definitions)
        {
            if (ice::has_all(definition.desc.flags, GfxStageFlags::Setup))
            {
                setup_stages += 1;
            }
        }

        ice::Array<ice::gfx::v2::GfxStageDefinition> definitions{ alloc, user_definitions };
        ice::sort(
            ice::array::slice(definitions),
            [](GfxStageDefinition const& left, GfxStageDefinition const& right)
            {
                bool const left_create = ice::has_all(left.desc.flags, GfxStageFlags::Setup);
                bool const right_create = ice::has_all(right.desc.flags, GfxStageFlags::Setup);
                return left_create > right_create;
            }
        );

        return { };
        //return ice::make_unique<SimpleGfxRenderGraph>(alloc, alloc, ice::move(definitions), setup_stages);
    }

    SimpleGfxRenderPassBuilder::SimpleGfxRenderPassBuilder(
        ice::Allocator& alloc,
        ice::gfx::v2::GfxRenderGraphBuilder& graph_builder
    ) noexcept
        : _allocator{ alloc }
        , _graph_builder{ graph_builder }
        , _stages{ _allocator }
    {
    }

    auto SimpleGfxRenderPassBuilder::stage(ice::StringID_Arg name) noexcept -> GfxRenderStageBuilder&
    {
        ice::array::push_back(_stages, ice::make_unique<SimpleGfxRenderStageBuilder>(_allocator, _allocator, name));
        return *ice::array::back(_stages);
    }

    SimpleGfxRenderGraphBuilder::SimpleGfxRenderGraphBuilder(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _handles{ _allocator }
        , _references{ _allocator }
        , _passes{ _allocator }
        , _builder_reference_params{ _allocator }
        //, _build_stack{ _allocator }
        //, _build_definitions{ _allocator }
    {
        //ice::array::reserve(_build_stack, 4);
        //ice::array::reserve(_build_definitions, 50);
        //ice::array::push_back(_build_stack, BuildEntry{ BuildContext::Global, GfxObjectType::Unknown });
    }

    SimpleGfxRenderGraphBuilder::~SimpleGfxRenderGraphBuilder() noexcept
    {
        for (void* ref_param : _builder_reference_params)
        {
            _allocator.deallocate(ref_param);
        }
        _allocator.deallocate(_final_data);
    //    for (auto& entry : _build_definitions)
    //    {
    //        detail::tree_destroy(_allocator, entry.data.child);
    //    }
    //    for (auto& entry : _build_stack)
    //    {
    //        detail::tree_destroy(_allocator, entry.data.child);
    //    }
    }

    auto SimpleGfxRenderGraphBuilder::import_image(ice::StringID_Arg name) noexcept -> Handle
    {
        ice::u64 const name_hash = ice::hash(name);
        ice::ucount handle_idx = ice::hashmap::get(_handles, name_hash, ice::ucount_max);
        if (handle_idx == ice::ucount_max)
        {
            handle_idx = ice::array::count(_references);
            ice::array::push_back(_references, "image.imported"_shardid | ice::stringid_hash(name));
            ice::array::push_back(_builder_reference_params, nullptr);
        }
        return Handle::create(IHT_Image, handle_idx);
    }

    auto SimpleGfxRenderGraphBuilder::create_image(ice::StringID_Arg name) noexcept -> Handle
    {
        ice::u64 const name_hash = ice::hash(name);
        ice::ucount handle_idx = ice::hashmap::get(_handles, name_hash, ice::ucount_max);
        if (handle_idx == ice::ucount_max)
        {
            handle_idx = ice::array::count(_references);
            ice::array::push_back(_references, "image.managed"_shardid | ice::stringid_hash(name));
            auto* params = _allocator.create<builtin::Stage_CreateImage::Params>(
                name,
                render::ImageType::Image2D,
                render::ImageFormat::Invalid,
                render::ImageUsageFlags::Sampled | render::ImageUsageFlags::TransferDst,
                ice::vec2f{ 0.f, 0.f }
            );
            ice::array::push_back(_builder_reference_params, params);
        }
        return Handle::create(IHT_Image, handle_idx);
    }

    auto SimpleGfxRenderGraphBuilder::renderpass() noexcept -> GfxRenderPassBuilder&
    {
        ice::array::push_back(_passes, ice::make_unique<SimpleGfxRenderPassBuilder>(_allocator, _allocator, *this));
        return *ice::array::back(_passes);
    }

    //using BuildData = SimpleGfxRenderGraphBuilder::BuildEntry::BuildData;

    //void SimpleGfxRenderGraphBuilder::renderpass_begin(ice::StringID_Arg name) noexcept
    //{
    //    ICE_ASSERT(
    //        ice::array::back(_build_stack).context == BuildContext::Global,
    //        "A renderpass can only be created in the global context. Finish your previous builder context first!"
    //    );

    //    BuildEntry entry{ .context = BuildContext::RenderPass, .type = GfxObjectType::Renderpass };
    //    entry.definition.name = name;
    //    entry.definition.desc.flags = GfxStageFlags::Create_Renderpass;
    //    entry.data.type = BuildDataType::RenderPass;
    //    entry.data.value.renderpass_info = render::RenderpassInfo{};
    //    entry.data.next = nullptr;
    //    entry.data.child = _allocator.create<BuildData>(BuildData{ .type = BuildDataType::RenderPass_SubPass });

    //    ice::array::push_back(_build_stack, entry);
    //}

    //void SimpleGfxRenderGraphBuilder::renderpass_subpass_begin() noexcept
    //{
    //    BuildEntry& entry = ice::array::back(_build_stack);
    //    ICE_ASSERT(
    //        entry.context == BuildContext::RenderPass && entry.type == GfxObjectType::Renderpass,
    //        "A texture can only be reference the renderpass context. Finish your previous context first!"
    //    );

    //    BuildData* subpass = _allocator.create<BuildData>(BuildData{ .type = BuildDataType::RenderPass_SubPass });
    //    subpass->next = ice::exchange(entry.data.child, subpass);
    //}

    //void SimpleGfxRenderGraphBuilder::read_texture(ice::StringID_Arg name, ice::render::ImageFormat format) noexcept
    //{
    //    BuildEntry& entry = ice::array::back(_build_stack);
    //    ICE_ASSERT(
    //        entry.context == BuildContext::RenderPass && entry.type == GfxObjectType::Renderpass,
    //        "A texture can only be reference the renderpass context. Finish your previous context first!"
    //    );

    //    if (format == render::ImageFormat::Invalid)
    //    {
    //        BuildData* subpass = entry.data.child;
    //        BuildData* attach_ref = _allocator.create<BuildData>(BuildData{ .type = BuildDataType::RenderPass_AttachmentReference });
    //        attach_ref->next = ice::exchange(subpass->child, attach_ref);
    //        attach_ref->name = name;
    //        attach_ref->value.renderpass_attachref.layout = render::ImageLayout::ShaderReadOnly;
    //        attach_ref->value.renderpass_attachref.attachment_index = ice::ucount_max;
    //        return;
    //    }

    //    BuildData* subpass = entry.data.child;
    //    BuildData* attach_bd = _allocator.create<BuildData>(BuildData{ .type = BuildDataType::RenderPass_RenderAttachment });
    //    attach_bd->next = ice::exchange(subpass->child, attach_bd);
    //    attach_bd->name = name;

    //    render::RenderAttachment& attach = attach_bd->value.renderpass_attachment;
    //    attach.type = render::AttachmentType::TextureImage;
    //    attach.format = format;
    //    attach.initial_layout = render::ImageLayout::Undefined;
    //    attach.final_layout = render::ImageLayout::ShaderReadOnly;
    //    attach.operations[0] = render::AttachmentOperation::Load_DontCare;
    //    attach.operations[1] = render::AttachmentOperation::Store_DontCare;
    //}

    //void SimpleGfxRenderGraphBuilder::write_texture(ice::StringID_Arg name, ice::render::ImageFormat format) noexcept
    //{
    //    BuildEntry& entry = ice::array::back(_build_stack);
    //    ICE_ASSERT(
    //        entry.context == BuildContext::RenderPass && entry.type == GfxObjectType::Renderpass,
    //        "A texture can only be reference the renderpass context. Finish your previous context first!"
    //    );

    //    if (format == render::ImageFormat::Invalid)
    //    {
    //        BuildData* subpass = entry.data.child;
    //        BuildData* attach_ref = _allocator.create<BuildData>(BuildData{ .type = BuildDataType::RenderPass_AttachmentReference });
    //        attach_ref->next = ice::exchange(subpass->child, attach_ref);
    //        attach_ref->name = name;
    //        attach_ref->value.renderpass_attachref.layout = render::ImageLayout::Color;
    //        attach_ref->value.renderpass_attachref.attachment_index = ice::ucount_max;
    //        return;
    //    }

    //    BuildData* subpass = entry.data.child;
    //    BuildData* attach_bd = _allocator.create<BuildData>(BuildData{ .type = BuildDataType::RenderPass_RenderAttachment });
    //    attach_bd->next = ice::exchange(subpass->child, attach_bd);
    //    attach_bd->name = name;

    //    render::RenderAttachment& attach = attach_bd->value.renderpass_attachment;
    //    attach.type = render::AttachmentType::TextureImage;
    //    attach.format = format;
    //    attach.initial_layout = render::ImageLayout::Undefined;
    //    attach.final_layout = render::ImageLayout::Color;
    //    attach.operations[0] = render::AttachmentOperation::Load_Clear;
    //    attach.operations[1] = render::AttachmentOperation::Store_DontCare;
    //}

    //void SimpleGfxRenderGraphBuilder::read_depthstencil(ice::StringID_Arg name, ice::render::ImageFormat format) noexcept
    //{
    //}

    //void SimpleGfxRenderGraphBuilder::write_depthstencil(ice::StringID_Arg name, ice::render::ImageFormat format) noexcept
    //{
    //}

    //void SimpleGfxRenderGraphBuilder::renderpass_end() noexcept
    //{
    //    BuildEntry& entry = ice::array::back(_build_stack);
    //    ICE_ASSERT(
    //        entry.context == BuildContext::RenderPass && entry.type == GfxObjectType::Renderpass,
    //        "A texture can only be reference the renderpass context. Finish your previous context first!"
    //    );

    //    [[maybe_unused]]
    //    ice::ucount const subpasses = detail::list_length(entry.data.child);
    //    [[maybe_unused]]
    //    ice::ucount const attachments = detail::list_children_count(entry.data.child);
    //    [[maybe_unused]]
    //    ice::ucount const references = detail::list_children_count_type(entry.data.child, BuildDataType::RenderPass_AttachmentReference);

    //    // Destroy the BuilData tree
    //    //detail::tree_destroy(_allocator, entry.data.child);
    //    ice::array::push_back(_build_definitions, entry);

    //    // Pop the context from the stack.
    //    ice::array::pop_back(_build_stack);
    //}

    void SimpleGfxRenderGraphBuilder::build() noexcept
    {
        ice::meminfo params_size{};
        for (ice::Shard ref : _references)
        {
            switch (ref.id.name.value)
            {
            case "image.managed"_shardid.name.value:
                params_size += ice::meminfo_of<builtin::Stage_CreateImage::Params>;
                break;
            default:
                break;
            };
        }

        for (auto const& pass_builder : _passes)
        {
            for (auto const& stage_builder : pass_builder->stages())
            {
                params_size += ice::meminfo_of<ice::u32> * 2;
                params_size += ice::meminfo_of<InternalHandleRef> * ice::count(stage_builder->_inputs);
                params_size += ice::meminfo_of<InternalHandleRef> * ice::count(stage_builder->_outputs);

                switch (stage_builder->_builtin)
                {
                using enum GfxBuildInStage;
                case None:
                    break;
                case CreateRenderpass:
                    //params_size += ice::meminfo_of<builtin::Stage_CreateRenderpass::Params>;
                    break;
                case CreateFramebuffer:
                    params_size += ice::meminfo_of<builtin::Stage_CreateFramebuffer::Params>;
                    break;
                default:
                    break;
                };
            }
        }

        ice::ucount const total_stage_count = ice::accumulate_over(ice::array::slice(_passes), [](auto const& o) { return ice::count(o->stages()); });

        ice::meminfo total_size = ice::meminfo_of<GfxRenderGraphData>;
        ice::usize const offset_passes = total_size += ice::meminfo_of<GfxRenderGraphPassData> * ice::count(_passes);
        ice::usize const offset_references = total_size += ice::meminfo_of<GfxRenderGraphReferenceData> * ice::count(_references);
        ice::usize const offset_stages = total_size += ice::meminfo_of<GfxRenderGraphStageData> * total_stage_count;
        ice::usize const offset_params = total_size += params_size;

        ice::Memory buffer = _allocator.allocate(total_size);
        {
            auto* const pass_array= reinterpret_cast<ice::gfx::v2::GfxRenderGraphPassData*>(ice::ptr_add(buffer, offset_passes).location);
            auto* const refs_array= reinterpret_cast<ice::gfx::v2::GfxRenderGraphReferenceData*>(ice::ptr_add(buffer, offset_references).location);
            auto* const stages_array= reinterpret_cast<ice::gfx::v2::GfxRenderGraphStageData*>(ice::ptr_add(buffer, offset_stages).location);

            GfxRenderGraphData* const rgdata = reinterpret_cast<GfxRenderGraphData*>(buffer.location);
            rgdata->num_renderpasses = ice::count(_passes);
            rgdata->num_references = ice::count(_references);
            rgdata->num_references_managed = 0;
            rgdata->num_stages = total_stage_count;
            rgdata->renderpasses = pass_array;
            rgdata->references = refs_array;
            rgdata->stages = stages_array;
            rgdata->params_data = ice::data_view(ice::ptr_add(buffer, offset_params));

            ice::usize params_offset = 0_B;
            ice::Memory params_data = ice::ptr_add(buffer, offset_params);

            auto refs_it = refs_array;
            auto refs_params_it = ice::array::begin(_builder_reference_params);
            for (auto const& reference : _references)
            {
                refs_it->reference = reference;

                switch (reference.id.name.value)
                {
                case "image.managed"_shardid.name.value:
                {
                    rgdata->num_references_managed += 1;
                    // TODO: Helper function?
                    ice::meminfo constexpr param_meminfo = ice::meminfo_of<builtin::Stage_CreateImage::Params>;
                    refs_it->offset_params = ice::align_to(params_offset, param_meminfo.alignment).value;
                    params_offset += param_meminfo.size;

                    ice::memcpy(
                        ice::ptr_add(params_data, refs_it->offset_params),
                        ice::data_view(*reinterpret_cast<builtin::Stage_CreateImage::Params const*>(*refs_params_it))
                    );
                    break;
                }
                default:
                    break;
                };

                refs_it += 1;
                refs_params_it += 1;
            }

            auto pass_it = pass_array;
            auto stages_it = stages_array;
            for (auto const& pass_builder : _passes)
            {
                pass_it->num_stages = ice::count(pass_builder->stages());

                for (auto const& stage_builder : pass_builder->stages())
                {
                    stages_it->flags = GfxStageFlags::None;
                    stages_it->name = stage_builder->_name;
                    stages_it->offset_params = ice::align_to(params_offset, ice::align_of<ice::u32>).value;

                    // Input / output info
                    ice::usize custom_offset = 0_B;
                    {
                        ice::usize iosize = ice::size_of<ice::u32> * 2;
                        iosize += ice::size_of<InternalHandleRef> * ice::count(stage_builder->_inputs);
                        iosize += ice::size_of<InternalHandleRef> * ice::count(stage_builder->_outputs);

                        ice::u32* iodata = reinterpret_cast<ice::u32*>(ice::ptr_add(params_data.location, stages_it->offset_params));
                        *(iodata++) = ice::count(stage_builder->_inputs);
                        *(iodata++) = ice::count(stage_builder->_outputs);

                        ice::Memory iomem{ iodata, ice::usize((iosize - ice::size_of<InternalHandleRef> * 2).value), ice::ualign::b_4 };
                        ice::memcpy(iomem, ice::array::data_view(stage_builder->_inputs));
                        iomem.location = ice::ptr_add(iomem.location, ice::array::data_view(stage_builder->_inputs).size);
                        ice::memcpy(iomem, ice::array::data_view(stage_builder->_outputs));

                        custom_offset = params_offset += iosize;
                    }

                    switch (stage_builder->_builtin)
                    {
                    using enum GfxBuildInStage;
                    case None: break;
                    case CreateRenderpass:
                        stages_it->flags = GfxStageFlags::Create_Renderpass;
                        break;
                    case CreateFramebuffer:
                    {
                        stages_it->flags = GfxStageFlags::Create_Framebuffer;
                        //ice::meminfo constexpr param_meminfo = ice::meminfo_of<builtin::Stage_CreateFramebuffer::Params>;
                        //ice::meminfo const inputs_meminfo = ice::meminfo_of<ice::u32> *ice::count(stage_builder->_inputs);
                        //stages_it->offset_params = ice::align_to(params_offset, param_meminfo.alignment).value;
                        //params_offset += param_meminfo.size;
                        //ice::usize const offset_inputs = ice::align_to(params_offset, inputs_meminfo.alignment).value;
                        //params_offset += inputs_meminfo.size;

                        //builtin::Stage_CreateFramebuffer::Params const params{
                        //    //.input_count = ice::count(stage_builder->_inputs)
                        //};
                        //ice::memcpy(
                        //    ice::ptr_add(params_data, stages_it->offset_params),
                        //    ice::data_view(params)
                        //);

                        //ice::u32* const inputs = reinterpret_cast<ice::u32*>(ice::ptr_add(params_data, offset_inputs).location);
                        //for (auto const& input : stage_builder->_inputs)
                        //{
                        //    *inputs = ice::u32(ice::bit_cast<ice::u64>(input));
                        //}
                        break;
                    }
                    default:
                        //ICE_ASSERT_CORE(false);
                        break;
                    };
                    stages_it += 1;
                }

                pass_it += 1;
            }

            //GfxRenderGraphBuilder::Data* gb_data = reinterpret_cast<GfxRenderGraphBuilder::Data*>(buffer.location);
            //gb_data->num_references = ice::count(_references);
            //gb_data->num_renderpasses = 0;
            //gb_data->references = reinterpret_cast<ice::Shard const*>(ice::ptr_add(buffer, offset_refs).location);
            //gb_data->reference_params_offsets = reinterpret_cast<ice::usize const*>(ice::ptr_add(buffer, offset_param_offsets).location);
            //gb_data->renderpasses = nullptr;
            //gb_data->reference_params = ice::ptr_add(buffer, offset_params).location;

            //ice::memcpy(ice::ptr_add(buffer, offset_refs), ice::array::data_view(_references));
            //ice::Memory mem_params = ice::ptr_add(buffer, offset_params);

            //ice::usize last_offset = 0_B;
            //ice::usize* param_offset = reinterpret_cast<ice::usize*>(ice::ptr_add(buffer, offset_param_offsets).location);
            //for (void const* ref_params : _builder_reference_params)
            //{
            //    if (ref_params != nullptr)
            //    {
            //        *param_offset = ice::exchange(last_offset, last_offset + ice::size_of<builtin::Stage_CreateImage::Params>);
            //        ice::Memory target = ice::exchange(mem_params, ice::ptr_add(mem_params, ice::size_of<builtin::Stage_CreateImage::Params>));
            //        ice::memcpy(target, ice::data_view(*reinterpret_cast<builtin::Stage_CreateImage::Params const*>(ref_params)));
            //    }
            //    else
            //    {
            //        *param_offset = { ice::u64_max }; // TODO: usize_max
            //    }
            //    param_offset += 1;
            //}
        }

        _final_data = buffer;
    }

    auto SimpleGfxRenderGraphBuilder::rendergraph() noexcept -> ice::UniquePtr<ice::gfx::v2::GfxRenderGraph>
    {
        build();
#if 0
        ice::gfx::v2::GfxRenderGraphBuilder::Data const* data = reinterpret_cast<GfxRenderGraphBuilder::Data const*>(_final_data.location);
        ice::Span const references{ data->references, data->num_references };

        void const* data_params = data->reference_params_offsets;

        ice::Span<ice::usize const> param_offsets{ reinterpret_cast<ice::usize const*>(data_params), data->num_references };
        data_params = ice::align_to(ice::ptr_add(data_params, ice::span::size_bytes(param_offsets)), ualign::b_16).value;

        // Data for active runtime stages.
        ice::meminfo runtime_meminfo = ice::meminfo_of<void*>;

        ice::Array<ice::usize> runtime_data_offsets{ _allocator };
        //ice::Array<ice::gfx::v2::GfxObject> runtime_objects{ _allocator };
        //ice::array::reserve(runtime_objects, data->num_references);

        ice::HashMap<ice::gfx::v2::GfxObject> runtime_object_map{ _allocator };
        ice::hashmap::reserve(runtime_object_map, data->num_references);

        // Prepare data estimations
        for (ice::Shard ref : references)
        {
            ice::u64 const name_hash = ice::hash(ice::shard_shatter<ice::StringID_Hash>(ref, ice::StringID_Invalid));

            switch (ref.id.name.value)
            {
            case "image.managed"_shardid.name.value:
                ice::array::push_back(runtime_data_offsets, runtime_meminfo += ice::meminfo_of<ice::gfx::v2::builtin::Stage_CreateImage::Runtime>);
                //ice::array::push_back(runtime_objects, GfxObject{ ._type = GfxObjectType::Image, ._flags = GfxObjectFlags::None });
                ice::hashmap::set(
                    runtime_object_map,
                    name_hash,
                    GfxObject{ ._type = GfxObjectType::Image, ._flags = GfxObjectFlags::None }
                );
                break;
            case "image.imported"_shardid.name.value:
                ice::array::push_back(runtime_data_offsets, 0_B);
                //ice::array::push_back(runtime_objects, GfxObject{ ._type = GfxObjectType::Image, ._flags = GfxObjectFlags::Imported });
                ice::hashmap::set(
                    runtime_object_map,
                    name_hash,
                    GfxObject{ ._type = GfxObjectType::Image, ._flags = GfxObjectFlags::Imported }
                );
                break;
            default:
                break;
            }
        }

        // Allocate all required runtime data.
        ice::Memory runtime_data = _allocator.allocate(runtime_meminfo);
        ice::Array<ice::gfx::v2::GfxStage*> stages{ _allocator };

        ice::Array<ice::gfx::v2::GfxStageDefinition> defs{ _allocator };

        // Create runtime stages.
        ice::ucount count_setup_stages = 0;
        for (ice::Shard ref : references)
        {
            ice::u64 const name_hash = ice::hash(ice::shard_shatter<ice::StringID_Hash>(ref, ice::StringID_Invalid));

            switch (ref.id.name.value)
            {
            case "image.managed"_shardid.name.value:
            {
                //using ice::gfx::v2::builtin::Stage_CreateImage;
                //auto* const runtime = reinterpret_cast<Stage_CreateImage::Runtime*>(ice::ptr_add(runtime_data.location, runtime_data_offsets[idx]));
                //runtime->_out_image = ice::hashmap::try_get(runtime_object_map, name_hash);

                //auto* const stage = _allocator.create<Stage_CreateImage>(
                //    reinterpret_cast<Stage_CreateImage::Params const*>(ice::ptr_add(data_params, param_offsets[idx])),
                //    runtime
                //);
                //ice::array::push_back(stages, stage);
                count_setup_stages += 1;
                ice::array::push_back(defs,
                    GfxStageDefinition{
                        .name = { .value = name_hash },
                        .desc = { .flags = GfxStageFlags::Create_Image }
                    }
                );
                break;
            }
            case "image.imported"_shardid.name.value:
                break;
            default:
                break;
            }
        }

        //return ice::make_unique<ConstantGfxRenderGraph>(
        //    _allocator,
        //    _allocator,
        //    ice::move(defs),
        //    ice::move(_final_data)
        //);
#endif
        return create_rendergraph(_allocator, ice::exchange(_final_data, {}), *reinterpret_cast<ice::gfx::v2::GfxRenderGraphData const*>(_final_data.location));
    }

    BuilderGfxRenderGraph::BuilderGfxRenderGraph(
        ice::Allocator& alloc,
        ice::ucount count_setup_stages,
        ice::Array<ice::gfx::v2::GfxStageDefinition>&& definitions,
        ice::Memory definitions_data
    ) noexcept
        : SimpleGfxRenderGraph{ alloc, ice::move(definitions), count_setup_stages }
        , _data{ definitions_data }
    {
    }

    BuilderGfxRenderGraph::~BuilderGfxRenderGraph() noexcept
    {
        _allocator.deallocate(_data);
    }

    auto BuilderGfxRenderGraph::create_runtime(
        ice::gfx::GfxDevice& device
    ) noexcept -> ice::UniquePtr<ice::gfx::v2::GfxRenderGraphRuntime>
    {
        return {};
    }

    enum ObjectOffset
    {
        offset_framebuffers = 0,
        offset_swcimages = 4,
        reserved = 8,
        offset_renderpasses = 16,
    };

    auto ConstantGfxRenderGraph::create_runtime(
        ice::Allocator& alloc,
        ice::gfx::GfxDevice& device
    ) const noexcept -> ice::UniquePtr<ice::gfx::v2::GfxRenderGraphRuntime>
    {
        //render::RenderSwapchain const& swapchain = device.swapchain();
        //ice::ucount const count_swapchain_images = swapchain.image_count();

        ice::Array<ice::gfx::v2::GfxStage*> stages{ alloc };
        ice::array::reserve(stages, ice::ucount(ice::count(definitions()) * 1.3));

        ice::HashMap<ice::u32> reference_indices{ alloc };

        ice::Array<GfxStageState> passstates{ alloc };
        ice::array::resize(passstates, _graph.num_renderpasses);

        ice::Array<GfxObject> objects{ alloc };
        ice::array::resize(objects, _graph.num_references + _graph.num_renderpasses + 16); // 32 internal references for various things

        ice::ucount const offset_runtime_objects = offset_renderpasses + _graph.num_renderpasses;
        ice::ucount idx_next_object = offset_runtime_objects;

        ice::meminfo runtime_memory{};
        for (GfxStageDefinition const& stage : _stages)
        {
            runtime_memory += stage.runtime_memory;
        }

        ice::ucount idx_pass = 0;
        ice::ucount idx_framebuffer = 0;

        ice::ucount count_pass_stages = _graph.renderpasses[idx_pass].num_stages + /* initially we also count manager references */ _graph.num_references_managed;

        GfxStageState* pass_state = ice::addressof(passstates[idx_pass]);

        ice::Memory rmem = alloc.allocate(std::exchange(runtime_memory, {}));
        for (GfxStageDefinition const& stage : _stages)
        {
            switch (stage.flags)
            {
            case GfxStageFlags::Create_Framebuffer:
            {
                ice::usize const offset = runtime_memory += ice::meminfo_of<builtin::Stage_CreateFramebuffer::Runtime>;
                [[maybe_unused]] auto* const params = reinterpret_cast<builtin::Stage_CreateFramebuffer::Params const*>(stage.params.location);
                [[maybe_unused]] auto* const inputs = reinterpret_cast<ice::u32 const*>(ice::ptr_add(stage.params.location, ice::size_of<builtin::Stage_CreateFramebuffer::Params>));
                auto* const rt = reinterpret_cast<builtin::Stage_CreateFramebuffer::Runtime*>(ice::ptr_add(rmem, offset).location);
                auto* const rtinputs = reinterpret_cast<GfxObject const**>(ice::ptr_add(rt, ice::size_of<builtin::Stage_CreateFramebuffer::Runtime>));
                rt->_framebuffer = objects[offset_framebuffers + idx_framebuffer].direct_reference<render::Framebuffer>();
                rt->_renderpass = objects[offset_renderpasses + idx_pass].direct_reference<render::Renderpass>();
                rt->_inputs = rtinputs;
                //for (ice::ucount idx = 0; idx < params->input_count; ++idx)
                //{
                //    rt->_inputs[idx] = ice::addressof(objects[offset_runtime_objects + inputs[idx]]);
                //}
                //offset_runtime_objects += params->input_count;
                idx_framebuffer += 1;
                break;
            }
            case GfxStageFlags::Create_Renderpass:
            {
                ice::usize const offset = runtime_memory += ice::meminfo_of<builtin::Stage_CreateRenderpass::Runtime>;
                auto* const rt = reinterpret_cast<builtin::Stage_CreateRenderpass::Runtime*>(ice::ptr_add(rmem, offset).location);
                rt->_renderpass = objects[offset_renderpasses + idx_pass].direct_reference<render::Renderpass>();
                break;
            }
            case GfxStageFlags::Create_Image:
            {
                ice::usize const offset = runtime_memory += ice::meminfo_of<builtin::Stage_CreateImage::Runtime>;
                auto* const rt = reinterpret_cast<builtin::Stage_CreateImage::Runtime*>(ice::ptr_add(rmem, offset).location);
                rt->_image = objects[idx_next_object].direct_reference<ice::render::Image>();
                idx_next_object += 1;
                break;
            }
            default:
                ice::usize const offset = runtime_memory += stage.runtime_memory;
                break;
            }

            if (count_pass_stages -= 1; count_pass_stages == 0)
            {
                idx_pass += 1;
                idx_framebuffer = 0;
                count_pass_stages = _graph.renderpasses[idx_pass].num_stages;
                pass_state = ice::addressof(passstates[idx_pass]);
            }
        }

        return {};
    }

    auto create_rendergraph(
        ice::Allocator& alloc,
        ice::Memory data,
        ice::gfx::v2::GfxRenderGraphData const& rgdata
    ) noexcept -> ice::UniquePtr<ice::gfx::v2::GfxRenderGraph>
    {
        ice::Span<GfxRenderGraphPassData const> passes{ rgdata.renderpasses, rgdata.num_renderpasses };
        ice::Span<GfxRenderGraphReferenceData const> references{ rgdata.references, rgdata.num_references };
        ice::Span<GfxRenderGraphStageData const> stages{ rgdata.stages, rgdata.num_stages };

        ice::ucount const final_stage_count = ice::count(stages) + rgdata.num_references_managed;
        ice::Array<GfxStageDefinition> definitions{ alloc };
        ice::array::reserve(definitions, final_stage_count);

        //ice::Memory params_copy = alloc.allocate(rgdata.params_data.size);
        //ice::memcpy(params_copy, rgdata.params_data);

        //ice::ucount next_reference_index = 1; // idx == 0, swapchain
        ice::HashMap<ice::u32> reference_indices{ alloc };

        for (GfxRenderGraphReferenceData const& ref : references)
        {
            GfxStageDefinition definition{ ice::StringID_Invalid };
            if (ice::shard_inspect(ref.reference, definition.name.value) == false)
            {
                ICE_ASSERT(definition.name != ice::StringID_Invalid, "Invalid reference encountered in render graph data!");
                continue;
            }

            //ice::u64 const ref_hash = ice::hash(definition.name);
            //ice::u32 const ref_idx = ice::hashmap::get_or_set(reference_indices, ref_hash, ice::hashmap::count(reference_indices) + 1);

            switch (ref.reference.id.name.value)
            {
            case "image.imported"_shardid.name.value:
                break;
            case "image.managed"_shardid.name.value:
            {
                definition.flags = ice::gfx::v2::GfxStageFlags::Create_Image;
                definition.params = ice::ptr_add(rgdata.params_data, ref.offset_params);
                definition.runtime_memory = ice::meminfo_of<builtin::Stage_CreateImage::Runtime>;
                ice::array::push_back(definitions, definition);
                //definition.refidx = ref_idx;
                break;
            }
            default:
                break;
            }
        }

        for (GfxRenderGraphStageData const& stage : stages)
        {
            GfxStageDefinition definition{ stage.name };
            ICE_ASSERT(definition.name != ice::StringID_Invalid, "Invalid reference encountered in render graph data!");

            definition.flags = stage.flags;
            definition.params = ice::ptr_add(rgdata.params_data, stage.offset_params);

            // Load input / output info
            ice::u32 const* iodata = reinterpret_cast<ice::u32 const*>(definition.params.location);
            if (ice::has_none(stage.flags, GfxStageFlags::Create_Image))
            {
                ice::ucount const input_count = *(iodata++);
                ice::ucount const output_count = *(iodata++);

                definition.autoio = { (GfxStageIO const*)iodata, input_count + output_count };
                definition.params = ice::ptr_add(
                    definition.params,
                    ice::span::data_view(definition.autoio).size + ice::size_of<ice::u32> * 2
                );
            }

            switch (definition.flags)
            {
            case GfxStageFlags::Create_Framebuffer:
            {
                [[maybe_unused]] auto* const params = reinterpret_cast<builtin::Stage_CreateFramebuffer::Params const*>(definition.params.location);
                definition.runtime_memory = ice::meminfo_of<builtin::Stage_CreateFramebuffer::Runtime>;
                //definition.runtime_memory += ice::meminfo_of<GfxObject*> * params->input_count;
                break;
            }
            case GfxStageFlags::Create_Renderpass:
                definition.runtime_memory = ice::meminfo_of<builtin::Stage_CreateRenderpass::Runtime>;
                break;
            case GfxStageFlags::Create_Image:
                definition.runtime_memory = ice::meminfo_of<builtin::Stage_CreateImage::Runtime>;
                break;
            default:
                break;
            }
            ice::array::push_back(definitions, definition);
        }

        return ice::make_unique<ConstantGfxRenderGraph>(
            alloc,
            alloc,
            ice::move(definitions),
            rgdata,
            ice::move(data)
        );
    }

    auto create_rendergraph_builder(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::gfx::v2::GfxRenderGraphBuilder>
    {
        return ice::make_unique<SimpleGfxRenderGraphBuilder>(alloc, alloc);
    }

} // namespace ice::gfx::v2
