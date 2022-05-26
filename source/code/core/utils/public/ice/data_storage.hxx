#pragma once
#include <ice/stringid.hxx>
#include <ice/allocator.hxx>
#include <ice/memory/pointer_arithmetic.hxx>
#include <ice/pod/hash.hxx>
#include <ice/span.hxx>

namespace ice
{

    class DataStorage
    {
    public:
        virtual ~DataStorage() noexcept = default;

        virtual auto named_data(
            ice::StringID_Arg name
        ) noexcept -> void* = 0;

        virtual auto named_data(
            ice::StringID_Arg name
        ) const noexcept -> void const* = 0;

        virtual auto allocate_named_data(
            ice::StringID_Arg name,
            ice::u32 size,
            ice::u32 alignment
        ) noexcept -> void* = 0;

        virtual auto allocate_named_array(
            ice::StringID_Arg name,
            ice::u32 element_size,
            ice::u32 alignment,
            ice::u32 count
        ) noexcept -> void* = 0;

        virtual void release_named_data(
            ice::StringID_Arg name
        ) noexcept = 0;

        template<typename T>
        auto named_object(
            ice::StringID_Arg name
        ) noexcept -> T*;

        template<typename T>
        auto named_object(
            ice::StringID_Arg name
        ) const noexcept -> T const*;

        template<typename T>
        auto named_span(
            ice::StringID_Arg name
        ) noexcept -> ice::Span<T>;

        template<typename T>
        auto named_span(
            ice::StringID_Arg name
        ) const noexcept -> ice::Span<T const>;

        template<typename T, typename... Args>
        auto create_named_object(
            ice::StringID_Arg name,
            Args&&... args
        ) noexcept -> T*;

        template<typename T>
        void destroy_named_object(
            ice::StringID_Arg name
        ) noexcept;

        template<typename T>
        auto create_named_span(
            ice::StringID_Arg name,
            ice::u32 size
        ) noexcept -> ice::Span<T>;
    };

    template<typename T>
    inline auto DataStorage::named_object(
        ice::StringID_Arg name
    ) noexcept -> T*
    {
        return reinterpret_cast<T*>(named_data(name));
    }

    template<typename T>
    inline auto DataStorage::named_object(
        ice::StringID_Arg name
    ) const noexcept -> T const*
    {
        return reinterpret_cast<T const*>(named_data(name));
    }

    template<typename T>
    inline auto DataStorage::named_span(
        ice::StringID_Arg name
    ) noexcept -> ice::Span<T>
    {
        // We assume we used 'created_named_span' this the span size is at head of the data.
        void* span_data = named_data(name);

        return ice::Span<T>{
            reinterpret_cast<T*>(ice::memory::ptr_add(span_data, alignof(T))),
            *reinterpret_cast<ice::u32*>(span_data)
        };
    }

    template<typename T>
    inline auto DataStorage::named_span(
        ice::StringID_Arg name
    ) const noexcept -> ice::Span<T const>
    {
        // We assume we used 'created_named_span' this the span size is at head of the data.
        void const* span_data = named_data(name);

        return ice::Span<T const>{
            reinterpret_cast<T const*>(ice::memory::ptr_add(span_data, alignof(T))),
            *reinterpret_cast<ice::u32 const*>(span_data)
        };
    }

    template<typename T, typename... Args>
    inline auto DataStorage::create_named_object(
        ice::StringID_Arg name,
        Args&&... args
    ) noexcept -> T*
    {
        void* const allocated_data = allocate_named_data(name, sizeof(T), alignof(T));
        return new (allocated_data) T{ ice::forward<Args>(args)... };
    }

    template<typename T>
    inline void DataStorage::destroy_named_object(
        ice::StringID_Arg name
    ) noexcept
    {
        T* const obj = named_object<T>(name);
        if (obj != nullptr)
        {
            obj->~T();
            release_named_data(name);
        }
    }

    template<typename T>
    inline auto DataStorage::create_named_span(ice::StringID_Arg name, ice::u32 size) noexcept -> ice::Span<T>
    {
        // We are responsible to access the actual item data after a full 'alingment' ptr add.
        void* const allocated_data = allocate_named_array(name, sizeof(T), alignof(T), size);
        return ice::Span<T>{ reinterpret_cast<T*>(ice::memory::ptr_add(allocated_data, alignof(T))), size };
    }

    class HashedDataStorage final : public ice::DataStorage
    {
    public:
        HashedDataStorage(ice::Allocator& alloc) noexcept;
        ~HashedDataStorage() noexcept override;

        auto named_data(
            ice::StringID_Arg name
        ) noexcept -> void* override;

        auto named_data(
            ice::StringID_Arg name
        ) const noexcept -> void const* override;

        auto allocate_named_data(
            ice::StringID_Arg name,
            ice::u32 size,
            ice::u32 alignment
        ) noexcept -> void* override;

        auto allocate_named_array(
            ice::StringID_Arg name,
            ice::u32 element_size,
            ice::u32 alignment,
            ice::u32 count
        ) noexcept -> void* override;

        void release_named_data(
            ice::StringID_Arg name
        ) noexcept override;

    private:
        ice::Allocator& _allocator;
        ice::pod::Hash<void*> _named_data;
    };

} // namespace ice
