#pragma once
#include <core/allocator.hxx>
#include <iceshard/renderer/render_system.hxx>
#include <iceshard/renderer/render_model.hxx>

#include <iceshard/component/component_archetype_index.hxx>
#include <iceshard/component/component_archetype_iterator.hxx>
#include <iceshard/ecs/model.hxx>
#include <iceshard/ecs/transform.hxx>
#include <iceshard/engine.hxx>

#include <asset_system/asset_system.hxx>

namespace iceshard
{

    namespace detail
    {

        struct RenderInstance
        {
            uint32_t indice_offset;
            uint32_t vertice_offset;
            uint32_t instance_offset;
            uint32_t instance_count;
            iceshard::renderer::v1::Model const* model;
            iceshard::renderer::v1::Mesh const* mesh;
        };

        struct InstanceData
        {
            core::math::mat4x4 xform;
            core::math::vec4 color;
        };

        static_assert(sizeof(InstanceData) == sizeof(float) * 20);

    } // namespace tasks

    using Buffer = iceshard::renderer::api::Buffer;

    class IceshardStaticMeshRenderer final : public ComponentSystem
    {
    public:
        static constexpr auto SystemName = "isc.system.static-mesh-renderer"_sid;

        IceshardStaticMeshRenderer(
            core::allocator& alloc,
            iceshard::Engine& engine,
            iceshard::ecs::ArchetypeIndex& index,
            iceshard::renderer::RenderSystem& render_system,
            asset::AssetSystem& asset_system
        ) noexcept;

        void update(iceshard::Engine& engine) noexcept override;

        auto update_buffers_task(
            core::pod::Array<detail::RenderInstance> const* instances,
            core::pod::Array<detail::InstanceData> instance_data
        ) noexcept -> cppcoro::task<>;

        auto draw_task(
            core::pod::Array<detail::RenderInstance> const* instances
        ) noexcept -> cppcoro::task<>;

        ~IceshardStaticMeshRenderer() override;

    private:
        core::allocator& _allocator;
        iceshard::ecs::ArchetypeIndex& _archetype_index;
        iceshard::ecs::ComponentQuery<
            iceshard::component::Entity*,
            iceshard::component::ModelName*,
            iceshard::component::ModelMaterial*,
            iceshard::component::Transform*
        > _component_query;

        iceshard::renderer::RenderSystem& _render_system;
        asset::AssetSystem& _asset_system;

        // Render resources
        core::math::uvec2 _viewport{ 1280, 720 };

        iceshard::renderer::api::Buffer _indice_buffers;
        iceshard::renderer::api::Buffer _vertice_buffers;
        iceshard::renderer::api::Buffer _instance_buffers;

        iceshard::renderer::api::Pipeline _render_pipeline;
        iceshard::renderer::api::ResourceSet _render_resource_set;
    };

} // namespace iceshard
