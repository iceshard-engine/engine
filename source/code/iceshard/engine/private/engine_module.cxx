#include <ice/engine_module.hxx>
#include <ice/engine.hxx>

namespace ice
{

    auto create_engine(
        ice::Allocator& alloc,
        ice::ModuleRegister& registry,
        ice::EngineCreateInfo const& create_info
    ) noexcept -> ice::UniquePtr<ice::Engine>
    {
        ice::UniquePtr<ice::Engine> result = ice::make_unique_null<ice::Engine>();

        ice::detail::engine::v1::EngineAPI* engine_api;
        if (registry.find_module_api("iceshard.engine"_sid, 1, reinterpret_cast<void**>(&engine_api)))
        {
            ice::Engine* engine = engine_api->create_engine_fn(alloc, registry, create_info);
            result = ice::UniquePtr<ice::Engine>{ engine, { alloc, engine_api->destroy_engine_fn } };
        }

        return result;
    }

} // namespace ice
