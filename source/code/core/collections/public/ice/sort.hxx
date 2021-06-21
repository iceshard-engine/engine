#pragma once
#include <ice/base.hxx>
#include <ice/span.hxx>
#include <algorithm>

namespace ice
{

    template<typename T>
    inline void sort(ice::Span<T> span) noexcept;

    template<typename T, typename Pred>
    inline void sort(ice::Span<T> span, Pred&& pred) noexcept;

    template<typename K, typename V, typename Pred>
    inline void sort(ice::Span<K> keys, ice::Span<V> values, Pred&& pred) noexcept;

    namespace detail
    {

        template<typename K, typename V, typename Pred>
        inline auto qsort_partition(ice::Span<K> keys, ice::Span<V>& values, Pred&& pred, i32 left, i32 right) noexcept -> i32
        {
            K* pivot = &keys[right];

            i32 i = left - 1;
            i32 j = left;

            while (j < right)
            {
                if (ice::forward<Pred>(pred)(keys[j], *pivot))
                {
                    ++i;
                    ice::swap(keys[i], keys[j]);
                    ice::swap(values[i], values[j]);
                }
                ++j;
            }
            ice::swap(keys[i + 1], keys[right]);
            ice::swap(values[i + 1], values[right]);
            return i + 1;
        }

        //! A qsort implementation with tail call optimization (partial).
        //! \note Implementation base on articles:
        //!     http://www.algolist.net/Algorithms/Sorting/Quicksort
        //!     https://www.geeksforgeeks.org/quicksort-tail-call-optimization-reducing-worst-case-space-log-n/
        template<typename K, typename V, typename Pred>
        inline void qsort(ice::Span<K> keys, ice::Span<V> values, Pred&& pred, i32 left, i32 right) noexcept
        {
            while (left < right)
            {
                i32 pi = qsort_partition(keys, values, ice::forward<Pred>(pred), left, right);

                ice::detail::qsort(keys, values, ice::forward<Pred>(pred), left, pi - 1);

                left = pi + 1;
            }
        }

    } // namespace detail

    template<typename T>
    inline void sort(ice::Span<T> span) noexcept
    {
        std::sort(span.begin(), span.end());
    }

    template<typename T, typename Pred>
    inline void sort(ice::Span<T> span, Pred&& pred) noexcept
    {
        std::sort(span.begin(), span.end(), ice::forward<Pred>(pred));
    }

    template<typename K, typename V, typename Pred>
    inline void sort(ice::Span<K> keys, ice::Span<V> values, Pred&& pred) noexcept
    {
        i32 first_index = 0;
        i32 last_index = keys.size() - 1;

        ice::detail::qsort(keys, values, std::forward<Pred>(pred), first_index, last_index);
    }

} // namespace ice
