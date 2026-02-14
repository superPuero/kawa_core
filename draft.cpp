#include "kawa/core/core.h"

using namespace kawa;

struct user_data
{
	string name;
	string email;
};

int main()
{
	registry r({.name = "kawa_db", .max_entity_count = 1'000'000, .max_component_count = 32});	
	
}