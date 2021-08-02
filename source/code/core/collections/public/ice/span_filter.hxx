#pragma once
#include <ice/span.hxx>
#include <coroutine>

namespace ice
{

    namespace detail { template<typename T> class SpanIterator; }

    template<typename T, typename Fn>
    auto filter_span(ice::Span<T> span, Fn&& fn) noexcept -> ice::detail::SpanIterator<T>;


    namespace detail
    {

        template<typename T>
        class SpanIterator
        {

            struct PromiseType
            {
                inline auto get_return_object() noexcept -> SpanIterator<T>;

                auto initial_suspend() const noexcept -> std::suspend_never { return { }; }
                auto final_suspend() const noexcept -> std::suspend_always { return { }; }

                template<typename T>
                auto yield_value(T& value) noexcept -> std::suspend_always
                {
                    _result = std::addressof(value);
                    return { };
                }
                auto return_void() const noexcept -> std::suspend_always
                {
                    return { };
                }

                void unhandled_exception() const noexcept
                {
                    std::abort();
                }

                auto value() const noexcept -> T&
                {
                    return *_result;
                }

            private:
                T* _result;
            };

            template<bool IsConst>
            class Iterator
            {
            public:
                Iterator(std::coroutine_handle<PromiseType> coro) noexcept;
                ~Iterator() noexcept = default;

                void advance() noexcept
                {
                    _coro.resume();
                }

                auto operator!=(Iterator const& other) const noexcept
                {
                    return (_coro != nullptr ? _coro : other._coro).done() == false;
                }

                auto operator++() noexcept -> Iterator&
                {
                    advance();
                    return *this;
                }

                inline auto operator*() const noexcept -> T const& requires(IsConst);

                inline auto operator*() noexcept -> T& requires(!IsConst);

            private:
                std::coroutine_handle<PromiseType> _coro;
            };

        public:
            SpanIterator(std::coroutine_handle<PromiseType> coro) noexcept
                : _coro{ coro }
            {
            }

            ~SpanIterator() noexcept
            {
                if (_coro != nullptr)
                {
                    _coro.destroy();
                }
            }

            SpanIterator(SpanIterator&& other) noexcept
                : _coro{ std::exchange(other._coro, nullptr) }
            {
            }

            SpanIterator(SpanIterator const&) noexcept = delete;

            auto begin() const noexcept -> Iterator<true>
            {
                return Iterator<true>{ _coro };
            }

            auto end() const noexcept -> Iterator<true>
            {
                return{ nullptr };
            }

            auto begin() noexcept -> Iterator<false>
            {
                return Iterator<false>{ _coro };
            }

            auto end() noexcept -> Iterator<false>
            {
                return{ nullptr };
            }

            using promise_type = PromiseType;

        private:
            std::coroutine_handle<PromiseType> _coro;
        };

        template<typename T>
        inline auto SpanIterator<T>::PromiseType::get_return_object() noexcept -> SpanIterator<T>
        {
            return SpanIterator<T>{ std::coroutine_handle<PromiseType>::from_promise(*this) };
        }

        template<typename T>
        template<bool IsConst>
        inline SpanIterator<T>::Iterator<IsConst>::Iterator(std::coroutine_handle<typename SpanIterator<T>::PromiseType> coro) noexcept
            : _coro{ coro }
        {
        }

        template<typename T>
        template<bool IsConst>
        inline auto SpanIterator<T>::Iterator<IsConst>::operator*() const noexcept -> T const& requires(IsConst)
        {
            return _coro.promise().value();
        }

        template<typename T>
        template<bool IsConst>
        inline auto SpanIterator<T>::Iterator<IsConst>::operator*() noexcept -> T& requires(!IsConst)
        {
            return _coro.promise().value();
        }

    } // namespace detail

    template<typename T, typename Fn>
    auto filter_span(ice::Span<T> span, Fn&& fn) noexcept -> ice::detail::SpanIterator<T>
    {
        for (auto& value : span)
        {
            if (std::forward<Fn>(fn)(value))
            {
                co_yield value;
            }
        }
    }

    template<typename T, typename Fn> requires(std::invocable<Fn, T const&>)
    auto operator|(ice::Span<T> span, Fn&& fn) noexcept
    {
        return filter_span(ice::move(span), ice::forward<Fn>(fn));
    }

} // namespace ice
