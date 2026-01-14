// ===== kawa::core::any Usage & API Documentation =====

#include "../kawa/core/any.h"

using namespace kawa;

struct foo
{
	foo(i32 i) : value(i){ kw_info("ctor"); }
	foo(const foo&) { kw_info("copy"); }
	foo(foo&&) noexcept { kw_info("move"); }
	~foo() { kw_info("dtor"); }

	i32 value = 0;
};

int main()
{
	sized_any<64> any(any_construct_tag<foo>{}, 42); // stack allocated, can store value of type up to the provided size
	unsized_any uany(any_construct_tag<std::string>{}, "foo"); // exactly the same api as sized_any but does not have size requirement and is potentially heap allocated 

	auto optional = any.try_unwrap<f32>();
	kw_info("im{}f32", optional ? " " : " not ");

	kw_info("foo::value: {}", any.unwrap<foo>().value); // will assert in debug if type is wrong

	bool _1 = any.try_match( // will exit after matching on the first acceptable case
		[](const f32& f) {kw_info("im f32: {}", f); },
		[](foo& f) {kw_info("im foo, value: {}", f.value); },
		[]() {kw_info("im wildcard"); } // callable with no argument signifies "wildcard" or "default" match, that will fire if none of cases above were able to
	);

	bool _2 = any.try_poly_match( // will NOT exit after matching on the first acceptable case
		[](const f32& f) {kw_info("im f32: {}", f); },
		[](foo& f) {kw_info("im foo, value: {}", f.value); },
		[]() {kw_info("im wildcard"); } 
	);

	any.ensure_match( // will assert if none of the match cases were accepted
		[](const f32& f) {kw_info("im f32: {}", f); },
		[](foo& f) {kw_info("im foo, value: {}", f.value); },
		[]() {kw_info("im wildcard"); }
	);


	// support all of the RAII semantics
	auto any2(any);
	auto any3(std::move(any));

	any.refresh<f32>(123.4f); // in place value refresh


	// correctly calls the dtor
}