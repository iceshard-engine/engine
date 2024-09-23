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
            : _state{ 0u }
        { }

        Expected(ErrorType error) noexcept
            : _state{ 2u }
        {
            ICE_ASSERT_CORE(error == false); // Errors should not be successes.
            _error = error;
        }

        template<typename OtherValue> requires (std::is_convertible_v<OtherValue, Value>)
        Expected(OtherValue&& value) noexcept
            : _state{ 1u }
        {
            new (ice::addressof(_value)) Value { ice::forward<Value>(value) };
        }

        Expected(Expected&& other) noexcept requires (std::is_nothrow_move_constructible_v<Value>)
            : _state{ other._state } // Don't exchange, we need to destroy the empty value in the old object
        {
            if (_state == 1u) // Already checking our state
            {
                new (ice::addressof(_value)) Value { ice::move(other).value() };
            }
            else
            {
                _error = other._error;
            }
        }

        template<typename OtherValue> requires (std::is_convertible_v<OtherValue, Value>)
        auto operator=(OtherValue&& value) noexcept -> Expected& requires (std::is_nothrow_move_assignable_v<Value>)
        {
            if (std::exchange(_state, ice::u8(1u)) == 1u)
            {
                reinterpret_cast<Value*>(ice::addressof(_value))->~Value();
            }

            new (ice::addressof(_value)) Value { ice::forward<OtherValue>(value) };
            return *this;
        }

        auto operator=(ErrorType error) noexcept -> Expected&
        {
            if (std::exchange(_state, ice::u8(2u)) == 1u)
            {
                reinterpret_cast<Value*>(ice::addressof(_value))->~Value();
            }

            ICE_ASSERT_CORE(error == false); // Errors should not be successes.
            _error = error;
            return *this;
        }

        auto operator=(Expected&& other) noexcept -> Expected& requires (std::is_nothrow_move_assignable_v<Value>)
        {
            if (ice::addressof(other) != this)
            {
                _state = other._state;
                if (_state == 1u) // Already checking our state
                {
                    this->value() = ice::move(other.value());
                }
                else
                {
                    _error = other._error;
                }
            }
            return *this;
        }

        ~Expected() noexcept
        {
            if (_state == 1u)
            {
                reinterpret_cast<Value*>(ice::addressof(_value))->~Value();
            }
        }

        bool valid() const noexcept { return _state != 0u; }
        bool succeeded() const noexcept { return _state == 1; }
        bool failed() const noexcept { return _state == 2; }

        auto value() const & noexcept -> Value const&
        {
            ICE_ASSERT_CORE(_state == 1u);
            return *reinterpret_cast<Value const*>(ice::addressof(_value));
        }

        auto value() && noexcept -> Value&&
        {
            ICE_ASSERT_CORE(_state == 1u);
            return ice::move(*reinterpret_cast<Value*>(ice::addressof(_value)));
        }

        auto error() const noexcept -> ErrorType
        {
            ICE_ASSERT_CORE(_state == 2u);
            return _error;
        }

        inline bool operator==(ErrorType error) const noexcept
        {
            return _state == 2u && _error == error;
        }

        inline explicit operator bool() const noexcept
        {
            return _state == 1u;
        }

#if 0
        inline operator Value const&() const noexcept
            requires (std::is_same_v<Value, bool> == false)
        {
            return value();
        }
#endif

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

        Expected(bool issuccess) noexcept
            : _value{ issuccess ? ErrorCode{S_Ok} : ErrorCode{E_Fail} }
        {
        }

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
