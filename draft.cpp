#include "kawa/core/core.h"
#include "kawa/core/experimental.h"

#include "kawa/core/multi_broadcaster.h"

using namespace kawa;


struct sub
{
	void fn(const int& v)
	{
		kw_info("{}", v);
	}

};


int main()
{
	broadcaster<int> v;
	sub s;

	v.subscribe(s, &sub::fn);

	v.emit(42);
}