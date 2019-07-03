#pragma once

template<class T>
void mooned::io::push(MessagePipe& pipe, T&& message)
{
    static_assert(std::is_pod_v<T>, "A message must be a POD object!");
    static constexpr auto msg_id = core::cexpr::stringid(message::MessageInfo<T>::Name);

    pipe.push(msg_id, &message, sizeof(T));
}

template<class T>
void mooned::io::push(MessagePipe& pipe, const T& message)
{
    static_assert(std::is_pod_v<T>, "A message must be a POD object!");
    static constexpr auto msg_id = core::cexpr::stringid(message::MessageInfo<T>::Name);

    pipe.push(msg_id, &message, sizeof(T));
}

template<class T>
void for_each(const MessagePipe& pipe, void(*func)(const T&))
{
    static_assert(std::is_pod<T>::value, "A message must be a POD object!");
    static constexpr auto msg_id = core::cexpr::stringid(message::MessageInfo<T>::Name);

    pipe.for_each([&](const message::Metadata& mdata, void* data, int size)
        {
            if (mdata.identifier == msg_id)
            {
                assert(sizeof(T) == size);
                func(*reinterpret_cast<T*>(data));
            }
        });
}

template<class T>
void for_each(const MessagePipe& pipe, void(*func)(const T&, const message::Metadata&))
{
    static_assert(std::is_pod<T>::value, "A message must be a POD object!");
    static constexpr auto msg_id = core::cexpr::stringid(message::MessageInfo<T>::Name);

    pipe.for_each([&](const message::Metadata& mdata, void* data, int size)
        {
            if (mdata.identifier == msg_id)
            {
                assert(sizeof(T) == size);
                func(*reinterpret_cast<T*>(data), mdata);
            }
        });
}


template<class T>
void mooned::io::for_each(const MessagePipe& pipe, void* udata, void(*func)(void*, const T&))
{
    static_assert(std::is_pod<T>::value, "A message must be a POD object!");
    static constexpr auto msg_id = core::cexpr::stringid(message::MessageInfo<T>::Name);

    pipe.for_each([&](const message::Metadata& mdata, void* data, int size)
        {
            if (mdata.identifier == msg_id)
            {
                assert(sizeof(T) == size);
                func(udata, *reinterpret_cast<const T*>(data));
            }
        });
}

template<class T>
void mooned::io::for_each(const MessagePipe& pipe, void* udata, void(*func)(void*, const T&, const message::Metadata&))
{
    static_assert(std::is_pod<T>::value, "A message must be a POD object!");
    static constexpr auto msg_id = core::cexpr::stringid(message::MessageInfo<T>::Name);

    pipe.for_each([&](const message::Metadata& mdata, void* data, int size)
        {
            if (mdata.identifier == msg_id)
            {
                assert(sizeof(T) == size);
                func(udata, *reinterpret_cast<const T*>(data), mdata);
            }
        });
}

template<class C, class T>
void for_each(const MessagePipe& pipe, C* obj, void(C::*method)(const T&))
{
    static_assert(std::is_pod<T>::value, "A message must be a POD object!");
    static constexpr auto msg_id = core::cexpr::stringid(message::MessageInfo<T>::Name);

    pipe.for_each([&](const message::Metadata& mdata, const void* data, int size)
        {
            if (mdata.identifier == msg_id)
            {
                assert(sizeof(T) == size);
                (obj->*method)(*reinterpret_cast<const T*>(data));
            }
        });
}

template<class C, class T>
void for_each(const MessagePipe& pipe, C* obj, void(C::*method)(const T&, const message::Metadata&))
{
    static_assert(std::is_pod<T>::value, "A message must be a POD object!");
    static constexpr auto msg_id = core::cexpr::stringid(message::MessageInfo<T>::Name);

    pipe.for_each([&](const message::Metadata& mdata, const void* data, int size)
        {
            if (mdata.identifier == msg_id)
            {
                assert(sizeof(T) == size);
                (obj->*method)(*reinterpret_cast<const T*>(data), mdata);
            }
        });
}
