/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <ice/span.hxx>
#include <algorithm>

namespace ice
{

    template<typename Type, typename Fn>
    inline auto accumulate_over(ice::Span<Type> objects, Fn&& fn) noexcept -> ice::u32
    {
        ice::u32 result = 0;
        for (Type const& object : objects)
        {
            result += ice::forward<Fn>(fn)(object);
        }
        return result;
    }

    template<typename T, typename U = T> requires (std::convertible_to<T, U>)
    constexpr auto lower_bound(ice::Span<T> values, U const& value) noexcept -> ice::u32;

    template<typename T, typename Comp, typename U = T> requires (std::convertible_to<T, U>)
    constexpr auto lower_bound(ice::Span<T> values, U const& value, Comp&& comp) noexcept -> ice::u32;

    template<typename T, typename U = T> requires (std::convertible_to<T, U>)
    constexpr auto upper_bound(ice::Span<T> values, U const& value) noexcept -> ice::u32;

    template<typename T, typename Comp, typename U = T> requires (std::convertible_to<T, U>)
    constexpr auto upper_bound(ice::Span<T> values, U const& value, Comp&& comp) noexcept -> ice::u32;

    template<typename T, typename U = T> requires (std::convertible_to<T, U>)
    constexpr bool binary_search(ice::Span<T> values, U const& value, ice::u32& out_index) noexcept;

    template<typename T, typename Comp, typename U = T> requires (std::convertible_to<T, U>)
    constexpr bool binary_search(ice::Span<T> values, U const& value, Comp&& comp, ice::u32& out_index) noexcept;

    template<typename T, typename U = T> requires (std::convertible_to<T, U>)
    constexpr bool search(ice::Span<T> values, U const& value, ice::u32& out_index) noexcept;

    template<typename T, typename Comp, typename U = T>
    constexpr bool search(ice::Span<T> values, U const& value, Comp&& comp, ice::u32& out_index) noexcept;

    template<typename T>
    inline void sort(ice::Span<T> span) noexcept;

    template<typename T, typename Pred>
    inline void sort(ice::Span<T> span, Pred&& pred) noexcept;

    template<typename K, typename V, typename Pred>
    inline void sort(ice::Span<K> keys, ice::Span<V> values, Pred&& pred) noexcept;

    template<typename T>
    constexpr auto constexpr_sort_stdarray(T const& arr, ice::u32 start_offset = 0) noexcept -> T;


    template<typename K, typename Pred>
    inline void sort_indices(ice::Span<K> keys, ice::Span<ice::u32> indices, Pred&& pred) noexcept;

    template<typename Node, typename Pred>
    inline auto sort_linked_list(Node* left_list, ice::u32 size, Pred&& pred) noexcept -> Node*;


    template<typename T, typename U> requires (std::convertible_to<T, U>)
    constexpr auto lower_bound(ice::Span<T> values, U const& value) noexcept -> ice::u32
    {
        return static_cast<ice::u32>(std::lower_bound(values.begin(), values.end(), value) - values.begin());
    }

    template<typename T, typename Comp, typename U> requires (std::convertible_to<T, U>)
    constexpr auto lower_bound(ice::Span<T> values, U const& value, Comp&& comp) noexcept -> ice::u32
    {
        return static_cast<ice::u32>(std::lower_bound(values.begin(), values.end(), value, ice::forward<Comp>(comp)) - values.begin());
    }

    template<typename T, typename U> requires (std::convertible_to<T, U>)
    constexpr auto upper_bound(ice::Span<T> values, U const& value) noexcept -> ice::u32
    {
        return static_cast<ice::u32>(std::upper_bound(values.begin(), values.end(), value) - values.begin());
    }

    template<typename T, typename Comp, typename U> requires (std::convertible_to<T, U>)
    constexpr auto upper_bound(ice::Span<T> values, U const& value, Comp&& comp) noexcept -> ice::u32
    {
        return static_cast<ice::u32>(std::upper_bound(values.begin(), values.end(), value, ice::forward<Comp>(comp)) - values.begin());
    }

    template<typename T, typename U> requires (std::convertible_to<T, U>)
    constexpr bool binary_search(ice::Span<T> values, U const& predicate, ice::u32& out_index) noexcept
    {
        out_index = ice::lower_bound(values, predicate);
        return (values.size() != out_index) && (values[out_index] == predicate);
    }

    template<typename T, typename Comp, typename U> requires (std::convertible_to<T, U>)
    constexpr bool binary_search(ice::Span<T> values, U const& predicate, Comp&& comp, ice::u32& out_index) noexcept
    {
        out_index = ice::lower_bound(values, predicate, ice::forward<Comp>(comp));
        return (values.size() != out_index) && (values[out_index] == predicate);
    }

    template<typename T, typename U> requires (std::convertible_to<T, U>)
    constexpr bool search(ice::Span<T> values, U const& value, ice::u32& out_index) noexcept
    {
        return search(values, value, [](auto const& l, auto const& r) noexcept { return l == r; }, out_index);
    }

    template<typename T, typename Comp, typename U>
    constexpr bool search(ice::Span<T> values, U const& value, Comp&& comp, ice::u32& out_index) noexcept
    {
        for (ice::nindex idx = 0; idx < values.size(); ++idx)
        {
            if (ice::forward<Comp>(comp)(values[idx], value))
            {
                out_index = idx.u32();
                return true;
            }
        }
        return false;
    }

    template<typename T, typename Comp, typename... U>
    constexpr bool search_with(ice::Span<T> values, Comp&& comp, ice::u32& out_index, U const&... params) noexcept
    {
        for (ice::u32 idx = 0; idx < values.size(); ++idx)
        {
            if (ice::forward<Comp>(comp)(values[idx], idx, params...))
            {
                out_index = idx;
                return true;
            }
        }
        return false;
    }

    namespace detail
    {

        template<typename K, typename V, typename Pred>
        inline auto qsort_partition(ice::Span<K> keys, ice::Span<V>& values, Pred&& pred, ice::i32 left, ice::i32 right) noexcept -> ice::i32
        {
            K* pivot = &keys[right];

            ice::i32 i = left - 1;
            ice::i32 j = left;

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

        template<typename Pred, typename Key, typename... Values>
        inline auto qsort_partition_many(
            Pred&& pred,
            ice::i32 left,
            ice::i32 right,
            ice::Span<Key> keys,
            ice::Span<Values>... values
        ) noexcept -> ice::i32
        {
            Key* pivot = &keys[right];

            ice::i32 i = left - 1;
            ice::i32 j = left;

            while (j < right)
            {
                if (ice::forward<Pred>(pred)(keys[j], *pivot))
                {
                    ++i;
                    ice::swap(keys[i], keys[j]);
                    (ice::swap(values[i], values[j]), ...);
                }
                ++j;
            }
            ice::swap(keys[i + 1], keys[right]);
            (ice::swap(values[i + 1], values[right]), ...);
            return i + 1;
        }

        template<typename K, typename Pred>
        inline auto qsort_partition_indices(ice::Span<K> keys, ice::Span<ice::u32>& indices, Pred&& pred, ice::i32 left, ice::i32 right) noexcept -> ice::i32
        {
            K* pivot = &keys[indices[right]];

            ice::i32 i = left - 1;
            ice::i32 j = left;

            while (j < right)
            {
                if (ice::forward<Pred>(pred)(keys[indices[j]], *pivot))
                {
                    ++i;
                    ice::swap(indices[i], indices[j]);
                }
                ++j;
            }
            ice::swap(indices[i + 1], indices[right]);
            return i + 1;
        }

        //! A qsort implementation with tail call optimization (partial).
        //! \note Implementation base on articles:
        //!     http://www.algolist.net/Algorithms/Sorting/Quicksort
        //!     https://www.geeksforgeeks.org/quicksort-tail-call-optimization-reducing-worst-case-space-log-n/
        template<typename K, typename V, typename Pred>
        inline void qsort(ice::Span<K> keys, ice::Span<V> values, Pred&& pred, ice::i32 left, ice::i32 right) noexcept
        {
            while (left < right)
            {
                ice::i32 const pi = qsort_partition(keys, values, ice::forward<Pred>(pred), left, right);

                ice::detail::qsort(keys, values, ice::forward<Pred>(pred), left, pi - 1);

                left = pi + 1;
            }
        }

        template<typename Pred, typename Key, typename... Values>
        inline void qsort_many(
            Pred&& pred,
            ice::i32 left,
            ice::i32 right,
            ice::Span<Key> keys,
            ice::Span<Values>... values
        ) noexcept
        {
            while (left < right)
            {
                ice::i32 const pi = qsort_partition_many<Pred, Key, Values...>(
                    ice::forward<Pred>(pred), left, right, keys, values...
                );

                ice::detail::qsort_many(ice::forward<Pred>(pred), left, pi - 1, keys, values...);

                left = pi + 1;
            }
        }

        template<typename K, typename Pred>
        inline void qsort_indices(ice::Span<K> keys, ice::Span<ice::u32> indices, Pred&& pred, ice::i32 left, ice::i32 right) noexcept
        {
            while (left < right)
            {
                ice::i32 const pi = qsort_partition_indices(keys, indices, ice::forward<Pred>(pred), left, right);

                ice::detail::qsort_indices(keys, indices, ice::forward<Pred>(pred), left, pi - 1);

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
        ice::i32 const first_index = 0;
        ice::i32 const last_index = ice::count(keys) - 1;

        ice::detail::qsort(keys, values, std::forward<Pred>(pred), first_index, last_index);
    }

    template<typename Key, typename Pred, typename... Values>
        requires ice::concepts::ComparisonFunction<Pred, Key, Key>
    inline void sort_many(ice::Span<Key> keys, Pred&& pred, ice::Span<Values>... values) noexcept
    {
        ice::i32 const first_index = 0;
        ice::i32 const last_index = ice::count(keys) - 1;

        ice::detail::qsort_many(std::forward<Pred>(pred), first_index, last_index, keys, values...);
    }

    template<typename K, typename Pred>
    inline void sort_indices(ice::Span<K> keys, ice::Span<ice::u32> indices, Pred&& pred) noexcept
    {
        ice::i32 const first_index = 0;
        ice::i32 const last_index = keys.size().u32() - 1;

        ice::detail::qsort_indices(keys, indices, std::forward<Pred>(pred), first_index, last_index);
    }

    template<typename Node, typename Pred>
    inline auto sort_linked_list(Node* left_list, ice::u32 size, Pred&& pred) noexcept -> Node*
    {
        Node* right_list = left_list;
        if (size == 1)
        {
            left_list->next = nullptr;
            return left_list;
        }
        else if (size == 2)
        {
            right_list = right_list->next;
            right_list->next = nullptr;
            left_list->next = nullptr;
        }
        else
        {
            uint32_t const half_size = size / 2;
            for (uint32_t idx = half_size; idx > 0; --idx)
            {
                right_list = right_list->next;
            }

            Node* next = right_list->next;
            right_list->next = nullptr;
            right_list = next;

            left_list = ice::sort_linked_list(left_list, half_size + 1, pred);
            right_list = ice::sort_linked_list(right_list, size - (half_size + 1), pred);
        }

        Node result{ .next = left_list }; // Keep the head of the list
        left_list = &result;
        while (left_list->next != nullptr && right_list != nullptr)
        {
            Node* temp = left_list->next;
            if (pred(*temp, *right_list) == false) // TRUE == IS OKAY, FALSE == NEEDS SWAP
            {
                // We swapped the whole lists...
                left_list->next = right_list;
                right_list = temp;
            }

            // We can advance the left list
            left_list = left_list->next;
        }

        // Attach the rest of the right list
        left_list->next = right_list;

        // Return the 'next' element of the result.
        return result.next;
    }

    template<typename T>
    constexpr auto constexpr_sort_stdarray(T const& arr, ice::u32 start_offset) noexcept -> T
    {
        T result = arr;
        std::sort(std::next(std::begin(result), start_offset), std::end(result));
        return result;
    }

} // namespace ice
