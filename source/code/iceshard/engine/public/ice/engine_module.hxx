#pragma once
#include <ice/allocator.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/module_register.hxx>

namespace ice
{

    class AssetStorage;

    class Engine;
    class EngineDevUI;
    class WorldTraitArchive;

    struct EngineCreateInfo
    {
        ice::AssetStorage& asset_storage;
        ice::WorldTraitArchive const& trait_archive;
        ice::EngineDevUI* devui;
    };

    auto create_engine(
        ice::Allocator& alloc,
        ice::ModuleRegister& registry,
        ice::EngineCreateInfo const& create_info
    ) noexcept -> ice::UniquePtr<ice::Engine>;

    namespace detail::engine::v1
    {

        using CreateFn = auto (
            ice::Allocator&,
            ice::ModuleRegister&,
            ice::EngineCreateInfo const&
        ) noexcept -> ice::Engine*;

        using DestroyFn = void (ice::Allocator& alloc, ice::Engine*) noexcept;

        struct EngineAPI
        {
            CreateFn* create_engine_fn;
            DestroyFn* destroy_engine_fn;
        };

    } // namespace detail::engine::v1

} // namespace ice
