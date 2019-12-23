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
        virtual ~EngineModule() noexcept = default;

        //! \brief Returns the engine object from the loaded module.
        [[nodiscard]]
        virtual auto engine() noexcept -> Engine* = 0;

        //! \brief Returns the engine object from the loaded module.
        [[nodiscard]]
        virtual auto engine() const noexcept -> const Engine* = 0;
    };


    //! \brief Tries to load the engine module from the given library path.
    [[nodiscard]]
    auto load_engine_module(
        core::allocator& alloc,
        core::StringView<> path,
        resource::ResourceSystem& resources
    ) noexcept -> core::memory::unique_pointer<EngineModule>;


} // namespace iceshard
