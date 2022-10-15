#pragma once
#include <ice/stringid.hxx>
#include <ice/mem_allocator.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/span.hxx>

namespace ice
{

    class DataStorage
    {
    public:
        virtual ~DataStorage() noexcept = default;

        template<typename T>
        auto named_object(ice::StringID_Arg name) noexcept -> T*;

        template<typename T>
        auto named_object(ice::StringID_Arg name) const noexcept -> T const*;

        template<typename T>
            requires ice::TrivialContainerLogicAllowed<T>
        auto named_span(ice::StringID_Arg name) noexcept -> ice::Span<T>;

        template<typename T>
            requires ice::TrivialContainerLogicAllowed<T>
        auto named_span(ice::StringID_Arg name) const noexcept -> ice::Span<T const>;

        template<typename T, typename... Args>
        auto create_named_object(ice::StringID_Arg name, Args&&... args) noexcept -> T*;

        template<typename T>
            requires ice::TrivialContainerLogicAllowed<T>
        auto create_named_span(ice::StringID_Arg name, ice::ucount count) noexcept -> ice::Span<T>;

        template<typename T>
        void destroy_named_object(ice::StringID_Arg name) noexcept;

    protected:
        virtual auto allocate_named_data(
            ice::StringID_Arg name,
            ice::meminfo type_meminfo
        ) noexcept -> void* = 0;

        virtual auto allocate_named_array(
            ice::StringID_Arg name,
            ice::meminfo type_meminfo,
            ice::ucount count
        ) noexcept -> void* = 0;

        virtual auto named_data(ice::StringID_Arg name) noexcept -> void* = 0;

        virtual auto named_data(ice::StringID_Arg name) const noexcept -> void const* = 0;

        virtual void release_named_data(ice::StringID_Arg name) noexcept = 0;
    };

    template<typename T>
    inline auto DataStorage::named_object(ice::StringID_Arg name) noexcept -> T*
    {
        return reinterpret_cast<T*>(named_data(name));
    }

    template<typename T>
    inline auto DataStorage::named_object(ice::StringID_Arg name) const noexcept -> T const*
    {
        return reinterpret_cast<T const*>(named_data(name));
    }

    template<typename T>
        requires ice::TrivialContainerLogicAllowed<T>
    inline auto DataStorage::named_span(ice::StringID_Arg name) noexcept -> ice::Span<T>
    {
        // We assume we used 'created_named_span' this the span size is at head of the data.
        ice::ucount* span_info = reinterpret_cast<ice::ucount*>(named_data(name));

        return ice::Span<T>{
            reinterpret_cast<T*>(ice::ptr_add(span_info, span_info[1])),
            span_info[0]
        };
    }

    template<typename T>
        requires ice::TrivialContainerLogicAllowed<T>
    inline auto DataStorage::named_span(ice::StringID_Arg name) const noexcept -> ice::Span<T const>
    {
        // We assume we used 'created_named_span' this the span size is at head of the data.
        ice::ucount const* span_info = reinterpret_cast<ice::ucount const*>(named_data(name));

        return ice::Span<T>{
            reinterpret_cast<T*>(ice::ptr_add(span_info, span_info[1])),
            span_info[0]
        };
    }

    template<typename T, typename... Args>
    inline auto DataStorage::create_named_object(ice::StringID_Arg name, Args&&... args) noexcept -> T*
    {
        void* const allocated_data = allocate_named_data(name, ice::meminfo_of<T>);
        return new (allocated_data) T{ ice::forward<Args>(args)... };
    }

    template<typename T>
    inline void DataStorage::destroy_named_object(ice::StringID_Arg name) noexcept
    {
        T* const obj = named_object<T>(name);
        if (obj != nullptr)
        {
            obj->~T();
            release_named_data(name);
        }
    }

    template<typename T>
        requires ice::TrivialContainerLogicAllowed<T>
    inline auto DataStorage::create_named_span(ice::StringID_Arg name, ice::ucount count) noexcept -> ice::Span<T>
    {
        ice::ucount* span_info = reinterpret_cast<ice::ucount*>(
            allocate_named_array(name, ice::meminfo_of<T>, count)
        );

        ice::Span<T> result{
            reinterpret_cast<T*>(ice::ptr_add(span_info, { span_info[1] })),
            span_info[0]
        };

        // We initialize the objects if needed
        if constexpr (std::is_trivially_constructible_v<T> == false)
        {
            ice::mem_construct_n_at<T>(
                ice::Memory{ .location = result._data, .size = ice::span::size_bytes(result), .alignment = ice::align_of<T> },
                ice::span::count(result)
            );
        }

        return result;
    }

    class HashedDataStorage final : public ice::DataStorage
    {
    public:
        HashedDataStorage(ice::Allocator& alloc) noexcept;
        ~HashedDataStorage() noexcept override;

    protected:
        auto named_data(
            ice::StringID_Arg name
        ) noexcept -> void* override;

        auto named_data(
            ice::StringID_Arg name
        ) const noexcept -> void const* override;

        auto allocate_named_data(
            ice::StringID_Arg name,
            ice::meminfo type_meminfo
        ) noexcept -> void* override;

        auto allocate_named_array(
            ice::StringID_Arg name,
            ice::meminfo type_meminfo,
            ice::ucount count
        ) noexcept -> void* override;

        void release_named_data(
            ice::StringID_Arg name
        ) noexcept override;

    private:
        ice::Allocator& _allocator;
        ice::HashMap<ice::AllocResult> _named_data;
    };

} // namespace ice
