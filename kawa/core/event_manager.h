#ifndef KAWA_EVENT_MANAGER
#define KAWA_EVENT_MANAGER

#include <format>
#include "core_types.h"
#include "any.h"

namespace kawa
{
	template<usize event_storage_size = 64>
	class basic_event_manager
	{
	public:
		using event_t = sized_any<event_storage_size>;

		struct subscribe_guard
		{
			template<typename T>
			subscribe_guard(T& self, basic_event_manager& manager)
				: addr((uintptr_t)&self)
				, mgr(manager)				
			{
				mgr.subscribe(self);
			}

			~subscribe_guard()
			{
				mgr.unsubscribe(addr);
			}

			uintptr_t addr;
			basic_event_manager& mgr;
		};

		struct dispatcher
		{
			using dispatch_invoke_fn_t = void(void*, const event_t& e);
			void* value_ptr = nullptr;
			dispatch_invoke_fn_t* dispatch_invoke_fn = nullptr;

			void dispatch(const event_t& e) noexcept
			{
				dispatch_invoke_fn(value_ptr, e);
			}
		};

		template<typename value_t>
		void subscribe(value_t& val)
		{
			auto& d = dispatchers.emplace_back();
			d.dispatch_invoke_fn =
				[](void* value_ptr, const event_t& e)
				{
					static_cast<value_t*>(value_ptr)->dispatch(e);
				};
			d.value_ptr = &val;
		}

		void unsubscribe(uintptr_t addr)
		{
			usize i = 0;
			bool found = false;
			for (auto& d : dispatchers)
			{
				if (d.value_ptr == &addr)
				{
					found = true;
					break;
				}
				i++;
			}

			if (found)
			{
				dispatchers[i] = dispatchers.back();
				dispatchers.pop_back();
			}
		}

		template<typename value_t>
		void emit(const value_t& value) noexcept
		{
			for (auto& d : dispatchers)
			{
				d.dispatch(event_t::make(value));
			}
		}

		dyn_array<dispatcher> dispatchers;
	};
	
}

#endif // !KAWA_EVENT_MANAGER
