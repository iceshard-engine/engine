/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string/readonly_operations.hxx>
#include <fmt/format.h>

namespace ice::string
{

    using ice::concepts::MutableStringType;

    struct MutableOperations : ice::string::ReadOnlyOperations
    {
        template<MutableStringType Self>
        inline void clear(this Self& self) noexcept
        {
            self.resize(0);
        }

        template<MutableStringType Self>
        inline void push_back(this Self& self, typename Self::CharType character) noexcept
        {
            ice::ncount new_size = self.size() + 1;
            ice::ncount const capacity = self.capacity();

            // Handle resizing if supported
            if constexpr (ice::concepts::ResizableStringType<Self>)
            {
                if (new_size >= capacity)
                {
                    self.grow(new_size);
                }
            }
            else
            {
                ICE_ASSERT_CORE(new_size < capacity);
                new_size = ice::min(new_size, capacity - 1);
            }


            self.resize(new_size);
            self.data()[new_size - 1] = character;
        }

        template<MutableStringType Self>
        inline void push_back(this Self& self, typename Self::CharType const* cstr) noexcept
        {
            return self.push_back(ice::BasicString<typename Self::CharType>{ cstr });
        }

        template<MutableStringType Self>
        inline void push_back(this Self& self, ice::StringType auto const& other) noexcept
        {
            if (other.not_empty())
            {
                ice::ncount new_size = self.size() + other.size();
                ice::ncount const capacity = self.capacity();

                // Handle resizing if supported
                if constexpr (ice::concepts::ResizableStringType<Self>)
                {
                    if (new_size + 1 >= capacity)
                    {
                        self.grow(new_size + 1);
                    }
                }
                else
                {
                    ICE_ASSERT_CORE(new_size < capacity);
                    new_size = ice::min(new_size, capacity - 1);
                }

                ice::memcpy(
                    self.end(),
                    other.cbegin(),
                    other.size().bytes()
                );

                self.resize(new_size);
            }
        }

        template<MutableStringType Self, typename... Args>
        inline constexpr void push_format(
            this Self& self,
            fmt::format_string<Args...> format,
            Args&&... args
        ) noexcept
        {
            ice::ncount const pushed_size = ::fmt::formatted_size(format, std::forward<Args>(args)...);
            ice::ncount const final_size = self.size() + pushed_size;
            ice::ncount const capacity = self.capacity() - 1;

            // Handle resizing if supported
            if constexpr (ice::concepts::ResizableStringType<Self>)
            {
                if (final_size >= capacity)
                {
                    self.grow(final_size + 1);
                }

                ::fmt::format_to_n(self.end(), pushed_size, format, std::forward<Args>(args)...);
            }
            else
            {
                //ICE_ASSERT_CORE(final_size < capacity);
                final_size = ice::min(final_size, capacity);

                ice::ncount const allowed_growth = capacity - self.size();
                if (allowed_growth > 0)
                {
                    ::fmt::format_to_n(self.end(), allowed_growth, format, std::forward<Args>(args)...);
                }
            }
            self.resize(final_size);
        }

        template<MutableStringType Self>
        inline void pop_back(this Self& self, ice::ncount count = 1) noexcept
        {
            if (self.data() != nullptr)
            {
                ice::ncount const current_size = self.size();
                self.resize(current_size - ice::min(current_size, count));
            }
        }

        // Iterators

        template<MutableStringType Self>
        constexpr auto begin(this Self& self) noexcept -> typename Self::Iterator
        {
            return self.data();
        }

        template<MutableStringType Self>
        constexpr auto end(this Self& self) noexcept -> typename Self::Iterator
        {
            return self.data() + self.size();
        }

        template<MutableStringType Self>
        constexpr auto rbegin(this Self& self) noexcept -> typename Self::ReverseIterator
        {
            return typename Self::ReverseIterator{ self.data() + self.size() };
        }

        template<MutableStringType Self>
        constexpr auto rend(this Self& self) noexcept -> typename Self::ReverseIterator
        {
            return typename Self::ReverseIterator{ self.data() };
        }

        // Operators

        using ReadOnlyOperations::operator[];

        template<MutableStringType Self>
        constexpr auto operator[](this Self& self, ice::nindex index) noexcept -> typename Self::ValueType&
        {
            return self.data()[index.native()];
        }

        // Data Helpers

        template<MutableStringType Self>
        constexpr auto memory_view(this Self& self) noexcept -> ice::Memory
        {
            return Memory{ .location = self.data(), .size = self.capacity(), .alignment = ice::align_of<typename Self::CharType> };
        }
    };

} // namespace ice::string
