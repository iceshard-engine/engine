#pragma once
#include <ice/stringid.hxx>
#include <ice/engine_types.hxx>
#include <ice/mem_allocator.hxx>
#include <ice/span.hxx>

namespace ice
{

    struct DataStorage : ice::AllocatorBase<false>
    {
        using AllocatorBase<false>::AllocatorBase;

        virtual bool has(ice::StringID_Arg name) noexcept = 0;
        virtual bool set(ice::StringID_Arg name, void* value) noexcept = 0;
        virtual bool get(ice::StringID_Arg name, void*& value) noexcept = 0;
        virtual bool get(ice::StringID_Arg name, void const*& value) const noexcept = 0;

        virtual auto allocate_named(ice::StringID_Arg name, ice::AllocRequest alloc_request) noexcept -> ice::AllocResult = 0;

        template<typename T> requires (std::is_trivially_destructible_v<T>)
        inline auto store(ice::StringID_Arg name) noexcept -> T&;

        template<typename T> requires (std::is_trivially_destructible_v<ice::Span<T>>)
        inline auto store(ice::StringID_Arg name, ice::ucount size) noexcept -> ice::Span<T>;

        template<typename T> requires (std::is_trivially_destructible_v<T>)
        inline auto read_or_store(ice::StringID_Arg name) noexcept -> T&;

        template<typename T> requires (std::is_trivially_destructible_v<ice::Span<T>>)
        inline auto read_or_store(ice::StringID_Arg name, ice::ucount size) noexcept -> ice::Span<T>;

        template<typename T>
        inline auto read(ice::StringID_Arg name) noexcept -> T&;

        template<typename T>
        inline auto read(ice::StringID_Arg name) const noexcept -> T const&;
    };

    template<typename T> requires (std::is_trivially_destructible_v<T>)
    inline auto DataStorage::store(ice::StringID_Arg name) noexcept -> T&
    {
        ice::AllocResult result = this->allocate_named(name, ice::meminfo_of<T>);
        ICE_ASSERT_CORE(result.memory != nullptr);
        return *(new (result.memory) T{});
    }

    template<typename T> requires (std::is_trivially_destructible_v<ice::Span<T>>)
    inline auto DataStorage::store(ice::StringID_Arg name, ice::ucount size) noexcept -> ice::Span<T>
    {
        ice::meminfo minfo = ice::meminfo_of<ice::Span<T>>;
        ice::usize const offset = minfo += ice::meminfo_of<T> * size;

        ice::AllocResult result = this->allocate_named(name, minfo);
        ICE_ASSERT_CORE(result.memory != nullptr);
        return *(new (result.memory) ice::Span<T>{ reinterpret_cast<T*>(ice::ptr_add(result.memory, offset)), size });
    }

    template<typename T> requires (std::is_trivially_destructible_v<T>)
    inline auto DataStorage::read_or_store(ice::StringID_Arg name) noexcept -> T&
    {
        if (this->has(name))
        {
            return this->read<T>(name);
        }
        return this->store<T>(name);
    }

    template<typename T> requires (std::is_trivially_destructible_v<ice::Span<T>>)
    inline auto DataStorage::read_or_store(ice::StringID_Arg name, ice::ucount size) noexcept -> ice::Span<T>
    {
        if (this->has(name))
        {
            return this->read<T>(name);
        }
        return this->store<T>(name);
    }

    template<typename T>
    inline auto DataStorage::read(ice::StringID_Arg name) noexcept -> T&
    {
        void* ptr = nullptr;
        bool const object_found = this->get(name, ptr);
        ICE_ASSERT_CORE(object_found);
        return *reinterpret_cast<T*>(ptr);
    }

    template<typename T>
    inline auto DataStorage::read(ice::StringID_Arg name) const noexcept -> T const&
    {
        void const* ptr = nullptr;
        bool const object_found = this->get(name, ptr);
        ICE_ASSERT_CORE(object_found);
        return *reinterpret_cast<T const*>(ptr);
    }

} // namespace ice
