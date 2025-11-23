/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/detail/refcounted.hxx>

namespace ice
{

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    class Ptr
    {
        using RCPassKey = ice::detail::RCPassKey;
    public:
        ~Ptr() noexcept;
        explicit Ptr(std::nullptr_t = nullptr) noexcept;
        explicit Ptr(Object* object_ptr) noexcept;

        Ptr(Ptr const& other) noexcept;
        auto operator=(Ptr const& other) noexcept -> Ptr&;

        Ptr(Ptr&& other) noexcept;
        auto operator=(Ptr&& other) noexcept -> Ptr&;

        template<typename Derived> requires std::is_base_of_v<Object, Derived>
        Ptr(Ptr<Derived> const& other) noexcept;
        template<typename Derived> requires std::is_base_of_v<Object, Derived>
        auto operator=(Ptr<Derived> const& other) noexcept -> Ptr&;

        template<typename Self>
        [[nodiscard]]
        auto raw_ptr(this Self& self) noexcept -> Object* { return self._ptr; }

        template<typename Self>
        auto operator->(this Self& self) noexcept { return self.raw_ptr(); }

        template<typename Self>
        auto operator==(this Self const& self, std::nullptr_t) noexcept -> bool { return self.raw_ptr() == nullptr; }

    private:
        Object* _ptr;
    };

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    class Ptr<Object const>
    {
        using RCPassKey = ice::detail::RCPassKey;
    public:
        ~Ptr() noexcept;
        explicit Ptr(std::nullptr_t = nullptr) noexcept;

        Ptr(Ptr const& other) noexcept;
        auto operator=(Ptr const& other) noexcept -> Ptr&;
        Ptr(Ptr<Object> const& other) noexcept;
        auto operator=(Ptr<Object> const& other) noexcept -> Ptr&;

        Ptr(Ptr&& other) noexcept;
        auto operator=(Ptr&& other) noexcept -> Ptr&;
        Ptr(Ptr<Object>&& other) noexcept;
        auto operator=(Ptr<Object>&& other) noexcept -> Ptr&;

        template<typename Derived> requires std::is_base_of_v<Object, Derived>
        auto operator=(Ptr<Derived const> const& other) noexcept -> Ptr&;

        [[nodiscard]]
        auto raw_ptr() const noexcept -> Object const* { return _ptr; }

        auto operator->() const noexcept -> Object const* { return raw_ptr(); }

        auto operator==(std::nullptr_t) const noexcept -> bool { return raw_ptr() == nullptr; }

    private:
        Object* _ptr;
    };

    ///

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    inline Ptr<Object>::~Ptr() noexcept
    {
        if (_ptr != nullptr)
        {
            _ptr->rc_sub(RCPassKey{});
        }
    }

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    inline Ptr<Object>::Ptr(std::nullptr_t) noexcept
        : _ptr{ nullptr }
    {
    }

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    inline Ptr<Object>::Ptr(Object* object_ptr) noexcept
        : _ptr{ object_ptr->rc_claim(RCPassKey{}) }
    {
    }

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    inline Ptr<Object>::Ptr(Ptr const& other) noexcept
        : _ptr{ other->rc_add(RCPassKey{}) }
    {
    }

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    inline auto Ptr<Object>::operator=(Ptr const& other) noexcept -> Ptr&
    {
        if (this != ice::addressof(other))
        {
            if (Object* prev = ice::exchange(_ptr, nullptr); prev != nullptr)
            {
                prev->rc_sub(RCPassKey{});
            }

            _ptr = other->rc_add(RCPassKey{});
            ICE_ASSERT_CORE(_ptr == nullptr || _ptr->rc_unclaimed(RCPassKey{}) == false);
        }
        return *this;
    }

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    inline Ptr<Object>::Ptr(Ptr&& other) noexcept
        : _ptr{ std::exchange(other._ptr, nullptr) }
    {
        ICE_ASSERT_CORE(_ptr == nullptr || _ptr->rc_unclaimed(RCPassKey{}) == false);
    }

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    inline auto Ptr<Object>::operator=(Ptr&& other) noexcept -> Ptr&
    {
        if (this != ice::addressof(other))
        {
            if (Object* prev = ice::exchange(_ptr, nullptr); prev != nullptr)
            {
                prev->rc_sub(RCPassKey{});
            }

            _ptr = ice::exchange(other._ptr, nullptr);
            ICE_ASSERT_CORE(_ptr == nullptr || _ptr->rc_unclaimed(RCPassKey{}) == false);
        }
        return *this;
    }

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    template<typename Derived> requires std::is_base_of_v<Object, Derived>
    inline Ptr<Object>::Ptr(Ptr<Derived> const& other) noexcept
        : _ptr{ other.raw_ptr()->rc_add(RCPassKey{}) }
    {
        ICE_ASSERT_CORE(_ptr == nullptr || _ptr->rc_unclaimed(RCPassKey{}) == false);
    }

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    template<typename Derived> requires std::is_base_of_v<Object, Derived>
    inline auto Ptr<Object>::operator=(Ptr<Derived> const& other) noexcept -> Ptr&
    {
        if (Object* prev = ice::exchange(_ptr, nullptr); prev != nullptr)
        {
            prev->rc_sub(RCPassKey{});
        }

        _ptr = other.raw_ptr()->rc_add(RCPassKey{});
        ICE_ASSERT_CORE(_ptr == nullptr || _ptr->rc_unclaimed(RCPassKey{}) == false);
        return *this;
    }

    ///

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    inline Ptr<Object const>::~Ptr() noexcept
    {
        if (_ptr != nullptr)
        {
            _ptr->rc_sub(RCPassKey{});
        }
    }

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    inline Ptr<Object const>::Ptr(std::nullptr_t) noexcept
        : _ptr{ nullptr }
    {
    }

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    inline Ptr<Object const>::Ptr(Ptr const& other) noexcept
        : _ptr{ other._ptr->rc_add(RCPassKey{}) }
    {
    }

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    inline auto Ptr<Object const>::operator=(Ptr const& other) noexcept -> Ptr&
    {
        if (this != ice::addressof(other))
        {
            if (Object* prev = ice::exchange(_ptr, nullptr); prev != nullptr)
            {
                prev->rc_sub(RCPassKey{});
            }

            _ptr = other._ptr->rc_add(RCPassKey{});
            ICE_ASSERT_CORE(_ptr == nullptr || _ptr->rc_unclaimed(RCPassKey{}) == false);
        }
        return *this;
    }

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    inline Ptr<Object const>::Ptr(Ptr<Object> const& other) noexcept
        : _ptr{ other->rc_add(RCPassKey{}) }
    {
    }

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    inline auto Ptr<Object const>::operator=(Ptr<Object> const& other) noexcept -> Ptr&
    {
        if (Object* prev = ice::exchange(_ptr, nullptr); prev != nullptr)
        {
            prev->rc_sub(RCPassKey{});
        }

        _ptr = other->rc_add(RCPassKey{});
        ICE_ASSERT_CORE(_ptr == nullptr || _ptr->rc_unclaimed(RCPassKey{}) == false);
        return *this;
    }

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    inline Ptr<Object const>::Ptr(Ptr&& other) noexcept
        : _ptr{ std::exchange(other._ptr, nullptr) }
    {
        ICE_ASSERT_CORE(_ptr == nullptr || _ptr->rc_unclaimed(RCPassKey{}) == false);
    }

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    inline auto Ptr<Object const>::operator=(Ptr&& other) noexcept -> Ptr&
    {
        if (this != ice::addressof(other))
        {
            if (Object* prev = ice::exchange(_ptr, nullptr); prev != nullptr)
            {
                prev->rc_sub(RCPassKey{});
            }

            _ptr = ice::exchange(other._ptr, nullptr);
            ICE_ASSERT_CORE(_ptr == nullptr || _ptr->rc_unclaimed(RCPassKey{}) == false);
        }
        return *this;
    }

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    inline Ptr<Object const>::Ptr(Ptr<Object>&& other) noexcept
        : _ptr{ std::exchange(other._ptr, nullptr) }
    {
        ICE_ASSERT_CORE(_ptr == nullptr || _ptr->rc_unclaimed(RCPassKey{}) == false);
    }

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    inline auto Ptr<Object const>::operator=(Ptr<Object>&& other) noexcept -> Ptr&
    {
        if (this != ice::addressof(other))
        {
            if (Object* prev = ice::exchange(_ptr, nullptr); prev != nullptr)
            {
                prev->rc_sub(RCPassKey{});
            }

            _ptr = ice::exchange(other._ptr, nullptr);
            ICE_ASSERT_CORE(_ptr == nullptr || _ptr->rc_unclaimed(RCPassKey{}) == false);
        }
        return *this;
    }

    template<typename Object> requires ice::concepts::IsRefCounted<Object>
    template<typename Derived> requires std::is_base_of_v<Object, Derived>
    inline auto Ptr<Object const>::operator=(Ptr<Derived const> const& other) noexcept -> Ptr&
    {
        if (Object* prev = ice::exchange(_ptr, nullptr); prev != nullptr)
        {
            prev->rc_sub(RCPassKey{});
        }

        _ptr = other.raw_ptr()->rc_add(RCPassKey{});
        ICE_ASSERT_CORE(_ptr == nullptr || _ptr->rc_unclaimed(RCPassKey{}) == false);
        return *this;
    }

} // namespace ice
