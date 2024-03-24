/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <ice/build/build.hxx>
#include <ice/error_codes.hxx>
#include <type_traits>

namespace ice
{

    template<typename Value, typename ErrorType = ice::ErrorCode>
    class Expected
    {
    public:
        Expected() noexcept
            : _state{ 0 }
        { }

        Expected(ErrorType error) noexcept
            : _state{ 2 }
        {
            ICE_ASSERT_CORE(error == false); // Errors should not be successes.
            _error = error;
        }

        Expected(Value&& value) noexcept
            : _state{ 1 }
        {
            new (&_value[0]) Value { ice::forward<Value>(value) };
        }

        ~Expected() noexcept
        {
            if (_state == 1)
            {
                reinterpret_cast<Value*>(_value)->~Value();
            }
        }

        bool valid() const noexcept { return _state != 0; }

        auto value() const & noexcept -> Value const&
        {
            ICE_ASSERT_CORE(_state == 1);
            return *reinterpret_cast<Value*>(_value);
        }

        auto value() && noexcept -> Value&&
        {
            ICE_ASSERT_CORE(_state == 1);
            return ice::move(*reinterpret_cast<Value*>(_value));
        }

        auto error() const noexcept -> ErrorType
        {
            ICE_ASSERT_CORE(_state == 2);
            return _error;
        }

        inline bool operator==(ErrorType error) const noexcept
        {
            return _state == 2 && _error == error;
        }

        inline operator bool() const noexcept
        {
            return _state == 1;
        }

    private:
        union
        {
            alignas(Value) char _value[sizeof(Value)];
            ErrorType _error;
        };

        ice::u8 _state;
    };

    template<>
    class Expected<ice::ErrorCode, ice::ErrorCode>
    {
    public:
        Expected() noexcept
            : _value{ ice::E_Error } // Unknown error if never set
        { }

        Expected(ice::ErrorCode error) noexcept
            : _value{ error }
        {
        }

        bool valid() const noexcept { return true; }

        auto value() const noexcept -> ice::ErrorCode
        {
            return _value;
        }

        auto error() const noexcept -> ice::ErrorCode
        {
            return _value;
        }

        template<typename ErrorType> requires(std::is_base_of_v<ice::ErrorCode, ErrorType>)
        inline bool operator==(ErrorType error) const noexcept
        {
            return _value == error;
        }

        inline operator bool() const noexcept
        {
            return _value == true;
        }

    private:
        ice::ErrorCode _value;
    };

    using Result = ice::Expected<ice::ErrorCode>;

} // namespace ice
