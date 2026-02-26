#ifndef KAWA_MATH
#define KAWA_MATH

#include "core_types.h"

template<typename T>
using nvector = dyn_array<T>;

template<typename T>
struct nmat
{
	dyn_array<nvector<T>> value;
};


#endif