#pragma once
#include <core/allocator.hxx>
#include <core/data/queue.hxx>
#include <core/datetime/types.hxx>

#include <input/utils/message_info.h>

namespace input
{

class MessageFilter
{
public:
    MessageFilter(core::allocator& alloc);
    MessageFilter(MessageFilter&& other);
    MessageFilter& operator=(MessageFilter&& other);

    MessageFilter(const MessageFilter&) = delete;
    MessageFilter& operator=(const MessageFilter&) = delete;

    ~MessageFilter();

    template<class T>
    MessageFilter(core::allocator& alloc, void(*func)(const T&));

    template<class T>
    MessageFilter(core::allocator& alloc, void(*func)(const T&, core::datetime::tick_type));

    template<class T>
    MessageFilter(core::allocator& alloc, void* udata, void(*func)(const T&));

    template<class T>
    MessageFilter(core::allocator& alloc, void* udata, void(*func)(const T&, core::datetime::tick_type));

    template<class C, class T>
    MessageFilter(core::allocator& alloc, C* object, void(C::*func)(const T&));

    template<class C, class T>
    MessageFilter(core::allocator& alloc, C* object, void(C::*func)(const T&, core::datetime::tick_type));

    bool process(const message::Metadata& mdata, const void* data, uint32_t size) const;

private:
    struct IBase;

    template<class T>
    struct ICallable;

    template<class T>
    struct IFunction;

    template<class T>
    struct IFunctionWithUserdata;

    template<class C, class T>
    struct IMethod;

private:
    core::allocator& _allocator;
    IBase* _handle;
};

#include "message_filter.inl"

}