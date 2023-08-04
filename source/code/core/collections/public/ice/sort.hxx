/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <ice/span.hxx>
#include <algorithm>

namespace ice
{

    template<typename T, typename U = T> requires (std::convertible_to<T, U>)
    constexpr auto lower_bound(ice::Span<T> values, U const& value) noexcept -> ice::ucount;

    template<typename T, typename Comp, typename U = T> requires (std::convertible_to<T, U>)
    constexpr auto lower_bound(ice::Span<T> values, U const& value, Comp&& comp) noexcept -> ice::ucount;

    template<typename T, typename U = T> requires (std::convertible_to<T, U>)
    constexpr auto upper_bound(ice::Span<T> values, U const& value) noexcept -> ice::ucount;

    template<typename T, typename Comp, typename U = T> requires (std::convertible_to<T, U>)
    constexpr auto upper_bound(ice::Span<T> values, U const& value, Comp&& comp) noexcept -> ice::ucount;

    template<typename T, typename U = T> requires (std::convertible_to<T, U>)
    constexpr bool binary_search(ice::Span<T> values, U const& value, ice::ucount& out_index) noexcept;

    template<typename T, typename Comp, typename U = T> requires (std::convertible_to<T, U>)
    constexpr bool binary_search(ice::Span<T> values, U const& value, Comp&& comp, ice::ucount& out_index) noexcept;

    template<typename T>
    inline void sort(ice::Span<T> span) noexcept;

    template<typename T, typename Pred>
    inline void sort(ice::Span<T> span, Pred&& pred) noexcept;

    template<typename K, typename V, typename Pred>
    inline void sort(ice::Span<K> keys, ice::Span<V> values, Pred&& pred) noexcept;

    template<typename Node, typename Pred>
    inline auto sort_linked_list(Node* left_list, ice::u32 size, Pred&& pred) noexcept -> Node*;


    template<typename T, typename U> requires (std::convertible_to<T, U>)
    constexpr auto lower_bound(ice::Span<T> values, U const& value) noexcept -> ice::ucount
    {
        return static_cast<ice::ucount>(std::lower_bound(ice::span::begin(values), ice::span::end(values), value) - ice::span::begin(values));
    }

    template<typename T, typename Comp, typename U> requires (std::convertible_to<T, U>)
    constexpr auto lower_bound(ice::Span<T> values, U const& value, Comp&& comp) noexcept -> ice::ucount
    {
        return static_cast<ice::ucount>(std::lower_bound(ice::span::begin(values), ice::span::end(values), value, ice::forward<Comp>(comp)) - ice::span::begin(values));
    }

    template<typename T, typename U> requires (std::convertible_to<T, U>)
    constexpr auto upper_bound(ice::Span<T> values, U const& value) noexcept -> ice::ucount
    {
        return static_cast<ice::ucount>(std::upper_bound(ice::span::begin(values), ice::span::end(values), value) - ice::span::begin(values));
    }

    template<typename T, typename Comp, typename U> requires (std::convertible_to<T, U>)
    constexpr auto upper_bound(ice::Span<T> values, U const& value, Comp&& comp) noexcept -> ice::ucount
    {
        return static_cast<ice::ucount>(std::upper_bound(ice::span::begin(values), ice::span::end(values), value, ice::forward<Comp>(comp)) - ice::span::begin(values));
    }

    template<typename T, typename U> requires (std::convertible_to<T, U>)
    constexpr bool binary_search(ice::Span<T> values, U const& predicate, ice::ucount& out_index) noexcept
    {
        out_index = ice::lower_bound(values, predicate);
        return (ice::count(values) != out_index) && (values[out_index] == predicate);
    }

    template<typename T, typename Comp, typename U> requires (std::convertible_to<T, U>)
    constexpr bool binary_search(ice::Span<T> values, U const& predicate, Comp&& comp, ice::ucount& out_index) noexcept
    {
        out_index = ice::lower_bound(values, predicate, ice::forward<Comp>(comp));
        return (ice::count(values) != out_index) && (values[out_index] == predicate);
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

    } // namespace detail

    template<typename T>
    inline void sort(ice::Span<T> span) noexcept
    {
        std::sort(span.begin(), span.end());
    }

    template<typename T, typename Pred>
    inline void sort(ice::Span<T> span, Pred&& pred) noexcept
    {
        std::sort(ice::span::begin(span), ice::span::end(span), ice::forward<Pred>(pred));
    }

    template<typename K, typename V, typename Pred>
    inline void sort(ice::Span<K> keys, ice::Span<V> values, Pred&& pred) noexcept
    {
        ice::i32 const first_index = 0;
        ice::i32 const last_index = ice::count(keys) - 1;

        ice::detail::qsort(keys, values, std::forward<Pred>(pred), first_index, last_index);
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

} // namespace ice
