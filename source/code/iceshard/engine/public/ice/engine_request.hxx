#pragma once
#include <ice/stringid.hxx>

namespace ice
{

    struct EngineRequest
    {
        ice::StringID name;
        ice::uptr payload;
    };

    template<typename T>
    constexpr auto create_request(ice::StringID_Arg name, T value) noexcept -> ice::EngineRequest
    {
        static_assert(std::is_pointer_v<T>, "Non pointer values require explicit specialization before a request can be created!");
        return { name, reinterpret_cast<ice::uptr>(value) };
    }

} // namespace ice
