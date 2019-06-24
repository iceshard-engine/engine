#include "kernel_pch.h"
#include <kernel/multithreading/multithreading.h>

#include <vector>
#include <unordered_map>

#ifdef _DEBUG
#include <sstream>

#if defined(_MSC_VER)

#define NO_MINMAX
#pragma warning( push )
#pragma warning( disable : 4091)
#include <windows.h>
#include <DbgHelp.h>
#pragma warning( pop )

#pragma comment(lib, "Dbghelp.lib") // #todo Move out of this file into a project description

static void get_stack_trace(void* /*sample_address*/, unsigned from, std::vector<assert::StackTraceSymbol>& out)
{
    // Get the stack back trace capture function
    using CaptureStackBackTraceType = USHORT(WINAPI *)(__in ULONG, __in ULONG, __out PVOID*, __out_opt PULONG);
    auto capture_stack_trace = (CaptureStackBackTraceType)(GetProcAddress(LoadLibraryW(L"kernel32.dll"), "RtlCaptureStackBackTrace"));

    // Does this function exists?
    if (nullptr == capture_stack_trace)
    {
        return;
    }

    // Quote from Microsoft Documentation:
    // ## Windows Server 2003 and Windows XP:
    // ## The sum of the FramesToSkip and FramesToCapture parameters must be less than 63.
    const int max_callers = 62;

    void* callers_stack[max_callers];
    int frames;
    SYMBOL_INFO* symbol;
    HANDLE process;

    // Get the current process handle and initialize the debug library
    process = GetCurrentProcess();
    SymInitialize(process, nullptr, TRUE);

    // Capture the stack trace
    frames = (capture_stack_trace)(from, max_callers, callers_stack, nullptr);

    // Create a symbol object which will be used to get info from the stack trace
    symbol = reinterpret_cast<SYMBOL_INFO*>(calloc(sizeof(SYMBOL_INFO) + (256 * sizeof(char)), 1));
    symbol->MaxNameLen = 255; // Maximum symbol name length
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    // Get the first symbol on the stack trace and save it's name
    SymFromAddr(process, reinterpret_cast<DWORD64>(callers_stack[0]), nullptr, symbol);
    out.push_back(assert::StackTraceSymbol{ symbol->Name, static_cast<uint64_t>(symbol->Address) });

    // Make sure we don't get past the max callers limit
    const int callers_to_read = std::min(max_callers, frames);

    for (int i = 0; i < callers_to_read && strcmp(symbol->Name, "main") != 0; i++)
    {
        // Get the next symbol on the stack
        SymFromAddr(process, reinterpret_cast<DWORD64>(callers_stack[i]), nullptr, symbol);
        out.push_back(assert::StackTraceSymbol{ symbol->Name, static_cast<uint64_t>(symbol->Address) });
    }

    free(symbol);
}

#else // Unix systems

static void get_stack_trace(void* /*sample_address*/, unsigned /*from*/, std::vector<assert::StackTraceSymbol>& /*out*/)
{
    out.push_back(assert::StackTraceSymbol{ "???", 0 });
}

#endif

static assert::FailBehaviour default_assertion_handler(const assert::SmartAssertData& data)
{
    std::stringstream out;
    out << data.file_name() << "(" << data.file_line() << "): Assertion Failed!\n";
    out << "- Condition: " << data.condition() << ".\n";
    out << "- Message: " << data.message() << ".\n";

    int stack_trace_entry = 0;
    auto trace = data.stacktrace();
    out << "- Function: " << trace.front().name << "\n";
    out << "- Stack Trace: \n";
    std::for_each(trace.begin()++, trace.end(), [&](assert::StackTraceSymbol& symbol) {
        out << "  * " << stack_trace_entry << ": " << symbol.name << " {" << std::hex << symbol.address << "}" << std::dec << std::endl;
    });

    out << "# Press 'Debug' to start debugging...\n";
    out << "# Press 'Abort' to crash exit...\n";

    assert::FailBehaviour result = assert::FailBehaviour::ABORT;
    if (result == assert::FailBehaviour::ABORT)
        abort();
    return result;
}

bool assert::AssertionInfo(const std::string& file_and_line, bool update)
{
    static std::unordered_map<std::string, bool> ignored_asserts;
    if (update)
    {
        ignored_asserts.insert({ file_and_line, true });
        return false;
    }
    return ignored_asserts.count(file_and_line) > 0 && ignored_asserts[file_and_line];
}

bool assert::SmartAssert::report_failure(const char* file, int line, const char* condition, const char* message, ...)
{
    sp_criticalsection mutex;
    sp_autosection lock{ mutex };

    SmartAssertData data;
    data.set_source(file, line);
    data.set_condition(condition);

    if (nullptr != message)
    {
        va_list args;
        va_start(args, message);
        char buffer[1024];
        memset(buffer, '\0', 1024);
        vsnprintf(buffer, 1023, message, args);
        data.set_message(buffer);
        va_end(args);
    }

    std::vector<StackTraceSymbol> stack;
    get_stack_trace(this, 2, stack);
    data.set_stacktrace(std::move(stack));

    return report_failure(data) == assert::FailBehaviour::BREAK;
}

assert::FailBehaviour assert::SmartAssert::report_failure(const SmartAssertData& data)
{
    return get_handler()(data);
}

assert::SmartAssertHandler& assert::SmartAssert::get_handler()
{
    static SmartAssertHandler handler_instance = &default_assertion_handler;
    return handler_instance;
}

assert::SmartAssertHandler assert::SmartAssert::set_handler(SmartAssertHandler handler)
{
    SmartAssertHandler current = get_handler();
    get_handler() = handler;
    return current;
}


//////////////////////////////////////////////////////////////////////////
// Smart Assert Data
void assert::SmartAssertData::set_source(std::string file, int line)
{
    _file = std::move(file);
    _line = line;
}

void assert::SmartAssertData::set_message(std::string message)
{
    _message = std::move(message);
}

void assert::SmartAssertData::set_condition(std::string condition)
{
    _condition = std::move(condition);
}

void assert::SmartAssertData::set_stacktrace(std::vector<StackTraceSymbol> trace)
{
    _stacktrace = std::move(trace);
}
std::string assert::SmartAssertData::message() const
{
    return _message;
}

std::string assert::SmartAssertData::condition() const
{
    return _condition;
}

std::string assert::SmartAssertData::file_name() const
{
    return _file;
}

int assert::SmartAssertData::file_line() const
{
    return _line;
}

std::vector<assert::StackTraceSymbol> assert::SmartAssertData::stacktrace() const
{
    return _stacktrace;
}

#endif

int test_func()
{
    return 0;
}
