#pragma once
#include <core/cexpr/stringid.hxx>

namespace iceshard
{

    struct Material;

    struct MaterialResources;

    class MaterialSystem
    {
    public:
        virtual ~MaterialSystem() noexcept = default;

        virtual bool create_material(core::stringid_arg_type name, Material const& definition) noexcept = 0;

        virtual bool get_material(core::stringid_arg_type name, MaterialResources& resources) noexcept = 0;
    };

} // namespace iceshard
