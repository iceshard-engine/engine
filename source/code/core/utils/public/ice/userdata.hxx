#pragma once
#include <ice/base.hxx>
#include <typeinfo>

namespace ice
{

    template<bool IsValidated = false>
    struct [[deprecated("Don't use for the time beeing!")]] UserdataBase
    {
        void* _pointer;

        UserdataBase(std::nullptr_t) noexcept
            : _pointer{ nullptr }
        {
        }

        template<typename T>
        UserdataBase(T* object) noexcept
            : _pointer{ object }
        {
        }

        auto operator=(std::nullptr_t) noexcept -> ice::UserdataBase<IsValidated>&
        {
            _pointer = nullptr;
            return *this;
        }

        template<typename T>
        auto operator=(T* object) noexcept -> ice::UserdataBase<IsValidated>&
        {
            _pointer = object;
            return *this;
        }

        template<typename T>
        inline auto as() noexcept -> T*
        {
            return reinterpret_cast<T*>(_pointer);
        }
    };

    template<>
    struct [[deprecated("Don't use for the time beeing!")]] UserdataBase<true>
    {
        void* _pointer;

        UserdataBase(std::nullptr_t) noexcept
            : _pointer{ nullptr }
            , _internal{ typeid(void).hash_code() }
        { }

        template<typename T>
        UserdataBase(T* object) noexcept
            : _pointer{ object }
            , _internal{ typeid(T).hash_code() }
        {
        }

        auto operator=(std::nullptr_t) noexcept -> ice::UserdataBase<true>&
        {
            _pointer = nullptr;
            _internal = typeid(void).hash_code();
            return *this;
        }

        template<typename T>
        auto operator=(T* object) noexcept -> ice::UserdataBase<true>&
        {
            _pointer = object;
            _internal = typeid(T).hash_code();
            return *this;
        }

        template<typename T>
        inline auto as() const noexcept -> T*
        {
            validate(typeid(T));
            return reinterpret_cast<T*>(_pointer);
        }

    private:
        ice::u64 _internal;

        void validate(std::type_info const& info) const noexcept;
    };

    using Userdata = UserdataBase<ice::build::is_debug || ice::build::is_develop>;

} // namespace ice
