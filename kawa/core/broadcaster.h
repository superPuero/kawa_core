#ifndef KAWA_BROADCASTER
#define KAWA_BROADCASTER

#include <format>
#include <semaphore>
#include "core_types.h"
#include "any.h"

namespace kawa
{
	template<typename message_type>
	class broadcaster
	{
	public:
		using message_t = message_type;

		struct subscribe_guard
		{
			template<typename T>
			subscribe_guard(T& self, broadcaster& manager) noexcept
				: addr((uintptr_t)&self)
				, mgr(manager)				
			{
				mgr.subscribe(self);
			}

			~subscribe_guard() noexcept
			{
				try_unsubscribe();
			}

			bool try_unsubscribe() noexcept
			{
				return mgr.try_unsubscribe(addr);
			}

			uintptr_t addr;
			broadcaster& mgr;
		};

		struct dispatcher
		{
			using dispatch_invoke_fn_t = void(void*, const message_t& msg);
			void* value_ptr = nullptr;
			dispatch_invoke_fn_t* dispatch_invoke_fn = nullptr;

			void dispatch(const message_t& msg) noexcept
			{
				dispatch_invoke_fn(value_ptr, msg);
			}
		};

		template<typename value_t>
		void subscribe(value_t& val) noexcept
		{
			auto& d = dispatchers.emplace_back();
			d.dispatch_invoke_fn =
				[](void* value_ptr, const message_t& msg)
				{
					static_cast<value_t*>(value_ptr)->dispatch(msg);
				};
			d.value_ptr = &val;
		}

		template<typename value_t>
		bool try_unsubscribe(const value_t* ptr) noexcept
		{
			return try_unsubscribe((uintptr_t)ptr);
		}

		bool try_unsubscribe(uintptr_t addr) noexcept
		{
			usize i = 0;
			bool found = false;
			for (auto& d : dispatchers)
			{
				if ((uintptr_t)d.value_ptr == addr)
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

			return found;
		}

		void emit(const message_t& e) noexcept
		{
			for (auto& d : dispatchers)
			{
				d.dispatch(e);
			}
		}

		dyn_array<dispatcher> dispatchers;
	};	
}

#endif // !KAWA_BROADCASTER
