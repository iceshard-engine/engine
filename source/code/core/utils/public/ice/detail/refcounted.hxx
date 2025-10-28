#pragma once
#include <ice/base.hxx>
#include <ice/mem_allocator.hxx>

namespace ice
{

    struct RefCountStats
    {
        ice::u32 strong_refs;
        ice::u32 weak_refs;
    };

    namespace detail
    {

        class RCPassKey;

    } // namespace detail

    namespace concepts
    {

        template<typename Object>
        concept IsRefCounted = requires(std::remove_const_t<Object>&o, ice::detail::RCPassKey const& passkey) {
            { o.rc_add(passkey) } -> std::convertible_to<std::remove_const_t<Object>*>;
            { o.rc_sub(passkey) } -> std::convertible_to<void>;
            { o.rc_stats(passkey) } -> std::convertible_to<ice::RefCountStats>;
            { o.rc_unclaimed(passkey) } -> std::convertible_to<bool>;
            { o.rc_claim(passkey) } -> std::convertible_to<std::remove_const_t<Object>*>;
        };

    } // namespace concepts

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    class Ptr;

    namespace detail
    {

        class RefCounted
        {
        public:
            RefCounted(ice::Allocator& alloc) noexcept;
            virtual ~RefCounted() noexcept;

            auto allocator() const noexcept -> ice::Allocator& { return *_allocator; }

            auto rc_add(this auto& self, RCPassKey const& pass_key) noexcept { return self.rc_add_internal(), ice::addressof(self); }
            void rc_sub(RCPassKey const& pass_key) noexcept;

            auto rc_stats(RCPassKey const& pass_key) const noexcept -> ice::RefCountStats;
            bool rc_unclaimed(RCPassKey const& pass_key) const noexcept;

            auto rc_claim(this auto& self, RCPassKey const& pass_key) noexcept { return self.rc_claim_internal(), ice::addressof(self); }
            auto rc_extract(this auto& self, RCPassKey const& pass_key) noexcept { return self.rc_extract_internal(), ice::addressof(self); }
            void rc_delete_extracted() noexcept;

        private:
            void rc_claim_internal() noexcept;
            void rc_extract_internal() noexcept;
            void rc_add_internal() noexcept;

        private:
            ice::Allocator* const _allocator = nullptr;

            // We start with a weak reference. This means that even if the object was created with an 'new' call,
            //  it will be considered 'dead'.
            ice::i16 _weak = 1;
            ice::i8 _strong = 0;

            static constexpr ice::i8 ClaimMagic_Claimed = 0;
            static constexpr ice::i8 ClaimMagic_Extracted = 42;
            static constexpr ice::i8 ClaimMagic_Unclaimed = 123;
            ice::i8 _claim = ClaimMagic_Unclaimed;
        };

        class RCPassKey
        {
            RCPassKey() noexcept = default;

            template<typename Object> requires ice::concepts::IsRefCounted<Object>
            friend class Ptr;
        };

    } // namespace detail

    using RefCounted = ice::detail::RefCounted;

} // namespace ice
