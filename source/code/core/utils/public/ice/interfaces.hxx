#pragma once
#include <ice/expected.hxx>
#include <ice/stringid.hxx>
#include <concepts>

namespace ice
{

    namespace concepts
    {

        template<typename T>
        concept InterfaceType = requires(T t) {
            { ice::clear_type_t<T>::InterfaceID } -> std::convertible_to<ice::StringID const>;
        };

    } // namespace concepts

    namespace detail
    {

        template<ice::concepts::InterfaceType... Interfaces>
        auto interface_select_helper(auto* ptr, ice::StringID_Arg id) noexcept -> ice::Expected<void*>
        {
            using InterfacePointers = std::tuple<Interfaces*...>;

            if constexpr (sizeof...(Interfaces) > 0)
            {
                ice::StringID const identifiers[]{
                    Interfaces::InterfaceID...
                };

                void* pointers[]{
                    static_cast<Interfaces*>(ptr)...
                };

                ice::u32 idx = 0;
                for (ice::StringID_Arg iface_id : identifiers)
                {
                    if (iface_id == id)
                    {
                        return pointers[idx];
                    }
                    idx += 1;
                }
            }

            return ice::E_Fail;
        }

    } // namespace detail

    struct InterfaceSelector
    {
        virtual ~InterfaceSelector() noexcept = default;

        virtual auto query_interface(ice::StringID_Arg id) noexcept -> ice::Expected<void*>
        {
            return ice::E_Fail;
        }

        template<typename T>
        auto query_interface(T*& out_interface) noexcept -> ice::Result
        {
            ice::Expected<void*> const result = this->query_interface(T::InterfaceID);
            if (result.succeeded())
            {
                out_interface = reinterpret_cast<T*>(result.value());
                return S_Ok;
            }
            return result.error();
        }
    };

    template<typename Derived, ice::concepts::InterfaceType... Interfaces>
    struct InterfaceSelectorOf : public ice::InterfaceSelector
    {
        auto query_interface(ice::StringID_Arg id) noexcept -> ice::Expected<void*> override
        {
            return ice::detail::interface_select_helper<Interfaces...>(static_cast<Derived*>(this), id);
        }
    };

    template<ice::concepts::InterfaceType... Interfaces>
    struct Implements : public InterfaceSelector, public Interfaces...
    {
        auto query_interface(ice::StringID_Arg id) noexcept -> ice::Expected<void*> override
        {
            return ice::detail::interface_select_helper<Interfaces...>(this, id);
        }
    };

} // namespace ice
