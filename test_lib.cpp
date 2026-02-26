#define KAWA_TEST_LIB_EXPORT


#include "lib_test.h"

extern "C"
{
    KAWA_API void test_fn(kawa::registry& reg)
    {
        reg.query([](kawa::i32& val) {kw_info("{}", val); });
    }
}
