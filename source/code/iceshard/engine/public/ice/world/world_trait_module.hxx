#pragma once
#include <ice/world/world_trait_archive.hxx>

namespace ice
{

    class ModuleRegister;

    void load_trait_descriptions(
        ice::Allocator& alloc,
        ice::ModuleRegister const& registry,
        ice::WorldTraitArchive& trait_archive
    ) noexcept;

    namespace detail::world_traits::v1
    {

        static constexpr ice::StringID Constant_APIName_WorldTraitsModule = "ice.world-traits-module"_sid;

        using RegisterTraitsFn = bool(
            ice::WorldTraitArchive&
        ) noexcept;

        struct TraitsModuleAPI
        {
            RegisterTraitsFn* register_traits_fn;
        };

    } // namespace detail::engine::v1

    using ice::detail::world_traits::v1::Constant_APIName_WorldTraitsModule;

} // namespace ice
