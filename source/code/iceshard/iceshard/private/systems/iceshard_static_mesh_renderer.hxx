#pragma once
#include <core/allocator.hxx>
#include <iceshard/renderer/render_system.hxx>
#include <iceshard/renderer/render_model.hxx>

#include <iceshard/render/render_stage.hxx>
#include <iceshard/component/component_archetype_index.hxx>
#include <iceshard/component/component_archetype_iterator.hxx>
#include <iceshard/ecs/model.hxx>
#include <iceshard/ecs/transform.hxx>
#include <iceshard/engine.hxx>
#include <iceshard/material/material.hxx>

#include <asset_system/asset_system.hxx>

#include "iceshard_mesh_loader.hxx"

namespace iceshard
{

    namespace detail
    {

        struct RenderInstance
        {
            uint32_t instance_offset;
            uint32_t instance_count;
            MaterialResources material;
            MeshInfo mesh_info;
            iceshard::renderer::v1::Mesh const* mesh;
        };

        struct InstanceData
        {
            ism::mat4x4 xform;
        };

        static_assert(sizeof(InstanceData) == sizeof(float) * 16);

    } // namespace tasks

    using Buffer = iceshard::renderer::api::Buffer;

    class IceshardStaticMeshRenderer final : public ComponentSystem, public RenderStageTaskFactory
    {
    public:
        static constexpr auto SystemName = "isc.system.static-mesh-renderer"_sid;

        IceshardStaticMeshRenderer(
            core::allocator& alloc,
            iceshard::Engine& engine,
            iceshard::ecs::ArchetypeIndex& index,
            iceshard::renderer::RenderSystem& render_system,
            iceshard::MeshLoader& mesh_loader,
            asset::AssetSystem& asset_system
        ) noexcept;

        void update(iceshard::Frame& frame) noexcept override;

        auto update_buffers_task(
            iceshard::renderer::api::CommandBuffer cmds,
            core::pod::Array<detail::RenderInstance> const* instances,
            core::pod::Array<detail::InstanceData> instance_data
        ) noexcept -> cppcoro::task<>;

        auto draw_task(
            iceshard::renderer::api::CommandBuffer cmds,
            core::pod::Array<detail::RenderInstance> const* instances
        ) noexcept -> cppcoro::task<>;

        auto render_task_factory() noexcept -> RenderStageTaskFactory* override
        {
            return this;
        }

        void create_render_tasks(
            iceshard::Frame const& current,
            iceshard::renderer::api::CommandBuffer cmds,
            core::Vector<cppcoro::task<>>& task_list
        ) noexcept override;

        ~IceshardStaticMeshRenderer() override;

    private:
        core::allocator& _allocator;
        iceshard::ecs::ArchetypeIndex& _archetype_index;
        iceshard::ecs::ComponentQuery<
            iceshard::component::Entity*,
            iceshard::component::ModelName*,
            iceshard::component::Material*,
            iceshard::component::Transform*
        > _component_query;

        iceshard::renderer::RenderSystem& _render_system;
        asset::AssetSystem& _asset_system;
        iceshard::MeshLoader& _mesh_loader;
        iceshard::MaterialSystem& _material_system;

        // Render resources
        ism::vec2u _viewport{ 1280, 720 };

        iceshard::renderer::api::Buffer _instance_buffers;
    };

} // namespace iceshard
