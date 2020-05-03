#include "iceshard_static_mesh_renderer.hxx"

#include <core/pod/hash.hxx>
#include <core/pod/algorithm.hxx>

#include <core/message/buffer.hxx>
#include <core/message/operations.hxx>

#include <input_system/message/window.hxx>

#include <iceshard/engine.hxx>
#include <iceshard/frame.hxx>

#include <render_system/render_commands.hxx>
#include <iceshard/renderer/render_pass.hxx>
#include <iceshard/renderer/render_pipeline.hxx>
#include <iceshard/renderer/render_resources.hxx>

#include <asset_system/assets/asset_mesh.hxx>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

namespace iceshard
{

    using BufferType = iceshard::renderer::api::BufferType;
    using DataView = iceshard::renderer::api::DataView;
    using Model = iceshard::renderer::v1::Model;

    IceshardStaticMeshRenderer::IceshardStaticMeshRenderer(
        core::allocator& alloc,
        iceshard::Engine& engine,
        iceshard::ecs::ArchetypeIndex& index,
        iceshard::renderer::RenderSystem& render_system,
        asset::AssetSystem& asset_system
    ) noexcept
        : _allocator{ alloc }
        , _archetype_index{ index }
        , _component_query{ _allocator }
        , _render_system{ render_system }
        , _asset_system{ asset_system }
    {
        _indice_buffers = _render_system.create_data_buffer(iceshard::renderer::api::BufferType::IndexBuffer, 1024 * 32);
        _vertice_buffers = _render_system.create_data_buffer(iceshard::renderer::api::BufferType::VertexBuffer, 1024 * 128);
        _instance_buffers = _render_system.create_data_buffer(iceshard::renderer::api::BufferType::VertexBuffer, 1024 * 16);

        // First frame init
        engine.add_task([](IceshardStaticMeshRenderer* self, core::allocator& alloc) noexcept -> cppcoro::task<>
            {

                {
                    core::pod::Array<asset::AssetData> shader_assets{ alloc };
                    core::pod::array::resize(shader_assets, 2);

                    self->_asset_system.load(asset::AssetShader{ "shaders/debug/test-vert" }, shader_assets[0]);
                    self->_asset_system.load(asset::AssetShader{ "shaders/debug/test-frag" }, shader_assets[1]);

                    self->_render_pipeline = self->_render_system.create_pipeline(
                        "static-mesh.3d"_sid,
                        iceshard::renderer::RenderPipelineLayout::Default,
                        shader_assets
                    );
                }

                self->_render_resource_set = self->_render_system.get_resource_set(
                    "static-mesh.3d"_sid
                );

                co_return;
            }(this, engine.current_frame().frame_allocator()));
    }

    IceshardStaticMeshRenderer::~IceshardStaticMeshRenderer()
    {
        _render_system.destroy_pipeline("static-mesh.3d"_sid);
    }

    void IceshardStaticMeshRenderer::update(iceshard::Engine& engine) noexcept
    {
        using asset::Asset;
        using asset::AssetData;
        using asset::AssetType;
        using asset::AssetStatus;

        using iceshard::component::ModelName;
        using iceshard::component::ModelMaterial;
        using iceshard::component::Transform;
        using iceshard::component::Entity;

        using iceshard::renderer::v1::Mesh;
        using iceshard::renderer::v1::Model;

        auto& frame = engine.current_frame();
        auto& frame_alloc = frame.frame_allocator();

        core::message::filter<input::message::WindowSizeChanged>(frame.messages(), [this](auto const& msg) noexcept
            {
                fmt::print("Window size changed to: {}x{}\n", msg.width, msg.height);
                _viewport = { msg.width, msg.height };
            });

        // Render data
        core::pod::Hash<Model const*> models{ _allocator };
        core::pod::Array<uint64_t> instance_models{ _allocator };
        core::pod::Array<detail::InstanceData> transforms{ _allocator };

        // Get all asset models for rendering
        iceshard::ecs::for_each_entity(
            iceshard::ecs::query_index(_component_query, _archetype_index),
            [&](Entity* e, ModelName* static_model, ModelMaterial* color, Transform* transform) noexcept
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

                    core::pod::array::push_back(transforms, detail::InstanceData{ transform->xform, color->color });
                    core::pod::array::push_back(instance_models, model_name);
                }
            }
        );

        // Sort transforms and instance models (using instance models as the key)
        core::pod::sort(instance_models, transforms, [](uint64_t left, uint64_t right) noexcept
            {
                return left < right;
            });

        core::pod::array::push_back(instance_models, core::hash(core::stringid_invalid));

        // Build render instances
        auto* instance_data = frame.new_frame_object<core::pod::Array<detail::InstanceData>>("render-instances.data"_sid, frame_alloc);
        auto* instances = frame.new_frame_object<core::pod::Array<detail::RenderInstance>>("render-instances"_sid, frame_alloc);

        uint32_t total_indice_count = 0;
        uint32_t total_vertice_count = 0;
        uint32_t total_instance_count = 0;
        uint32_t current_model_count = 0;
        uint32_t processed_model_count = 0;
        uint64_t last_instance_model = core::pod::array::front(instance_models);
        detail::RenderInstance* last_instance = nullptr;

        std::for_each(core::pod::begin(instance_models), core::pod::end(instance_models), [&](uint64_t model) noexcept
            {
                if (last_instance_model != model && current_model_count > 0)
                {
                    auto const* model_data = core::pod::hash::get(models, last_instance_model, nullptr);

                    auto const* it = model_data->mesh_list;
                    auto const* const end = model_data->mesh_list + model_data->mesh_count;

                    auto const indice_offset = total_indice_count;
                    auto const vertice_offset = total_vertice_count;

                    while (it != end)
                    {
                        using namespace core::math;
                        auto const instance_offset = total_instance_count;

                        for (uint32_t i = 0; i < current_model_count; ++i)
                        {
                            core::pod::array::push_back(*instance_data,
                                detail::InstanceData{
                                    .xform = core::math::mul(it->local_xform, transforms[processed_model_count + i].xform),
                                    .color = transforms[processed_model_count + i].color
                                }
                            );
                            total_instance_count += 1;
                        }

                        core::pod::array::push_back(
                            *instances,
                            detail::RenderInstance
                            {
                                .indice_offset = indice_offset,
                                .vertice_offset = vertice_offset,
                                .instance_offset = instance_offset,
                                .instance_count = current_model_count,
                                .model = model_data,
                                .mesh = it,
                            }
                        );

                        total_indice_count += it->indice_count;
                        total_vertice_count += it->vertice_count;

                        it += 1;
                    }

                    last_instance_model = model;
                    processed_model_count += current_model_count;
                    current_model_count = 0;
                }

                current_model_count += 1;
            });

        // Load models into render buffers
        engine.add_task(
            update_buffers_task(instances, std::move(*instance_data))
        );

        // Draw all instances
        engine.add_task(
            draw_task(instances)
        );
    }

    auto IceshardStaticMeshRenderer::update_buffers_task(
        core::pod::Array<detail::RenderInstance> const* instances,
        core::pod::Array<detail::InstanceData> instance_data
    ) noexcept -> cppcoro::task<>
    {
        if (core::pod::array::empty(*instances))
        {
            co_return;
        }

        Buffer mapped_buffers[] = {
            _vertice_buffers,
            _instance_buffers,
            _indice_buffers,
        };
        DataView mapped_buffer_views[core::size(mapped_buffers)];

        iceshard::renderer::api::render_api_instance->buffer_array_map_data(
            mapped_buffers,
            mapped_buffer_views,
            core::size(mapped_buffers)
        );

        for (auto const& instance : *instances)
        {
            iceshard::renderer::v1::Model const* model = instance.model;

            void* const vertice_buffer_ptr = core::memory::utils::pointer_add(
                mapped_buffer_views[0].data,
                instance.vertice_offset * sizeof(core::math::vec3) * 2
            );
            void* const indice_buffer_ptr = core::memory::utils::pointer_add(
                mapped_buffer_views[2].data,
                instance.indice_offset * sizeof(core::math::u16)
            );

            memcpy(
                vertice_buffer_ptr,
                model->vertice_data,
                model->vertice_data_size
            );
            memcpy(
                indice_buffer_ptr,
                model->indice_data,
                model->indice_data_size
            );
        }

        memcpy(
            mapped_buffer_views[1].data,
            core::pod::begin(instance_data),
            core::pod::array::size(instance_data) * sizeof(detail::InstanceData)
        );

        co_return;
    }

    auto IceshardStaticMeshRenderer::draw_task(
        core::pod::Array<detail::RenderInstance> const* instances
    ) noexcept -> cppcoro::task<>
    {
        if (core::pod::array::empty(*instances))
        {
            co_return;
        }

        using iceshard::renderer::RenderPassStage;
        using namespace iceshard::renderer::commands;

        auto cb = _render_system.acquire_command_buffer(RenderPassStage::Geometry);

        bind_pipeline(cb, _render_pipeline);
        bind_resource_set(cb, _render_resource_set);
        set_viewport(cb, _viewport.x, _viewport.y);
        set_scissor(cb, _viewport.x, _viewport.y);

        {
            Buffer buffers[] = {
                _vertice_buffers,
                _instance_buffers,
            };

            core::pod::Array<Buffer> buffer_view{ core::memory::globals::null_allocator() };
            core::pod::array::create_view(buffer_view, buffers, 2);

            bind_index_buffer(cb, _indice_buffers);
            bind_vertex_buffers(cb, buffer_view);
        }

        for (auto const& instance : *instances)
        {
            draw_indexed(cb,
                instance.mesh->indice_count,
                instance.instance_count,
                instance.indice_offset + instance.mesh->indice_offset,
                instance.vertice_offset + instance.mesh->vertice_offset, // * sizeof(core::math::vec3) * 2,
                instance.instance_offset
            );
        }

        _render_system.submit_command_buffer(cb);

        //Model const* model = reinterpret_cast<Model const*>(_mesh_asset.content.data());

        //{
        //    DataView mapped_buffer_view;

        //    iceshard::renderer::api::render_api_instance->buffer_array_map_data(
        //        &_buffers[1],
        //        &mapped_buffer_view,
        //        1
        //    );

        //    glm::mat4* it = reinterpret_cast<glm::mat4*>(mapped_buffer_view.data);

        //    uint32_t model_count = 0;
        //    iceshard::ecs::for_each_entity(
        //        iceshard::ecs::query_index(_xform_query, _index),
        //        [&](Transform* tform) noexcept
        //        {
        //            *it = tform->xform;
        //            model_count += 1;
        //            it += 1;
        //        }
        //    );

        //    iceshard::renderer::api::render_api_instance->buffer_array_unmap_data(
        //        &_buffers[1],
        //        1
        //    );

        //    core::pod::Array<Buffer> buffer_view{ core::memory::globals::null_allocator() };
        //    core::pod::array::create_view(buffer_view, _buffers, 2);

        //    using namespace iceshard::renderer::commands;

        //    bind_index_buffer(cb, _buffers[2]);
        //    bind_vertex_buffers(cb, buffer_view);
        //    draw_indexed(cb, model->mesh_list->indice_count, model_count, 0, 0, 0);
        //}

        co_return;
    }

} // namespace iceshard
