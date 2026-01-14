#include "kawa/core/core.h"

using namespace kawa;

int main()
{	
	{
		kw_profile_scope("foo");
		kw_info("{}", type_info_of<std::tuple<int&, float>>.name);
	}

	kw_info_expr(kw_profiler().thread_map[std::this_thread::get_id()].entry_map["foo"].first);
}