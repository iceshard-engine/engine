#include <iceshard/engine.hxx>
#include <core/allocators/proxy_allocator.hxx>

namespace iceshard
{

    class IceShardEngine final : public iceshard::Engine
    {
    public:
        IceShardEngine(core::allocator& alloc) noexcept
            : _allocator{ "iceshard-engine", alloc }
        { }

        ~IceShardEngine() noexcept = default;


        auto revision() const noexcept -> uint32_t override
        {
            return 1;
        }

    private:
        core::memory::proxy_allocator _allocator;
    };

}

extern "C"
{

    __declspec(dllexport) auto create_engine(core::allocator& alloc) -> iceshard::Engine*
    {
        return alloc.make<iceshard::IceShardEngine>(alloc);
    }

    __declspec(dllexport) void release_engine(core::allocator& alloc, iceshard::Engine* engine)
    {
        alloc.destroy(engine);
    }

}
