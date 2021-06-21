#pragma once
#include <ice/stringid.hxx>
#include <ice/allocator.hxx>
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
        void* const allocated_data = allocate_named_data(name, sizeof(T) * size, alignof(T));
        return ice::Span<T>{ reinterpret_cast<T*>(allocated_data), size };
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

        void release_named_data(
            ice::StringID_Arg name
        ) noexcept override;

    private:
        ice::Allocator& _allocator;
        ice::pod::Hash<void*> _named_data;
    };

} // namespace ice
