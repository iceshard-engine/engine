#include <ice/detail/refcounted.hxx>
#include <ice/assert.hxx>

namespace ice::detail
{

    RefCounted::RefCounted(ice::Allocator& alloc) noexcept
        : _allocator{ ice::addressof(alloc) }
    {
    }

    RefCounted::~RefCounted() noexcept
    {
        ICE_ASSERT(_strong <= 0, "Deleting an object that still has strong references!");
        ICE_ASSERT(_strong >= 0 && _weak >= 0, "Trying to deleat actual garbage or one of the counters broke!");
        ICE_ASSERT(_claim != ClaimMagic_Extracted, "Trying to delete an object that was extracted!");
    }

    void RefCounted::rc_sub(RCPassKey const& pass_key) noexcept
    {
        ICE_ASSERT_CORE(_strong > 0);
        if (_strong -= 1; _strong == 0)
        {
            _allocator->destroy(this);
        }
    }

    auto RefCounted::rc_stats(RCPassKey const& pass_key) const noexcept -> ice::RefCountStats
    {
        ICE_ASSERT(_strong > 0 && _weak >= 0, "Trying to access an deleted object or actual garbage!");
        return { .strong_refs = ice::u32(_strong), .weak_refs = ice::u32(_weak) };
    }

    bool RefCounted::rc_unclaimed(RCPassKey const& pass_key) const noexcept
    {
        ICE_ASSERT(_strong > 0 || (_strong == 0 && _weak == 1), "Trying to access invalid object!");
        return _strong == 0 && _weak == 1 && _claim == ClaimMagic_Unclaimed;
    }

    void RefCounted::rc_delete_extracted() noexcept
    {
        ICE_ASSERT(_strong == 0 && _weak == 0 && _claim == ClaimMagic_Extracted, "Trying to delete an object that was not extracted!");
        _allocator->destroy(this);
    }

    void RefCounted::rc_claim_internal() noexcept
    {
        ICE_ASSERT(_strong == 0 && _weak == 1 && _claim == ClaimMagic_Unclaimed, "Trying to access invalid object!");
        _claim -= ClaimMagic_Unclaimed; _strong += 1; _weak -= 1;
        ICE_ASSERT(_strong == 1 && _weak == 0 && _claim == ClaimMagic_Claimed, "Trying to access invalid object!");
    }

    void RefCounted::rc_extract_internal() noexcept
    {
        ICE_ASSERT_CORE(_strong >= 1 && _claim == ClaimMagic_Claimed);
        _claim = ClaimMagic_Extracted;
    }

    void RefCounted::rc_add_internal() noexcept
    {
        ICE_ASSERT_CORE(_strong < 255);
        _strong += 1;
    }

} // namespace ice
