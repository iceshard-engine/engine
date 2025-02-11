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
            , _error{ error }
        {
            if constexpr (std::is_same_v<ice::ErrorCode, ErrorType>)
            {
                ICE_ASSERT_CORE(error == false);
            }
        }

        template<typename OtherValue> requires (std::is_convertible_v<OtherValue, Value>)
        Expected(OtherValue&& value) noexcept
            : _state{ 1u }
            , _value{ ice::forward<OtherValue>(value) }
        {
        }

        Expected(Expected&& other) noexcept requires (std::is_nothrow_move_constructible_v<Value>)
            : _state{ other._state } // Don't exchange, we need to destroy the empty value in the old object
        {
            if (_state == 1u) // Already checking our state
            {
                new (ice::addressof(_value)) Value { ice::forward(other)._value };
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
                _value.~Value();
            }

            new (ice::addressof(_value)) Value { ice::forward<OtherValue>(value) };
            return *this;
        }

        auto operator=(ErrorType error) noexcept -> Expected&
        {
            if (std::exchange(_state, ice::u8(2u)) == 1u)
            {
                _value.~Value();
            }

            ICE_ASSERT_CORE(error == false); // Errors should not be successes.
            _error = error;
            return *this;
        }

        auto operator=(Expected&& other) noexcept -> Expected& requires (std::is_nothrow_move_assignable_v<Value>)
        {
            if (ice::addressof(other) != this)
            {
                if (_state == 1u && other._state == 1u) // Already checking our state
                {
                    _value = ice::move(other._value);
                }
                else if (other._state == 1u)
                {
                    _state = 1u;
                    new (ice::addressof(_value)) Value{ ice::move(other._value) };
                }
                else
                {
                    if (_state == 1u)
                    {
                        _value.~Value();
                    }

                    _error = other._error;
                    _state = 2u;
                }
            }
            return *this;
        }

        ~Expected() noexcept
        {
            if (_state == 1u)
            {
                _value.~Value();
            }
        }

        bool valid() const noexcept { return _state != 0u; }
        bool succeeded() const noexcept { return _state == 1; }
        bool failed() const noexcept { return _state == 2; }

        template<typename Self>
        auto value(this Self&& self) noexcept -> auto&&
        {
            ICE_ASSERT_CORE(self._state == 1u);
            return ice::forward<Self>(self)._value;
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

        inline operator Value&() & noexcept { return this->value(); }
        inline operator Value&&() && noexcept { return ice::move(*this).value(); }
        inline operator Value() const noexcept { return this->value(); }

    private:
        union
        {
            ErrorType _error;
            Value _value;
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
