#pragma once
#include <ice/base.hxx>
#include <memory_resource>

namespace ice
{

    class Allocator : public std::pmr::memory_resource
    {
    public:
        static constexpr uint32_t Constant_SizeNotTracked = 0xffffffffu;
        static constexpr uint32_t Constant_DefaultAlignment = 4u;

        Allocator() noexcept = default;
        virtual ~Allocator() noexcept = default;

        template<class T, class... Args>
        auto make(Args&&... args) noexcept -> T*;

        template<class T>
        void destroy(T* ptr) noexcept;


        virtual auto allocate(uint32_t size, uint32_t align = Constant_DefaultAlignment) noexcept -> void* = 0;

        virtual void deallocate(void* ptr) noexcept = 0;

        virtual auto allocated_size(void* ptr) noexcept -> uint32_t = 0;

        virtual auto total_allocated() noexcept -> uint32_t = 0;


        Allocator(Allocator const& other) noexcept = delete;
        auto operator=(Allocator const& other) noexcept -> Allocator& = delete;

    private:
        auto do_allocate(size_t size, size_t alignment) noexcept -> void* final
        {
            return allocate(static_cast<uint32_t>(size), static_cast<uint32_t>(alignment));
        }

        void do_deallocate(
            void* ptr,
            [[maybe_unused]] size_t size,
            [[maybe_unused]] size_t alignment
        ) noexcept final
        {
            return deallocate(ptr);
        }

        bool do_is_equal(memory_resource const& other) const noexcept final
        {
            return &other == this;
        }
    };

    template<class T, class... Args>
    auto Allocator::make(Args&&... args) noexcept -> T*
    {
        return new (allocate(sizeof(T), alignof(T))) T(std::forward<Args>(args)...);
    }

    template<class T>
    void Allocator::destroy(T* ptr) noexcept
    {
        if (ptr != nullptr)
        {
            if constexpr (std::is_same_v<std::remove_pointer_t<std::remove_cv_t<std::remove_reference_t<T>>>, void> == false)
            {
                ptr->~T();
            }
            deallocate(ptr);
        }
    }

    namespace memory
    {

        auto null_allocator() noexcept -> ice::Allocator&;

    } // namespace memory

} // namespace ice
