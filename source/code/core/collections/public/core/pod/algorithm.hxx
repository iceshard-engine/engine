#pragma once
#include <core/pod/array.hxx>
#include <algorithm>
#include <utility>

namespace core::pod
{
    namespace detail
    {

        //! A qsort implementation with tail call optimization (partial).
        //! \note Implementation base on articles:
        //!     http://www.algolist.net/Algorithms/Sorting/Quicksort
        //!     https://www.geeksforgeeks.org/quicksort-tail-call-optimization-reducing-worst-case-space-log-n/
        template<typename K, typename V, typename Pred>
        inline int qsort_partition(Array<K>& keys, Array<V>& values, Pred&& pred, int left, int right)
        {
            K* pivot = &keys[right];

            int i = left - 1;
            int j = left;

            while (j < right)
            {
                if (std::forward<Pred>(pred)(keys[j], *pivot))
                {
                    ++i;
                    std::swap(keys[i], keys[j]);
                    std::swap(values[i], values[j]);
                }
                ++j;
            }
            std::swap(keys[i + 1], keys[right]);
            std::swap(values[i + 1], values[right]);
            return i + 1;
        }

        //! A qsort implementation with tail call optimization (partial).
        //! \note Implementation base on articles:
        //!     http://www.algolist.net/Algorithms/Sorting/Quicksort
        //!     https://www.geeksforgeeks.org/quicksort-tail-call-optimization-reducing-worst-case-space-log-n/
        template<typename K, typename V, typename Pred>
        inline void qsort(Array<K>& keys, Array<V>& values, Pred&& pred, int left, int right)
        {
            while (left < right)
            {
                int pi = qsort_partition(keys, values, std::forward<Pred>(pred), left, right);

                qsort(keys, values, std::forward<Pred>(pred), left, pi - 1);

                left = pi + 1;
            }

            return;
        }

    } // namespace detail

    template<typename T>
    inline void sort(Array<T>& a)
    {
        std::sort(begin(a), end(a));
    }

    template<typename T, typename Pred>
    inline void sort(Array<T>& a, Pred&& pred)
    {
        std::sort(begin(a), end(a), std::forward<Pred>(pred));
    }

    //! qsort implementation for 2 arrays of the same length, where one is used in the sorting algorithm, and the second one is sorted accordingly.
    template<typename K, typename V, typename Pred>
    inline void sort(Array<K>& keys, Array<V>& values, Pred&& pred)
    {
        int first_index = 0;
        int last_index = pod::array::size(keys) - 1;

        detail::qsort(keys, values, std::forward<Pred>(pred), first_index, last_index);
    }

    template<typename T, typename Pred = T>
    inline bool contains(Array<T> const& values, Pred&& predicate) noexcept
    {
        if constexpr (std::is_function_v<Pred> || std::is_class_v<Pred>)
        {
            for (auto const& value : values)
            {
                if (std::forward<Pred>(predicate)(value))
                {
                    return true;
                }
            }
        }
        else
        {
            for (auto const& value : values)
            {
                if (value == predicate)
                {
                    return true;
                }
            }
        }
        return false;
    }

    template<typename T, typename Pred = T>
    inline auto remove_if(Array<T>& values, Pred&& predicate) noexcept -> uint32_t
    {
        if constexpr (std::is_function_v<Pred> || std::is_class_v<Pred>)
        {
            uint32_t beg = 0;
            uint32_t end = core::pod::array::size(values);

            while (beg < end)
            {
                if (std::forward<Pred>(predicate)(values[beg]))
                {
                    end -= 1;
                    values[beg] = values[end];
                }
                else
                {
                    beg += 1;
                }
            }

            return end;
        }
        else
        {
            uint32_t beg = 0;
            uint32_t end = core::pod::array::size(values);

            while (beg < end)
            {
                if (values[beg] == predicate)
                {
                    end -= 1;
                    values[beg] = values[end];
                }
                else
                {
                    beg += 1;
                }
            }

            return end;
        }
    }

} // namespace core::pod
