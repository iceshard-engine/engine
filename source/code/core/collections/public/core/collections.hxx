#pragma once
#include <core/allocator.hxx>
#include <unordered_map>

namespace core
{

    namespace detail
    {

        template<typename T>
        class MemoryResourceAllocatorType : public std::pmr::polymorphic_allocator<T>
        {
        public:
            using value_type = std::pmr::polymorphic_allocator<T>::value_type;

            MemoryResourceAllocatorType() noexcept = delete;

            MemoryResourceAllocatorType(core::allocator& allocator) noexcept
                : std::pmr::polymorphic_allocator<T>{ &allocator }
            {
            }

            template<typename U>
            MemoryResourceAllocatorType(std::pmr::polymorphic_allocator<U> const& other) noexcept
                : std::pmr::polymorphic_allocator<T>{ other }
            {
            }
        };

    } // namespace detail

    template<typename T>
    using Vector = std::vector<T, detail::MemoryResourceAllocatorType<T>>;

    template<typename K, typename V>
    using Map = std::unordered_map<K, V, std::hash<K>, std::equal_to<K>, detail::MemoryResourceAllocatorType<std::pair<const K, V>>>;

} // namespace core
