#include "kawa/core/core.h"
#include "kawa/core/de_serialize.h"

using namespace kawa;


struct test_t
{
	int x;
};

template<>
struct registry::serialize_trait<test_t>
{
	static void write(std::ofstream& out, test_t& value)
	{
		out << value.x << '\n';
	}
};

template<>
struct registry::deserialize_trait<test_t>
{
	static void read(registry& reg, entity_id id, std::ifstream& in)
	{
		test_t value;
		in >> value.x;
		reg.add(id, value);
	}
};

int main()
{

	registry r({});

	r.ensure<test_t>();

	registry::deserializer deser{ .file{"test.txt"} };
	//registry::serializer ser{ .file{"test.txt"} };

	//r.entity(
		//test_t{ 42 }
	//);

	//r.serialize(ser);
	r.deserialize(deser);

	r.query_info(
		[](entity_id i, component_info info)
		{
			kw_info("{}", (u64)i);
			kw_info("{}", info.type_info.name);
		}
	);

	//r.entity(
	//	test_t{ 42 }
	//);

	//serializer ser;

	//r.serialize(ser);

	//serializer_map map;
	//map.ensure<test_t>();
	//
	//serializer s;
	//test_t v{ 12 };
	//s.put(v);

	//deserializer_map map;
	//map.ensure<test_t>();

	//deserializer s;
	//test_t v{ 42 };
	//s.get(map, &v);

	//kw_info("{}", v.x);

}