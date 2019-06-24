#pragma once
#include <type_traits>

namespace mooned
{

template <class Tag, typename T>
class strong_typedef
{
public:
    strong_typedef()
        : _value{}
    {
    }

    explicit strong_typedef(const T& value) : _value{ value }
    {
    }

    explicit strong_typedef(T&& value) noexcept(std::is_nothrow_move_constructible<T>::value)
        : _value{ std::move(value) }
    {
    }

    explicit operator T&() noexcept
    {
        return _value;
    }

    explicit operator const T&() const noexcept
    {
        return _value;
    }

    friend void swap(strong_typedef& a, strong_typedef& b) noexcept
    {
        std::swap(static_cast<T&>(a), static_cast<T&>(b));
    }

private:
    T _value;
};

template <class Tag, typename T>
struct strong_numeric_typedef
{
    static_assert(std::is_integral_v<T>, "Not an integral type!");

    strong_numeric_typedef()
        : _value{}
    {
    }

    strong_numeric_typedef(const strong_numeric_typedef& b) 
        : _value{ b._value }
    {
    }

    strong_numeric_typedef(strong_numeric_typedef&& b) noexcept(std::is_nothrow_move_constructible<T>::value)
        : _value{ std::move(b._value) }
    {
    }

    explicit strong_numeric_typedef(const T& value) : _value{ value }
    {
    }

    explicit strong_numeric_typedef(T&& value) noexcept(std::is_nothrow_move_constructible<T>::value)
        : _value{ std::move(value) }
    {
    }

    explicit operator T&() noexcept
    {
        return _value;
    }

    explicit operator const T&() const noexcept
    {
        return _value;
    }

    bool operator==(const strong_numeric_typedef& b) const noexcept
    {
        return _value == b._value;
    }

    bool operator==(const T& b) const noexcept
    {
        return _value == b;
    }

    bool operator!=(const strong_numeric_typedef& b) const noexcept
    {
        return !(*this == b);
    }

    bool operator!=(const T& b) const noexcept
    {
        return !(*this == b);
    }

    strong_numeric_typedef& operator=(const strong_numeric_typedef& b)
    {
        if (this == std::addressof(b)) return *this;
        _value = b._value;
        return *this;
    }

    strong_numeric_typedef& operator=(const T& b)
    {
        _value = b;
        return *this;
    }

    strong_numeric_typedef& operator=(strong_numeric_typedef&& b)
    {
        if (this == std::addressof(b)) return *this;
        std::swap(_value, b._value);
        return *this;
    }

    strong_numeric_typedef& operator=(T&& b)
    {
        std::swap(_value, b);
        return *this;
    }

    friend void swap(strong_numeric_typedef& a, strong_numeric_typedef& b) noexcept
    {
        std::swap(static_cast<T&>(a), static_cast<T&>(b));
    }

    T _value;
};

}
