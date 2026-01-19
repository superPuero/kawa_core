#ifndef KAWA_BROADCASTER
#define KAWA_BROADCASTER

#include <format>
#include <semaphore>
#include "core_types.h"
#include "any.h"

namespace kawa
{
	/// Generic message/event broadcasting system.
	/// Allows objects to subscribe to events of a specific type (message_type)
	/// without inheriting from a common "Listener" interface.
	template<typename message_type>
	class broadcaster
	{
	public:
		using message_t = message_type;

		broadcaster() noexcept = default;
		
		/// INTENTIONAL: Empty to prevent copying the listeners. 
		/// We dont what that bacause listeners are bound to specific memory addresses of the broadcaster.
		broadcaster(const broadcaster& other) noexcept {}		

		broadcaster(broadcaster&& other) noexcept = default;

		/// RAII helper
		/// Automatically subscribes the object on creation and unsubscribes on destruction.
		/// This prevents dangling pointers if a listener is destroyed before the broadcaster.
		struct listner
		{
			template<typename T>
			listner(T* self_, broadcaster& manager) noexcept
				: self(self_)
				, mgr(manager)
			{
				mgr.subscribe(*self_);
			}

			listner(const listner& other) noexcept = delete;
			listner(listner&& other) noexcept
				: mgr(other.mgr)
				, self(other.self)
			{
				other.self = nullptr;
			}

			~listner() noexcept
			{
				mgr.try_unsubscribe(self);
			}

			void* self;
			broadcaster& mgr;
		};

		struct _listner_backend
		{
			using recieve_invoke_fn_t = void(void*, const message_t& msg);
			void* value_ptr = nullptr;
			recieve_invoke_fn_t* recieve_invoke_fn = nullptr;

			void recieve(const message_t& msg) noexcept
			{
				recieve_invoke_fn(value_ptr, msg);
			}
		};

		template<typename value_t>
		void subscribe(value_t& val) noexcept
		{
			auto& d = _listners.emplace_back();
			d.recieve_invoke_fn =
				[](void* value_ptr, const message_t& msg)
				{
					static_cast<value_t*>(value_ptr)->recieve(msg);
				};
			d.value_ptr = &val;
		}

		template<typename value_t>
		bool try_unsubscribe(const value_t* ptr) noexcept
		{
			return try_unsubscribe(ptr);
		}

		bool try_unsubscribe(void* addr) noexcept
		{
			if (!addr) return false;

			usize i = 0;
			bool found = false;
			for (auto& d : _listners)
			{
				if (d.value_ptr == addr)
				{
					found = true;
					break;
				}
				i++;
			}

			if (found)
			{
				_listners[i] = _listners.back();
				_listners.pop_back();
			}

			return found;
		}

		void emit(const message_t& e) noexcept
		{
			for (auto& d : _listners)
			{
				d.recieve(e);
			}
		}

		dyn_array<_listner_backend> _listners;
	};		
}

#endif // !KAWA_BROADCASTER
