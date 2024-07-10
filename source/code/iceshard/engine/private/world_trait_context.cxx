#include <ice/world/world_trait_context.hxx>

namespace ice
{

    TraitContext::TraitContext(ice::Trait* trait, ice::detail::TraitContextImpl* impl) noexcept
        : _trait{ trait }
        , _implementation{ impl }
    {
    }

    auto TraitContext::checkpoint(ice::StringID id) noexcept -> ice::TaskCheckpointGate
    {
        ICE_ASSERT_CORE(_implementation != nullptr);
        return _implementation->checkpoint(id);
    }

    bool TraitContext::register_checkpoint(ice::StringID id, ice::TaskCheckpoint& checkpoint) noexcept
    {
        if (_implementation == nullptr)
        {
            return ice::E_TraitContextNotInitialized;
        }

        return _implementation->register_checkpoint(id, checkpoint);
    }

    auto TraitContext::bind(ice::TraitTaskBinding const& binding) noexcept -> ice::Result
    {
        if (_implementation == nullptr)
        {
            return ice::E_TraitContextNotInitialized;
        }

        return _implementation->bind(_trait, binding);
    }

} // namespace ice
