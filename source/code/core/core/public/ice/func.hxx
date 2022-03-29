#pragma once

namespace ice
{

    struct FnUserdata { void* value; };

    //! \brief A simple type to simplify catching and storing of function pointers.
    //! \note If the first function argument is of type 'FnUserdata' the function will also store a void* pointer and pass during a call.
    template<typename = void>
    struct Fn;


    //! \brief Returns a Fn object for the given function.
    template<typename R, typename... Args>
    auto bind_fn(R(*fn)(Args...) noexcept) noexcept -> ice::Fn<R(Args...) noexcept>;

    //! \brief Returns a Fn object for the given function and userdata.
    template<typename R, typename... Args>
    auto bind_fn(R(*fn)(ice::FnUserdata, Args...) noexcept, void* userdata) noexcept -> ice::Fn<R(ice::FnUserdata, Args...) noexcept>;


    //! \brief Function pointer WITHOUT any attached userdata.
    template<typename R, typename... Args>
    struct Fn<R(Args...) noexcept>
    {
        using Type = R(Args...) noexcept;

        Type* _function;

        bool is_set() const noexcept
        {
            return _function != nullptr;
        }

        auto operator()(Args... args) const noexcept -> R
        {
            return _function(std::forward<Args>(args)...);
        }
    };

    //! \brief Function pointer WITH attached userdata.
    template<typename R, typename... Args>
    struct Fn<R(ice::FnUserdata, Args...) noexcept>
    {
        using Type = R(void*, Args...) noexcept;

        Type* _function;
        ice::FnUserdata _userdata;

        bool is_set() const noexcept
        {
            return _function != nullptr;
        }

        auto operator()(Args... args) const noexcept -> R
        {
            return _function(_userdata.value, std::forward<Args>(args)...);
        }
    };

    template<typename R, typename... Args>
    auto bind_fn(R(*fn)(Args...) noexcept) noexcept -> ice::Fn<R(Args...) noexcept>
    {
        return ice::Fn<R(Args...) noexcept>{
            ._function = fn
        };
    }

    template<typename R, typename... Args>
    auto bind_fn(R(*fn)(ice::FnUserdata, Args...) noexcept, void* userdata) noexcept -> ice::Fn<R(ice::FnUserdata, Args...) noexcept>
    {
        return ice::Fn<R(ice::FnUserdata, Args...) noexcept>{
            ._function = fn,
            ._userdata = userdata
        };
    }

} // namespace ice
