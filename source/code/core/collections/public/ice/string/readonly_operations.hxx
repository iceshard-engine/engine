/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <utility>
#include <ice/string/string_concepts.hxx>

namespace ice::string
{

    using ice::concepts::StringType;

    struct ReadOnlyOperations
    {
        template<StringType Self>
        constexpr bool is_empty(this Self const& self) noexcept
        {
            return self.size() == 0;
        }

        template<StringType Self>
        constexpr bool not_empty(this Self const& self) noexcept
        {
            return self.size() > 0;
        }

        template<StringType Self>
        inline auto front(this Self const& self) noexcept -> typename Self::CharType
        {
            return self.data()[0];
        }

        template<StringType Self>
        inline auto back(this Self const& self) noexcept -> typename Self::CharType
        {
            return self.data()[self.size() - 1];
        }

        template<StringType Self>
        constexpr auto substr(
            this Self const& self, ice::nindex pos, ice::ncount len = {}
        ) noexcept -> typename Self::StringType
        {
            ice::ncount const size = self.size();
            if (pos >= size)
            {
                return { };
            }

            if (len == ice::ncount_none)
            {
                return { self.data() + pos, size - pos };
            }
            else
            {
                return { self.data() + pos, std::min(len, size - pos) };
            }
        }

        template<StringType Self>
        constexpr auto substr(
            this Self const& self, ice::ref32 ref
        ) noexcept -> typename Self::StringType
        {
            return self.substr(ref.offset, ref.size);
        }

        template<StringType Self>
        constexpr auto starts_with(this Self const& self, StringType auto prefix) noexcept
        {
            return self.substr(0, prefix.size()) == prefix;
        }

        template<StringType Self>
        constexpr auto find_first_of(
            this Self const& self,
            typename Self::CharType character_value,
            ice::nindex start = 0
        ) noexcept -> ice::nindex
        {
            auto const* it = self.cbegin() + start;
            auto const* const beg = it;
            auto const* const end = self.cend();

            while (it < end && *it != character_value)
            {
                it += 1;
            }

            return it >= end ? ice::nindex{} : ice::nindex{ start + (it - beg) };
        }

        template<StringType Self>
        constexpr auto find_first_of(
            this Self const& self,
            typename Self::StringType character_values,
            ice::nindex start = 0
        ) noexcept -> ice::nindex
        {
            auto const* it = self.cbegin() + start;
            auto const* const beg = it;
            auto const* const it_end = self.cend();

            while (it < it_end && character_values.find_first_of(*it) == none_index)
            {
                it += 1;
            }

            return it >= it_end ? ice::nindex{} : ice::nindex{ start + (it - beg) };
        }

        template<StringType Self>
        constexpr auto find_last_of(
            this Self const& self,
            typename Self::CharType character_value,
            ice::nindex start = 0
        ) noexcept -> ice::nindex
        {
            auto it = self.crbegin();
            auto const it_end = self.crend();

            while (it != it_end && start > 0)
            {
                it += 1;
                start -= 1;
            }

            while (it != it_end && *it != character_value)
            {
                it += 1;
            }

            return it == it_end ? ice::nindex{} : ice::nindex{ u64((it_end - it) - 1) };
        }

        template<StringType Self>
        constexpr auto find_last_of(
            this Self const& self,
            typename Self::StringType character_values,
            ice::nindex start = 0
        ) noexcept -> ice::nindex
        {
            auto it = self.crbegin();
            auto const it_end = self.crend();

            while (it != it_end && start > 0)
            {
                it += 1;
                start -= 1;
            }

            while (it != it_end && character_values.find_first_of(*it) == none_index)
            {
                it += 1;
            }

            return it == it_end ? ice::nindex{} : ice::nindex{ u64((it_end - it) - 1) };
        }

        template<StringType Self>
        constexpr auto find_first_not_of(
            this Self const& self,
            typename Self::CharType character_value,
            ice::nindex start_idx = 0
        ) noexcept -> ice::nindex
        {
            auto const* it = self.cbegin() + start_idx;
            auto const* const beg = it;
            auto const* const end = self.cend();

            while (it < end && *it == character_value)
            {
                it += 1;
            }

            return it >= end ? ice::nindex{} : start_idx + ice::nindex{ u64(it - beg) };
        }

        template<StringType Self>
        constexpr auto find_first_not_of(
            this Self const& self,
            typename Self::StringType character_values,
            ice::nindex start_idx = 0
        ) noexcept -> ice::nindex
        {
            auto const* it = self.cbegin() + start_idx;
            auto const* const beg = it;
            auto const* const it_end = self.cend();

            while (it < it_end && character_values.find_first_of(*it) != ice::none_index)
            {
                it += 1;
            }

            return it >= it_end ? ice::nindex{} : start_idx + ice::nindex{ u64(it - beg) };
        }

        template<StringType Self>
        constexpr auto find_last_not_of(
            this Self const& self,
            typename Self::CharType character_value,
            ice::nindex start_idx = 0
        ) noexcept -> ice::nindex
        {
            auto it = self.crbegin();
            auto const end = self.crend();

            while (it != end && start_idx > 0)
            {
                it += 1;
                start_idx -= 1;
            }

            while (it != end && *it == character_value)
            {
                it += 1;
            }

            return it == end ? ice::nindex{} : ice::nindex{ u64((end - it) - 1) };
        }

        template<StringType Self>
        constexpr auto find_last_not_of(
            this Self const& self,
            typename Self::StringType character_values,
            ice::nindex start_idx = 0
        ) noexcept -> ice::nindex
        {
            auto it = self.crbegin();
            auto const it_end = self.crend();

            while (it != it_end && start_idx > 0)
            {
                it += 1;
                start_idx -= 1;
            }

            while (it != it_end && character_values.find_first_of(*it) != ice::none_index)
            {
                it += 1;
            }

            return it == it_end ? ice::nindex{} : ice::nindex{ (it_end - it) - 1 };
        }

        // Iterators

        template<StringType Self>
        constexpr auto cbegin(this Self const& self) noexcept -> typename Self::ConstIterator
        {
            return self.data();
        }

        template<StringType Self>
        constexpr auto cend(this Self const& self) noexcept -> typename Self::ConstIterator
        {
            return self.data() + self.size();
        }

        template<StringType Self>
        constexpr auto crbegin(this Self const& self) noexcept -> typename Self::ConstReverseIterator
        {
            return typename Self::ConstReverseIterator{ self.data() + self.size() };
        }

        template<StringType Self>
        constexpr auto crend(this Self const& self) noexcept -> typename Self::ConstReverseIterator
        {
            return typename Self::ConstReverseIterator{ self.data() };
        }

        template<StringType Self>
        constexpr auto begin(this Self const& self) noexcept -> typename Self::ConstIterator
        {
            return self.cbegin();
        }

        template<StringType Self>
        constexpr auto end(this Self const& self) noexcept -> typename Self::ConstIterator
        {
            return self.cend();
        }

        template<StringType Self>
        constexpr auto rbegin(this Self const& self) noexcept -> typename Self::ConstReverseIterator
        {
            return self.crbegin();
        }

        template<StringType Self>
        constexpr auto rend(this Self const& self) noexcept -> typename Self::ConstReverseIterator
        {
            return self.crend();
        }

        // Operators

        template<StringType Self>
        constexpr auto operator[](this Self const& self, ice::nindex index) noexcept -> typename Self::ValueType
        {
            return self.data()[index];
        }

        template<StringType Self>
        constexpr bool operator==(this Self const& self, typename Self::StringType const& other) noexcept
        {
            ice::ncount const size = self.size();
            if (size == other.size())
            {
                typename Self::ValueType const* self_data = self.data();
                typename Self::ValueType const* other_data = other.data();

                ice::nindex::base_type idx = 0;
                while (idx < size && self_data[idx] == other_data[idx])
                {
                    idx += 1;
                }
                return idx == size;
            }
            return false;
        }

        // Data Helpers

        template<StringType Self>
        constexpr auto meminfo(this Self const& self) noexcept -> ice::meminfo
        {
            return ice::meminfo{
                ice::meminfo_of<typename Self::CharType> * self.size().native()
            };
        }
    };

} // namespace ice::string
