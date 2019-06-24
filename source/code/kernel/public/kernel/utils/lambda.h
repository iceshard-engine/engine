#pragma once
#include "kernel_pch.h"

namespace ttl
{
    // Forward declare the class to specialize template argument usage
    template<class T>
    struct lambda { };

    // 'partially' specialize the class
    template<class R, class... Args>
    struct lambda<R(Args...)>
    {
        using TLambdaDestructor        = void    (*)(void*);
        using TLambdaCopy            = void*    (*)(void*);
        using TLambdaCall            = R        (*)(void*, Args...);

    public:
        lambda() : m_Destruct(nullptr), m_Copy(nullptr), m_Call(nullptr), m_Callback(nullptr) { }
        lambda(lambda&& that) : m_Destruct(that.m_Destruct), m_Call(that.m_Call), m_Copy(that.m_Copy), m_Callback(that.m_Callback)
        {
            that.m_Call = nullptr;
            that.m_Copy = nullptr;
            that.m_Destruct = nullptr;
            that.m_Callback = nullptr;
        }
        lambda(const lambda& that) : m_Destruct(that.m_Destruct), m_Call(that.m_Call), m_Copy(that.m_Copy)
        {
            // we need to copy the object
            m_Callback = m_Copy(that.m_Callback);
        }
        template<class T>
        lambda(T lambda)
        {
            // Create a new object (store it in a void ptr)
            m_Callback = MemNew(EMem::Kernel) T(lambda);

            // Store the objects 'type' in a lambda function
            m_Call = [](void* lambda, Args... args) -> R {
                return (*reinterpret_cast<T*>(lambda))(args...);
            };
            m_Copy = [](void* lambda) -> void* {
                return MemNew(EMem::Kernel) T(*reinterpret_cast<T*>(lambda));
            };
            m_Destruct = [](void* lambda) -> void {
                RELEASE_NOZERO(reinterpret_cast<T*>(lambda));
            };
        }

        void operator=(const lambda& that)
        {
            if (this == &that) return;
            // Destroy the current object first
            if (m_Destruct && m_Callback)
                m_Destruct(m_Callback);

            // Copy the values
            m_Copy = that.m_Copy;
            m_Call = that.m_Call;
            m_Destruct = that.m_Destruct;
            m_Callback = m_Copy(that.m_Callback);
        }
        void operator=(lambda&& that)
        {
            if (this == &that) return;
            // Destroy the current object first
            if (m_Destruct && m_Callback)
                m_Destruct(m_Callback);

            // Move all values
            m_Copy = that.m_Copy;
            m_Call = that.m_Call;
            m_Destruct = that.m_Destruct;
            m_Callback = that.m_Callback;
            that.m_Call = nullptr;
            that.m_Copy = nullptr;
            that.m_Destruct = nullptr;
            that.m_Callback = nullptr;
        }

        // Call the function
        R operator()(Args... args)
        {
            return m_Call(m_Callback, args...);
        }

        // Check the validity
        bool valid() const
        {
            return m_Callback != nullptr;
        }

        // destroy it
        ~lambda()
        {
            if (m_Destruct && m_Callback)
                m_Destruct(m_Callback);
        }

    private:
        TLambdaDestructor m_Destruct;
        TLambdaCopy m_Copy;
        TLambdaCall m_Call;
        void* m_Callback;
    };

    // Specialize for void returning lambdas
    template<class... Args>
    struct lambda<void(Args...)>
    {
        using TLambdaDestructor        = void    (*)(void*);
        using TLambdaCopy            = void*    (*)(void*);
        using TLambdaCall            = void    (*)(void*, Args...);

    public:
        lambda() : m_Destruct(nullptr), m_Copy(nullptr), m_Call(nullptr), m_Callback(nullptr) { }
        lambda(const lambda& that) : m_Destruct(that.m_Destruct), m_Call(that.m_Call), m_Copy(that.m_Copy)
        {
            // we need to copy the object
            m_Callback = m_Copy(that.m_Callback);
        }
        lambda(lambda&& that) : m_Destruct(that.m_Destruct), m_Call(that.m_Call), m_Copy(that.m_Copy), m_Callback(that.m_Callback)
        {
            that.m_Call = nullptr;
            that.m_Copy = nullptr;
            that.m_Destruct = nullptr;
            that.m_Callback = nullptr;
        }
        template<class T>
        lambda(T lambda)
        {
            // Create a new lambda object
            m_Callback = MemNew(EMem::Kernel) T(lambda);

            // Store the objects 'type' in a lambda function
            m_Call = [](void* lambda, Args... args) -> void {
                return (*reinterpret_cast<T*>(lambda))(args...);
            };
            m_Copy = [](void* lambda) -> void* {
                return MemNew(EMem::Kernel) T(*reinterpret_cast<T*>(lambda));
            };
            m_Destruct = [](void* lambda) {
                RELEASE_NOZERO(reinterpret_cast<T*>(lambda));
            };
        }

        void operator=(const lambda& that)
        {
            if (this == &that) return;
            // Destroy the current object first
            if (m_Destruct && m_Callback)
                m_Destruct(m_Callback);

            // Copy values
            m_Destruct = that.m_Destruct;
            m_Copy = that.m_Copy;
            m_Call = that.m_Call;
            m_Callback = m_Copy(that.m_Callback);
        }
        void operator=(lambda&& that)
        {
            if (this == &that) return;
            // Destroy the current object first
            if (m_Destruct && m_Callback)
                m_Destruct(m_Callback);

            // Move values
            m_Destruct = that.m_Destruct;
            m_Copy = that.m_Copy;
            m_Call = that.m_Call;
            m_Callback = that.m_Callback;
            that.m_Call = nullptr;
            that.m_Copy = nullptr;
            that.m_Destruct = nullptr;
            that.m_Callback = nullptr;
        }

        // Call the function
        void operator()(Args... args)
        {
            m_Call(m_Callback, args...);
        }

        // Check the validity
        bool valid() const
        {
            return m_Callback != nullptr;
        }

        // destroy the lambda object
        ~lambda()
        {
            if (m_Destruct && m_Callback)
                m_Destruct(m_Callback);
        }

    private:
        TLambdaDestructor m_Destruct;
        TLambdaCopy m_Copy;
        TLambdaCall m_Call;
        void* m_Callback;
    };

}
