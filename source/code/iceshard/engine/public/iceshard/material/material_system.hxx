#pragma once
#include <core/cexpr/stringid.hxx>

namespace iceshard
{

    namespace renderer
    {
        enum class RenderPipelineLayout : uint32_t;
    }

    struct Material;

    struct MaterialResources;

    class MaterialSystem
    {
    public:
        virtual ~MaterialSystem() noexcept = default;

        virtual bool create_material(
            core::stringid_arg_type name, 
            Material const& definition,
            iceshard::renderer::RenderPipelineLayout layout
        ) noexcept = 0;

        virtual bool get_material(core::stringid_arg_type name, MaterialResources& resources) noexcept = 0;
    };

} // namespace iceshard
