#include <iceshard/engine.hxx>

#include <resource/system.hxx>
#include <input_system/system.hxx>
#include <input_system/module.hxx>

#include <core/allocators/proxy_allocator.hxx>

namespace iceshard
{

    class IceShardEngine final : public iceshard::Engine
    {
    public:
        IceShardEngine(core::allocator& alloc, resource::ResourceSystem& resources) noexcept
            : _allocator{ "iceshard-engine", alloc }
            , _resources{ resources }
            , _input_module{ nullptr, { _allocator } }
        {
            auto* sdl_driver_module_location = resources.find({ "sdl2_driver.dll" });
            IS_ASSERT(sdl_driver_module_location != nullptr, "Missing SDL2 driver module!");

            _input_module = input::load_driver_module(_allocator, sdl_driver_module_location->location().path);
        }

        ~IceShardEngine() noexcept = default;


        auto revision() const noexcept -> uint32_t override
        {
            return 1;
        }

        auto input_system() const noexcept->input::InputSystem*
        {
            return _input_module->input_system();
        }

    private:
        core::memory::proxy_allocator _allocator;

        //! \brief The provided resource system.
        resource::ResourceSystem& _resources;

        //! \brief The loaded input system.
        core::memory::unique_pointer<input::InputModule> _input_module;
    };

}

extern "C"
{

    __declspec(dllexport) auto create_engine(core::allocator& alloc, resource::ResourceSystem& resources) -> iceshard::Engine*
    {
        return alloc.make<iceshard::IceShardEngine>(alloc, resources);
    }

    __declspec(dllexport) void release_engine(core::allocator& alloc, iceshard::Engine* engine)
    {
        alloc.destroy(engine);
    }

}
