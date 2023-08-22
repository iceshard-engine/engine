#include <jni.h>
#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/sensor.h>
#include <android/log.h>

#include <ice/string/string.hxx>
#include <ice/task.hxx>
#include <ice/task_queue.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/task_thread.hxx>
#include <ice/task_utils.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/mem_allocator_host.hxx>

#include <ice/log_module.hxx>
#include <ice/module_register.hxx>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

auto test(ice::TaskScheduler sched) noexcept -> ice::Task<>
{
    ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Game, "Test {}", syscall(__NR_gettid));
    co_await sched;
    ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Game, "Test {}", syscall(__NR_gettid));
    co_return;
};

extern "C"
{

JNIEXPORT jstring JNICALL Java_net_example_simple_MyApp_stringFromJNI(
    JNIEnv* env,
    jobject thiz
)
{
    static ice::HostAllocator alloc;
    static ice::TaskQueue queue;
    auto mreg = ice::create_default_module_register(alloc);
    mreg->load_module(alloc, ice::load_log_module, ice::unload_log_module);

    ice::TaskScheduler sched{ queue };
    auto thread = ice::create_thread(alloc, queue, { .exclusive_queue = true });
    ice::ManualResetEvent ev;
    ice::manual_wait_for(test(sched), ev);
    ICE_LOG(ice::LogSeverity::Retail, ice::LogTag::Game, "Test");
    ev.wait();
    thread = nullptr;

    return (*env).NewStringUTF("Hello from JNI! Compiled with ABI arm64.");
}

}
