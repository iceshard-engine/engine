#include "iceshard_static_mesh_renderer.hxx"

#include <core/pod/hash.hxx>
#include <core/pod/algorithm.hxx>

#include <core/message/buffer.hxx>
#include <core/message/operations.hxx>

#include <input_system/message/window.hxx>

#include <iceshard/engine.hxx>
#include <iceshard/frame.hxx>

#include <asset_system/assets/asset_shader.hxx>
#include <iceshard/renderer/render_pass.hxx>
#include <iceshard/renderer/render_pipeline.hxx>
#include <iceshard/renderer/render_resources.hxx>
#include <iceshard/renderer/render_commands.hxx>
#include <iceshard/renderer/render_buffers.hxx>
#include <iceshard/material/material_system.hxx>

#include <asset_system/assets/asset_mesh.hxx>

namespace iceshard
{

    using BufferType = iceshard::renderer::api::BufferType;
    using DataView = iceshard::renderer::api::DataView;
    using Model = iceshard::renderer::data::Model;

    IceshardStaticMeshRenderer::IceshardStaticMeshRenderer(
        core::allocator& alloc,
        iceshard::Engine& engine,
        iceshard::ecs::ArchetypeIndex& index,
        iceshard::renderer::RenderSystem& render_system,
        iceshard::MeshLoader& mesh_loader,
        asset::AssetSystem& asset_system
    ) noexcept
        : _allocator{ alloc }
        , _archetype_index{ index }
        , _component_query{ _allocator }
        , _render_system{ render_system }
        , _asset_system{ asset_system }
        , _mesh_loader{ mesh_loader }
        , _material_system{ engine.material_system() }
    {
        _instance_buffers = renderer::create_buffer(renderer::api::BufferType::VertexBuffer, 1024 * 32);
    }

    IceshardStaticMeshRenderer::~IceshardStaticMeshRenderer()
    {
    }

    void IceshardStaticMeshRenderer::update(iceshard::Frame& frame) noexcept
    {
        using asset::Asset;
        using asset::AssetData;
        using asset::AssetType;
        using asset::AssetStatus;

        using iceshard::component::ModelName;
        using iceshard::component::Material;
        using iceshard::component::Transform;
        using iceshard::component::Entity;

        using iceshard::renderer::data::Mesh;
        using iceshard::renderer::data::Model;

        auto& frame_alloc = frame.frame_allocator();

        core::message::filter<::input::message::WindowSizeChanged>(frame.messages(), [this](auto const& msg) noexcept
            {
                fmt::print("Window size changed to: {}x{}\n", msg.width, msg.height);
                _viewport = { msg.width, msg.height };
            });

        // Render data
        core::pod::Hash<Model const*> models{ _allocator };

        struct ModelInfo
        {
            uint64_t model_name;
            core::stringid_hash_type material_name;
        };

        core::pod::Array<ModelInfo> instance_models{ _allocator };
        core::pod::Array<detail::InstanceData> transforms{ _allocator };

        // Get all asset models for rendering
        iceshard::ecs::for_each_entity(
            iceshard::ecs::query_index(_component_query, _archetype_index),
            [&](Entity* e, ModelName* static_model, Material* material, Transform* transform) noexcept
            {
                AssetData model_data;
                if (AssetStatus::Loaded == _asset_system.load(Asset{ static_model->name, AssetType::Mesh }, model_data))
                {
                    auto const model_name = core::hash(static_model->name);
                    if (core::pod::hash::has(models, model_name) == false)
                    {
                        auto const* const model = reinterpret_cast<Model const*>(model_data.content.data());
                        core::pod::hash::set(models, model_name, model);
                    }

                    core::pod::array::push_back(transforms, detail::InstanceData{ transform->xform });
                    core::pod::array::push_back(instance_models,
                        ModelInfo{
                            .model_name = model_name,
                            .material_name = material->material_name.hash_value,
                        }
                    );
                }
            }
        );

        // Sort transforms and instance models (using instance models as the key)
        core::pod::sort(instance_models, transforms, [](ModelInfo left, ModelInfo right) noexcept
            {
                return left.model_name == right.model_name;
            });

        core::pod::array::push_back(instance_models, ModelInfo{ .model_name = core::hash(core::stringid_invalid) });

        // Build render instances
        auto* instance_data = frame.new_frame_object<core::pod::Array<detail::InstanceData>>("render-instances.data"_sid, frame_alloc);
        auto* instances = frame.new_frame_object<core::pod::Array<detail::RenderInstance>>("render-instances"_sid, frame_alloc);

        uint32_t total_instance_count = 0;
        uint32_t current_model_count = 0;
        uint32_t processed_model_count = 0;
        ModelInfo last_instance_model = core::pod::array::front(instance_models);
        detail::RenderInstance* last_instance = nullptr;

        std::for_each(core::pod::begin(instance_models), core::pod::end(instance_models), [&](ModelInfo model_info) noexcept
            {
                if (last_instance_model.model_name != model_info.model_name && current_model_count > 0)
                {
                    auto const* model_data = core::pod::hash::get(models, last_instance_model.model_name, nullptr);

                    auto const* it = model_data->mesh_list;
                    auto const* const end = model_data->mesh_list + model_data->mesh_count;

                    while (it != end)
                    {
                        using namespace core::math;
                        auto const instance_offset = total_instance_count;

                        for (uint32_t i = 0; i < current_model_count; ++i)
                        {
                            core::pod::array::push_back(*instance_data,
                                detail::InstanceData{
                                    .xform = transforms[processed_model_count + i].xform * it->local_xform,
                                }
                            );
                            total_instance_count += 1;
                        }

                        MaterialResources resources;
                        _material_system.get_material({ last_instance_model.material_name }, resources);

                        core::pod::array::push_back(
                            *instances,
                            detail::RenderInstance
                            {
                                .instance_offset = instance_offset,
                                .instance_count = current_model_count,
                                .material = resources,
                                .mesh_info = _mesh_loader.get_mesh(core::stringid_type{
                                    static_cast<core::stringid_hash_type>(last_instance_model.model_name)
                                }),
                                .mesh = it,
                            }
                        );

                        it += 1;
                    }

                    last_instance_model = model_info;
                    processed_model_count += current_model_count;
                    current_model_count = 0;
                }

                current_model_count += 1;
            });
    }

    void IceshardStaticMeshRenderer::create_render_tasks(
        iceshard::Frame const& current,
        iceshard::renderer::api::CommandBuffer cmds,
        core::Vector<cppcoro::task<>>& task_list
    ) noexcept
    {
        auto* instance_data = current.get_frame_object<core::pod::Array<detail::InstanceData>>("render-instances.data"_sid);
        auto* instances = current.get_frame_object<core::pod::Array<detail::RenderInstance>>("render-instances"_sid);

        if (instance_data && instances)
        {
            task_list.push_back(
                update_buffers_task(cmds, instances, std::move(*instance_data))
            );
            task_list.push_back(
                draw_task(cmds, instances)
            );
        }
    }

    auto IceshardStaticMeshRenderer::update_buffers_task(
        iceshard::renderer::api::CommandBuffer cmds,
        core::pod::Array<detail::RenderInstance> const* instances,
        core::pod::Array<detail::InstanceData> instance_data
    ) noexcept -> cppcoro::task<>
    {
        if (core::pod::array::empty(*instances))
        {
            co_return;
        }

        Buffer mapped_buffers[] = {
            _instance_buffers,
        };
        DataView mapped_buffer_views[core::size(mapped_buffers)];

        using namespace iceshard::renderer;

        auto buffers_arr = core::pod::array::create_view(mapped_buffers);
        auto views_arr = core::pod::array::create_view(mapped_buffer_views);

        map_buffers(buffers_arr, views_arr);

        memcpy(
            mapped_buffer_views[0].data,
            core::pod::begin(instance_data),
            core::pod::array::size(instance_data) * sizeof(detail::InstanceData)
        );

        unmap_buffers(buffers_arr);
        co_return;
    }

    auto IceshardStaticMeshRenderer::draw_task(
        iceshard::renderer::api::CommandBuffer cb,
        core::pod::Array<detail::RenderInstance> const* instances
    ) noexcept -> cppcoro::task<>
    {
        if (core::pod::array::empty(*instances))
        {
            co_return;
        }

        using iceshard::renderer::RenderPassStage;
        using namespace iceshard::renderer::commands;

        set_viewport(cb, _viewport.x, _viewport.y);
        set_scissor(cb, _viewport.x, _viewport.y);

        for (auto const& instance : *instances)
        {
            {
                Buffer buffers[] = {
                    instance.mesh_info.vertice_buffer,
                    _instance_buffers,
                };

                core::pod::Array<Buffer> buffer_view{ core::memory::globals::null_allocator() };
                core::pod::array::create_view(buffer_view, buffers, 2);

                bind_index_buffer(cb, instance.mesh_info.indice_buffer);
                bind_vertex_buffers(cb, buffer_view);
            }

            bind_pipeline(cb, instance.material.pipeline);

            iceshard::renderer::ResourceSet resource_sets[] = {
                instance.material.resource,
                _render_system.get_resource_set("static-mesh.3d"_sid)
            };
            bind_resource_sets(cb, core::pod::array::create_view(resource_sets));

            draw_indexed(cb,
                instance.mesh->indice_count,
                instance.instance_count,
                instance.mesh_info.indice_buffer_offset + instance.mesh->indice_offset,
                instance.mesh_info.vertice_buffer_offset + instance.mesh->vertice_offset, // * sizeof(core::math::vec3) * 2,
                instance.instance_offset
            );
        }

        co_return;
    }

} // namespace iceshard
