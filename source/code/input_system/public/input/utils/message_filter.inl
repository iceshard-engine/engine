
//////////////////////////////////////////////////////////////////////////
// Private structures

struct input::MessageFilter::IBase
{
    virtual ~IBase() = default;

    virtual uint64_t message_id() const = 0;
    virtual bool process(const void* data, uint32_t size) const = 0;
};

template<class T>
struct MessageFilter::ICallable : MessageFilter::IBase
{
    uint64_t message_id() const final
    {
        return input::message::MessageInfo<T>::ID;
    }

    bool process(const void* data, uint32_t size) const final
    {
        if (sizeof(T) != size) return false;

        this->call(*reinterpret_cast<const T*>(data));
        return true;
    }

    virtual void call(const T& message) const = 0;
};

template<class T>
struct MessageFilter::IFunction : MessageFilter::ICallable<T>
{
    using FuncPtr = void(*)(const T&);
    IFunction(FuncPtr func) : _func{ func } { }

    void call(const T& message) const final
    {
        _func(message);
    }

    FuncPtr _func;
};

template<class T>
struct MessageFilter::IFunctionWithUserdata : MessageFilter::ICallable<T>
{
    using FuncPtr = void(*)(void*, const T&);
    IFunctionWithUserdata(void* udata, FuncPtr func) : _userdata{ udata }, _func{ func } { }

    void call(const T& message) const final
    {
        _func(_userdata, message);
    }

    void* _userdata;
    FuncPtr _func;
};

template<class C, class T>
struct MessageFilter::IMethod : MessageFilter::ICallable<T>
{
    using MethodPtr = void(C::*)(const T&);
    IMethod(C* obj, MethodPtr method) : _object{ obj }, _method{ method } { }

    void call(const T& message) const final
    {
        (_object->*_method)(message);
    }

    C* _object;
    MethodPtr _method;
};

//////////////////////////////////////////////////////////////////////////
// Template methods

template<class T>
MessageFilter::MessageFilter(core::allocator& alloc, void(*func)(const T&))
    : _allocator{ alloc }
    , _handle{ nullptr }
{
    MAKE_DELETE(_allocator, IBase, _handle);
    _handle = MAKE_NEW(_allocator, IFunction<T>, { func });
}

template<class T>
MessageFilter::MessageFilter(core::allocator& alloc, void* udata, void(*func)(const T&))
    : _allocator{ alloc }
    , _handle{ nullptr }
{
    MAKE_DELETE(_allocator, IBase, _handle);
    _handle = MAKE_NEW(_allocator, IFunctionWithUserdata<T>, { udata, func });
}

template<class C, class T>
MessageFilter::MessageFilter(core::allocator& alloc, C* object, void(C::*func)(const T&))
    : _allocator{ alloc }
    , _handle{ nullptr }
{
    MAKE_DELETE(_allocator, IBase, _handle);
    using IMethodTypeResolved = IMethod<C, T>;
    _handle = MAKE_NEW(_allocator, IMethodTypeResolved, object, func);
}
