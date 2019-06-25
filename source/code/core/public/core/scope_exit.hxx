#pragma once

namespace core
{


//! \brief A object which calls the a given lambda function when scope ends.
template<class T>
class scope_guard
{
public:
    //! \brief A new scope guard from the given callback.
    scope_guard(T&& func) noexcept
        : _func{ std::move(func) }
    { }

    ~scope_guard() noexcept
    {
        _func();
    }

private:
    T _func;
};

//! \brief A deduction guide for the scope guard to accept any lambda object.
template<class Func>
scope_guard(Func) -> scope_guard<Func>;


} // namespace core::detail
