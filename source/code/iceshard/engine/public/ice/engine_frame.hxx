#pragma once
#include <ice/span.hxx>
#include <ice/stringid.hxx>
#include <ice/input/input_types.hxx>

namespace ice
{

    struct EngineRequest;

    class EngineFrame
    {
    public:
        virtual ~EngineFrame() noexcept = default;

        virtual auto memory_consumption() noexcept -> ice::u32 = 0;

        virtual auto input_events() const noexcept -> ice::Span<ice::input::InputEvent const> = 0;

        virtual void push_requests(
            ice::Span<EngineRequest const> requests
        ) noexcept = 0;

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

        template<typename T>
        auto named_object(
            ice::StringID_Arg name
        ) noexcept -> T*;

        template<typename T>
        auto named_object(
            ice::StringID_Arg name
        ) const noexcept -> T const*;

        template<typename T>
        auto create_named_object(
            ice::StringID_Arg name
        ) noexcept -> T*;

        template<typename T>
        auto create_named_span(
            ice::StringID_Arg name,
            ice::u32 size
        ) noexcept -> ice::Span<T>;
    };

    template<typename T>
    auto EngineFrame::named_object(
        ice::StringID_Arg name
    ) noexcept -> T*
    {
        return reinterpret_cast<T*>(named_data(name));
    }

    template<typename T>
    auto EngineFrame::named_object(
        ice::StringID_Arg name
    ) const noexcept -> T const*
    {
        return reinterpret_cast<T const*>(named_data(name));
    }

    template<typename T>
    inline auto EngineFrame::create_named_object(
        ice::StringID_Arg name
    ) noexcept -> T*
    {
        void* const allocated_data = allocate_named_data(name, sizeof(T), alignof(T));
        return new (allocated_data) T{};
    }

    template<typename T>
    inline auto EngineFrame::create_named_span(ice::StringID_Arg name, ice::u32 size) noexcept -> ice::Span<T>
    {
        void* const allocated_data = allocate_named_data(name, sizeof(T) * size, alignof(T));
        return ice::Span<T>{ reinterpret_cast<T*>(allocated_data), size };
    }

} // namespace ice
