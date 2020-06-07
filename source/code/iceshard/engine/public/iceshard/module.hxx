#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <core/string_types.hxx>
#include <resource/resource_system.hxx>

namespace iceshard
{

    class Engine;

    //! \brief Describes the engine module.
    class EngineModule
    {
    public:
        using EnginePtr = core::memory::unique_pointer<
            Engine, core::memory::detail::memsys_custom_deleter<Engine>
        >;

        virtual ~EngineModule() noexcept = default;

        //! \brief Creates a new engine instance from the initialized module.
        //! \detailed This call will required some basic resources to be available and loaded
        //!     so the engine can properly initialize itself.
        [[nodiscard]]
        virtual auto create_instance(
            core::allocator& alloc,
            resource::ResourceSystem& resources
        ) noexcept -> EnginePtr = 0;

        virtual auto native_handle() noexcept -> core::ModuleHandle = 0;
    };


    //! \brief Tries to load the engine module from the given library path.
    [[nodiscard]]
    auto load_engine_module(
        core::allocator& alloc,
        core::StringView path
    ) noexcept -> core::memory::unique_pointer<EngineModule>;

} // namespace iceshard
