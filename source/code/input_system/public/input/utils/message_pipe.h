#pragma once
#include <core/allocator.hxx>
#include <core/pod/hash.hxx>

#include <input/utils/message_data.h>
#include <input/utils/message_info.h>

#include <functional>
#include <vector>
#include <cassert>

namespace input
{

class MessageFilter;

template<class T>
struct MessageInfo;

class MessagePipe final
{
public:
    MessagePipe(core::allocator& alloc);
    ~MessagePipe();

    MessagePipe(MessagePipe&&) = delete;
    MessagePipe& operator=(MessagePipe&&) = delete;

    MessagePipe(const MessagePipe&) = delete;
    MessagePipe& operator=(const MessagePipe&) = delete;

    int count() const;

    void push(uint64_t id, const void* data, int size);

    void for_each(std::function<void(const message::Metadata&, const void* data, int size)> func) const;

    void clear();

private:
    core::allocator& _allocator;
    input::message::Data _data;
};

template<class T>
void push(MessagePipe& pipe, T&& message);

template<class T>
void push(MessagePipe& pipe, const T& message);

template<class T>
void for_each(const MessagePipe& pipe, void(*func)(const T&));

template<class T>
void for_each(const MessagePipe& pipe, void(*func)(const T&, const message::Metadata&));

template<class T>
void for_each(const MessagePipe& pipe, void* userdata, void(*func)(void*, const T&));

template<class T>
void for_each(const MessagePipe& pipe, void* userdata, void(*func)(void*, const T&, const message::Metadata&));

template<class C, class T>
void for_each(const MessagePipe& pipe, C* obj, void(C::*method)(const T&));

template<class C, class T>
void for_each(const MessagePipe& pipe, C* obj, void(C::*method)(const T&, const message::Metadata&));

void filter(const MessagePipe& pipe, const std::vector<MessageFilter>& filters);

#include "message_pipe.inl"

}
