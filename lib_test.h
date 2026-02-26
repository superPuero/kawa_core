#ifndef KAWA_LIB_TEST
#define KAWA_LIB_TEST

#include "kawa/core/core.h"

#ifdef KAWA_TEST_LIB_EXPORT
    #define KAWA_API __declspec(dllexport)
#else
    #define KAWA_API __declspec(dllimport)
#endif

extern "C" 
{
    KAWA_API void test_fn(kawa::registry& reg);
}

#endif