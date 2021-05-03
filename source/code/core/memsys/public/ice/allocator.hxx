#pragma once
#include <ice/base.hxx>
#include <memory_resource>

namespace ice
{

    class BaseAllocator : public std::pmr::memory_resource
    {
    public:
        static constexpr uint32_t Constant_SizeNotTracked = 0xffffffffu;
        static constexpr uint32_t Constant_DefaultAlignment = 4u;

		BaseAllocator() noexcept = default;
		BaseAllocator(BaseAllocator&) noexcept { }
        virtual ~BaseAllocator() noexcept = default;

        template<class T, class... Args>
        auto make(Args&&... args) noexcept -> T*;

        template<class T>
        void destroy(T* ptr) noexcept;


        virtual auto allocate(uint32_t size, uint32_t align = Constant_DefaultAlignment) noexcept -> void* = 0;

        virtual void deallocate(void* ptr) noexcept = 0;

        virtual auto allocated_size(void* ptr) const noexcept -> uint32_t = 0;

        virtual auto total_allocated() const noexcept -> uint32_t = 0;


		BaseAllocator(BaseAllocator const& other) noexcept = delete;
        auto operator=(BaseAllocator const& other) noexcept -> BaseAllocator& = delete;

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
    auto BaseAllocator::make(Args&&... args) noexcept -> T*
    {
		return new (allocate(sizeof(T), alignof(T))) T{ ice::forward<Args>(args)... };
    }

    template<class T>
    void BaseAllocator::destroy(T* ptr) noexcept
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

	class TrackedAllocator : public ice::BaseAllocator
	{
	public:
		TrackedAllocator() noexcept = default;
		TrackedAllocator(ice::TrackedAllocator& alloc) noexcept
			: _parent{ &alloc }
		{
			_parent->track_child(this);
		}

		~TrackedAllocator() noexcept override
		{
			if (_parent != nullptr)
			{
				if (_prev_sibling == nullptr)
				{
					_parent->_childs = _next_sibling;
					if (_next_sibling != nullptr)
					{
						_next_sibling->_prev_sibling = nullptr;
					}
				}
				else
				{
					_prev_sibling->_next_sibling = _next_sibling;
					if (_next_sibling != nullptr)
					{
						_next_sibling->_prev_sibling = _prev_sibling;
					}
				}
			}
		}

		void track_child(ice::TrackedAllocator* child_allocator) noexcept
		{
			child_allocator->_next_sibling = _childs;
			if (_childs != nullptr)
			{
				_childs->_prev_sibling = child_allocator;
			}
			_childs = child_allocator;
			_childs->_prev_sibling = nullptr;
		}

		auto child_allocators() const noexcept -> ice::TrackedAllocator const*
		{
			return _childs;
		}

		auto next_sibling() const noexcept -> ice::TrackedAllocator const*
		{
			return _next_sibling;
		}

	private:
		ice::TrackedAllocator* const _parent = nullptr;
		ice::TrackedAllocator* _childs = nullptr;
		ice::TrackedAllocator* _next_sibling = nullptr;
		ice::TrackedAllocator* _prev_sibling = nullptr;
	};

	template<bool debug_build = ice::build::is_debug>
	struct BaseAllocatorPicker
	{
		using AllocatorType = ice::BaseAllocator;
	};

	template<>
	struct BaseAllocatorPicker<true>
	{
		using AllocatorType = ice::TrackedAllocator;
	};

	using Allocator = typename BaseAllocatorPicker<>::AllocatorType;

    namespace memory
    {

        auto null_allocator() noexcept -> ice::Allocator&;

    } // namespace memory

} // namespace ice
