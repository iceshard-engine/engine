#pragma once

namespace ice
{

    struct FnUserdata { void* value; };

    //! \brief A simple type to simplify catching and storing of function pointers.
    //! \note If the first function argument is of type 'FnUserdata' the function will also store a void* pointer and pass during a call.
    template<typename = void>
    struct Fn;


    //! \brief Function pointer WITHOUT any attached userdata.
    template<typename R, typename... Args>
    struct Fn<R(Args...) noexcept>
    {
        using Type = R(Args...) noexcept;

        Type* _function;

        constexpr bool is_set() const noexcept
        {
            return _function != nullptr;
        }

        constexpr auto operator()(Args... args) const noexcept -> R
        {
            return _function(std::forward<Args>(args)...);
        }

        Fn(Fn&&) noexcept = default;
        Fn(Fn const&) noexcept = default;

        auto operator=(Fn&&) noexcept -> Fn& = default;
        auto operator=(Fn const&) noexcept -> Fn& = default;

        Fn(Type* fn = nullptr) noexcept
            : _function{ fn }
        {
        }
    };

    //! \brief Function pointer WITH attached userdata.
    template<typename R, typename... Args>
    struct Fn<R(ice::FnUserdata, Args...) noexcept>
    {
        using Type = R(void*, Args...) noexcept;

        Type* _function;
        ice::FnUserdata _userdata;

        constexpr bool is_set() const noexcept
        {
            return _function != nullptr;
        }

        constexpr auto operator()(Args... args) const noexcept -> R
        {
            return _function(_userdata.value, std::forward<Args>(args)...);
        }

        Fn(Fn&&) noexcept = default;
        Fn(Fn const&) noexcept = default;

        auto operator=(Fn&&) noexcept -> Fn& = default;
        auto operator=(Fn const&) noexcept -> Fn& = default;

        Fn(Type* fn = nullptr, void* userdata = nullptr) noexcept
            : _function{ fn }
            , _userdata{ userdata }
        {
        }
    };

} // namespace ice
