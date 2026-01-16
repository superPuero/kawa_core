#include "kawa/core/core.h"
#include "kawa/core/event_manager.h"

#include "kawa/core/experimental.h"

using namespace kawa;

using entity_event_manager = basic_event_manager<64>;

struct entity_added
{
	entity_id id;
};

struct recieve_component
{
	recieve_component(entity_id e, entity_event_manager& mgr) : id(e), _guard(*this, mgr) {}

	void dispatch(const entity_event_manager::event_t& e)
	{
		e.try_match(
			[&](const entity_added& added_event) { kw_info("entity {} greets {}", id.as_u64(), added_event.id.as_u64()); }
		);
	}

	entity_id id;
	entity_event_manager::subscribe_guard _guard;
};

int main()
{
	entity_event_manager event_manager;
	
	registry reg({.max_entity_count = 0xFFF});

	usize i = 1000;

	while (i--)
	{
		auto e = reg.entity();

		reg.emplace<recieve_component>(e, e, event_manager);
	}

	event_manager.emit(entity_added{ .id = reg.entity() });
}