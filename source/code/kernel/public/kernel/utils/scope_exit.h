#pragma once

namespace core::util::detail
{
    template<int Size>
    struct MemoryBlock { char memory[Size]; };

    template<class Lambda>
    class scope_exit {
    public:
        scope_exit(const Lambda& lambda) : m_Lambda(nullptr) {
            m_Lambda = new (m_LambdaMemory.memory) Lambda(lambda);
        }
        ~scope_exit() {
            (*m_Lambda)();
            m_Lambda->~Lambda();
        }

    private:
        Lambda* m_Lambda;
        MemoryBlock<sizeof(Lambda)> m_LambdaMemory;
    };

    struct scope_exit_helper{
        template<class T>
        scope_exit<T> operator+(const T& lambda) {
            return {lambda};
        }
    };
}


#define unique_scope_exit_name(a, b) a##b
#define scope_exit_name_(a, b) unique_scope_exit_name(a, b)
#define scope_exit_name() scope_exit_name_(SCOPE_EXIT_, __COUNTER__)
#define scope_exit auto scope_exit_name() = core::util::scope_exit_helper() + [&]
