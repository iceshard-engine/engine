#include <jni.h>
#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/sensor.h>
#include <android/log.h>

extern "C"
{

JNIEXPORT jstring JNICALL Java_net_example_simple_MyApp_stringFromJNI(
    JNIEnv* env,
    jobject thiz
)
{
    return (*env).NewStringUTF("Hello from JNI! Compiled with ABI arm64.");
}

}
