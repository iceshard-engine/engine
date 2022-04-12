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

        Fn(Fn&&) noexcept = default;
        Fn(Fn const&) noexcept = default;
        Fn(R(*fn)(Args...) noexcept = nullptr) noexcept;

        auto operator=(Fn&&) noexcept -> Fn& = default;
        auto operator=(Fn const&) noexcept -> Fn& = default;

        constexpr auto operator()(Args... args) const noexcept -> R;
        constexpr bool is_set() const noexcept;

        Type* _function;
    };

    //! \brief Function pointer WITH attached userdata.
    template<typename R, typename... Args>
    struct Fn<R(ice::FnUserdata, Args...) noexcept>
    {
        using Type = R(void*, Args...) noexcept;

        Fn(Fn&&) noexcept = default;
        Fn(Fn const&) noexcept = default;
        Fn(R(*fn)(void*, Args...) noexcept = nullptr, void* userdata = nullptr) noexcept;

        auto operator=(Fn&&) noexcept -> Fn& = default;
        auto operator=(Fn const&) noexcept -> Fn& = default;

        constexpr auto operator()(Args... args) const noexcept -> R;
        constexpr bool is_set() const noexcept;

        Type* _function;
        ice::FnUserdata _userdata;
    };

    template<typename R, typename... Args>
    Fn<R(Args...)noexcept>::Fn(R(*fn)(Args...) noexcept) noexcept
        : _function{ fn }
    {
    }

    template<typename R, typename... Args>
    constexpr auto Fn<R(Args...)noexcept>::operator()(Args... args) const noexcept -> R
    {
        return _function(std::forward<Args>(args)...);
    }

    template<typename R, typename... Args>
    inline constexpr bool Fn<R(Args...)noexcept>::is_set() const noexcept
    {
        return _function != nullptr;
    }

    template<typename R, typename... Args>
    Fn<R(ice::FnUserdata, Args...)noexcept>::Fn(R(*fn)(void*, Args...) noexcept, void* userdata) noexcept
        : _function{ fn }
        , _userdata{ userdata }
    {
    }

    template<typename R, typename... Args>
    constexpr auto Fn<R(ice::FnUserdata, Args...)noexcept>::operator()(Args... args) const noexcept -> R
    {
        return _function(_userdata.value, std::forward<Args>(args)...);
    }

    template<typename R, typename... Args>
    inline constexpr bool Fn<R(ice::FnUserdata, Args...)noexcept>::is_set() const noexcept
    {
        return _function != nullptr;
    }

} // namespace ice
