#ifndef KAWA_MULTI_BROADCASTER
#define KAWA_MULTI_BROADCASTER

#include "ecs.h"
#include "broadcaster.h"

namespace kawa
{
	struct multi_broadcaster
	{
		multi_broadcaster()
			: _handle(_container.entity())
		{}

		template<typename message_t>
		broadcaster<message_t>& get() noexcept
		{
			return _container.try_emplace<broadcaster<message_t>>(_handle);
		}

		template<typename message_t>
		void emit(const message_t& msg)
		{
			get<message_t>().emit(msg);
		}

		registry _container{ {.max_entity_count = 1} };
		entity_id _handle;
	};
}

#endif // !KAWA_MULTI_BROADCASTER
