#pragma once


// Additional defines
#ifndef FORCEINLINE
#define FORCEINLINE inline
#endif

// compilation time logging tools 
#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC__ __FILE__ "("__STR1__(__LINE__)") : Warning Msg: "
#define TODO(a) printf("<<<< TODO [%s at line %d]    %s\n", __FILE__, __LINE__, #a)

// Make a callback picker factory and create the callback pickers
#define CALLBACK_PICKER_1(A1, CALLBACK, ...) CALLBACK
#define CALLBACK_PICKER_2(A1, A2, CALLBACK, ...) CALLBACK
#define CALLBACK_PICKER_3(A1, A2, A3, CALLBACK, ...) CALLBACK
#define CALLBACK_PICKER_4(A1, A2, A3, A4, CALLBACK, ...) CALLBACK
#define CALLBACK_PICKER_5(A1, A2, A3, A4, A5, CALLBACK, ...) CALLBACK
#define CALLBACK_PICKER_6(A1, A2, A3, A4, A5, A6, CALLBACK, ...) CALLBACK
#define CALLBACK_PICKER_7(A1, A2, A3, A4, A5, A6, A7, CALLBACK, ...) CALLBACK


// Assertions
#define mError(...) issue_debug_notification(sprintf(...))
#define MAssert(x) { if(!(x)){ mError("Assert "__FI7LE__ ":%u ("#x")\n", __LINE__);__debugbreak(); }}