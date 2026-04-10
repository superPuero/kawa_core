#ifndef KAWA_ECS
#define KAWA_ECS

#define kw_component_table_growth_coef 10

#include "utils.h"

#include "core_types.h"
#include "macros.h"
#include "fast_map.h"
#include "task_manager.h"
#include "stable_tuple.h"
#include "broadcaster.h"

//#include "de_serialize.h"

namespace kawa
{
	enum entity_id : u64 { nullent = std::numeric_limits<u64>::max() };

	struct registry;

	template<typename value_t>
	struct component_construct_event
	{
		value_t& component;
		registry& registry;
		entity_id entity;
	};

	template<typename value_t>
	struct component_destruct_event
	{
		value_t& component;
		registry& registry;
		entity_id entity;
	};


	template<template<typename> typename EventT>
	struct component_event_broadcaster
	{
		using container_t = sized_any<sizeof(broadcaster<int>)>;

		using invoker_fn_t = void(container_t&, registry&, entity_id, void*);

		invoker_fn_t* invoker = nullptr;
		container_t container;

		component_event_broadcaster() noexcept = default;
		component_event_broadcaster(const component_event_broadcaster& other) noexcept = default;
		component_event_broadcaster(component_event_broadcaster&& other) noexcept = default;

		component_event_broadcaster& operator=(const component_event_broadcaster& other) noexcept = default;
		component_event_broadcaster& operator=(component_event_broadcaster&& other) noexcept = default;

		template<typename component_t>
		void refresh() noexcept
		{
			using bcaster_t = broadcaster<EventT<component_t>>;
			container.refresh<bcaster_t>();

			invoker = +[](container_t& bc, registry& r, entity_id e, void* comp)
				{
					bc.unwrap<bcaster_t>().emit({ .component = *reinterpret_cast<component_t*>(comp), .registry = r, .entity = e });
				};
		}

		void release() noexcept
		{
			invoker = nullptr;
		}

		void try_invoke(registry& r, entity_id e, void* comp) noexcept
		{
			if (invoker)
			{
				invoker(container, r, e, comp);
			}
		}
	};

	struct component_table_t
	{
		registry& _owner;

		std::unique_ptr<std::mutex> _mutex = std::make_unique<std::mutex>();

		struct event_broadcasters_t
		{
			component_event_broadcaster<component_construct_event> on_construct;
			component_event_broadcaster<component_destruct_event> on_destruct;
		} event_broadcasters;

		lifetime_vtable* _vtable = nullptr;

		usize _dense_occupied = 0;
		usize _dense_capacity = 0;

		usize _sparse_capacity = 0;

		entity_id* _entries = 0; // dense

		usize* _indirection = 0; // sparse

		bool* _mask = nullptr; // sparse 
		void* _data = nullptr;

		template<typename T>
		component_table_t(construct_tag<T>, registry& owner) noexcept : _owner(owner) { refresh<T>(); };

		component_table_t& operator=(const component_table_t& other)
		{
			if (this != &other)
			{
				_vtable = other._vtable;

				if (_vtable)
				{
					event_broadcasters = other.event_broadcasters;
					_dense_occupied = other._dense_occupied;
					_dense_capacity = other._dense_capacity;
					_sparse_capacity = other._sparse_capacity;

					_entries = new entity_id[_dense_capacity]{};
					memcpy(_entries, other._entries, _dense_capacity * sizeof(_entries[0]));

					_indirection = new usize[_sparse_capacity]{};
					_mask = new bool[_sparse_capacity] {};
					memcpy(_indirection, other._indirection, _sparse_capacity * sizeof(_indirection[0]));
					memcpy(_mask, other._mask, _sparse_capacity * sizeof(_mask[0]));

					_data = ::operator new(_dense_capacity * _vtable->type_info.size, std::align_val_t{ _vtable->type_info.alignment }, std::nothrow);
					kw_assert(_data);

					for (usize i = 0; i < _dense_occupied; i++)
					{
						entity_id e = _entries[i];
						usize index = _indirection[e];

						try_invoke_construct_callback(e, _vtable->with_offset(_data, index));
						_vtable->copy_ctor_offset(other._data, index, _data, index);
					}
				}
			}
			return *this;
		}

		component_table_t& operator=(component_table_t&& other) noexcept
		{
			if (this != &other)
			{
				release();

				_vtable = other._vtable;

				if (_vtable)
				{
					event_broadcasters = std::move(other.event_broadcasters);
					_dense_occupied = other._dense_occupied;
					_dense_capacity = other._dense_capacity;
					_sparse_capacity = other._sparse_capacity;
					_entries = other._entries;
					_mask = other._mask;
					_indirection = other._indirection;
					_data = other._data;

					other._vtable = nullptr;
					other._dense_occupied = 0;
					other._dense_capacity = 0;
					other._sparse_capacity = 0;
					other._entries = nullptr;
					other._mask = nullptr;
					other._indirection = nullptr;
					other._data = nullptr;
				}

			}
			return *this;
		}


		component_table_t(const component_table_t& other) noexcept
			: _owner(other._owner)
		{
			*this = other;
		}

		component_table_t(component_table_t&& other) noexcept
			: _owner(other._owner)
		{
			*this = std::move(other);
		}

		~component_table_t() noexcept
		{
			release();
		}

		void try_invoke_construct_callback(entity_id e, void* ptr) noexcept
		{
			event_broadcasters.on_construct.try_invoke(_owner, e, ptr);
		}

		void try_invoke_destruct_callback(entity_id e, void* ptr) noexcept
		{
			event_broadcasters.on_destruct.try_invoke(_owner, e, ptr);
		}

		void release() noexcept
		{
			if (_vtable)
			{
				for (usize i = 0; i < _dense_occupied; i++)
				{
					entity_id e = _entries[i];
					usize index = _indirection[e];

					try_invoke_destruct_callback(e, _vtable->with_offset(_data, index));
					_vtable->dtor_offset(_data, index);
				}

				_dense_occupied = 0;
				_dense_capacity = 0;
				_sparse_capacity = 0;

				::operator delete(_data, std::align_val_t{ _vtable->type_info.alignment });
				delete[] _mask;
				delete[] _entries;
				delete[] _indirection;

				_vtable = nullptr;
				_mask = nullptr;
				_entries = nullptr;
				_indirection = nullptr;
			}
		}

		template<typename T, typename...Args>
		T& emplace(entity_id id, Args&&...args) noexcept
		{
			ensure_sparse(id);
			try_expand_dense();

			bool& cell = _mask[id];

			if (cell)
			{
				erase(id);
			}

			cell = true;

			auto value = new (static_cast<u8*>(_data) + (_vtable->type_info.size * _dense_occupied)) T(kw_fwd(args)...);

			usize index = _dense_occupied++;

			try_invoke_construct_callback(id, _vtable->with_offset(_data, index));

			_indirection[id] = index;
			_entries[index] = id;

			return *value;
		}

		void reserve(usize size) noexcept
		{
			if (_dense_capacity < size)
			{
				void* new_data = ::operator new(
					size * _vtable->type_info.size,
					std::align_val_t{ _vtable->type_info.alignment },
					std::nothrow
					);

				auto new_entries = new entity_id[size]{};
				memcpy(new_entries, _entries, _dense_capacity * sizeof(_entries[0]));
				delete[] _entries;

				_entries = new_entries;

				kw_assert(new_data);

				if (_vtable->is_move_construicable())
				{
					for (usize i = 0; i < _dense_capacity; i++)
					{
						entity_id e = _entries[i];
						usize index = _indirection[e];
						_vtable->move_ctor_offset(_data, index, new_data, index);
						_vtable->dtor_offset(_data, index);
					}
				}
				else
				{
					for (usize i = 0; i < _dense_capacity; i++)
					{
						entity_id e = _entries[i];
						usize index = _indirection[e];
						_vtable->copy_ctor_offset(_data, index, new_data, index);
						_vtable->dtor_offset(_data, index);
					}
				}

				::operator delete(_data, std::align_val_t{ _vtable->type_info.alignment });

				_dense_capacity = size;
				_data = new_data;
			}
		}

		template<typename T>
		T& get(entity_id id) noexcept
		{
			ensure_sparse(id);
			kw_assert(_mask[id]);
			return *(static_cast<T*>(_data) + _indirection[id]);
		}

		template<typename T>
		const T& get(entity_id id) const noexcept
		{
			ensure_sparse(id);
			kw_assert(_mask[id]);
			return *(static_cast<const T*>(_data) + _indirection[id]);
		}

		template<typename T>
		T* try_get(entity_id id) noexcept
		{
			ensure_sparse(id);
			if (_mask[id])
			{
				return (static_cast<T*>(_data) + _indirection[id]);
			}
			return nullptr;
		}

		template<typename T>
		const T* try_get(entity_id id) const noexcept
		{
			if (_mask[id])
			{
				return (static_cast<const T*>(_data) + _indirection[id]);
			}
			return nullptr;
		}

		bool contains(entity_id id) const noexcept
		{
			if (id >= _sparse_capacity) return false;
			return _mask[id];
		}

		void move(entity_id from, entity_id to) noexcept
		{
			if (from == to) return;
			ensure_sparse(from);
			ensure_sparse(to);

			if (_mask[from])
			{
				usize from_index = _indirection[from];
				usize to_index = _indirection[to];

				bool& cell = _mask[to];
				if (cell)
				{
					try_invoke_destruct_callback(to, _vtable->with_offset(_data, to_index));
					_vtable->dtor_offset(_data, to_index);
				}

				_vtable->move_ctor_offset(_data, from_index, _data, to_index);
				try_invoke_construct_callback(to, _vtable->with_offset(_data, to_index));

				erase(from);
			}
		}

		void copy(entity_id from, entity_id to) noexcept
		{
			if (from == to) return;
			ensure_sparse(from);
			ensure_sparse(to);

			if (_mask[from])
			{
				usize from_index = _indirection[from];
				usize to_index = _indirection[to];

				bool& cell = _mask[to];
				if (cell)
				{
					try_invoke_destruct_callback(to, _vtable->with_offset(_data, to_index));
					_vtable->dtor_offset(_data, to_index);
				}

				_vtable->copy_ctor_offset(_data, from_index, _data, to_index);
				try_invoke_construct_callback(to, _vtable->with_offset(_data, to_index));
			}
		}

		void erase(entity_id id)
		{
			ensure_sparse(id);

			kw_assert(_mask[id]);

			bool& cell = _mask[id];
			if (cell)
			{
				usize _indirect_index = _indirection[id];

				_dense_occupied--;
				_entries[_indirect_index] = _entries[_dense_occupied];

				try_invoke_destruct_callback(id, _vtable->with_offset(_data, _indirect_index));
				_vtable->dtor_offset(_data, _indirect_index);

				if (_indirect_index != _dense_occupied)
				{
					if (_vtable->is_move_construicable())
					{
						_vtable->move_ctor_offset(_data, _dense_occupied, _data, _indirect_index);
					}
					else
					{
						_vtable->copy_ctor_offset(_data, _dense_occupied, _data, _indirect_index);
					}

					try_invoke_destruct_callback(id, _vtable->with_offset(_data, _dense_occupied));
					_vtable->dtor_offset(_data, _dense_occupied);
				}

				_indirection[_indirection[_dense_occupied]] = _indirect_index;

				cell = false;
			}
		}

		void try_expand_dense() noexcept
		{
			if (_dense_capacity == 0)
			{
				_dense_capacity = 1;

				_data = ::operator new(
					_dense_capacity * _vtable->type_info.size,
					std::align_val_t{ _vtable->type_info.alignment },
					std::nothrow
					);
				_entries = new entity_id[_dense_capacity];
			}
			else if (_dense_capacity == _dense_occupied)
			{
				reserve(_dense_capacity * kw_component_table_growth_coef);
			}
		}

		void ensure_sparse(u64 index) noexcept
		{
			if (_sparse_capacity <= index + 1)
			{
				// @TODO: change this garbage
				//u64 new_sparse_capacity = index + 1;
				u64 new_sparse_capacity = !index ? 1 : _sparse_capacity * kw_component_table_growth_coef;
				auto new_indirection = new usize[new_sparse_capacity]{};
				auto new_mask = new bool[new_sparse_capacity] {};

				memcpy(new_indirection, _indirection, _sparse_capacity * sizeof(_indirection[0]));
				memcpy(new_mask, _mask, _sparse_capacity * sizeof(_mask[0]));

				delete[] _indirection;
				delete[] _mask;

				_indirection = new_indirection;
				_mask = new_mask;
				_sparse_capacity = new_sparse_capacity;
			}
		}

		usize occupied() const noexcept
		{
			return _dense_occupied;
		}

		template<typename T>
		struct table_iterator_t
		{
			table_iterator_t(const component_table_t& s) noexcept : self(s) {}

			T* begin()
			{
				return static_cast<T*>(self._data);
			}

			T* end()
			{
				return (static_cast<T*>(self._data) + self._dense_occupied);
			}

			const component_table_t& self;
		};

		template<typename T>
		table_iterator_t<T> table_iterator() noexcept
		{
			kw_assert(_vtable);
			kw_assert(_vtable->type_info.is<T>());
			return { *this };
		}

		template<typename T>
		table_iterator_t<const T> table_iterator() const noexcept
		{
			kw_assert(_vtable);
			kw_assert(_vtable->type_info.is<T>());
			return { *this };
		}

		struct entries_iterator_t
		{
			entries_iterator_t(const component_table_t& s) noexcept : self(s) {}

			entity_id* begin() const noexcept
			{
				return self._entries;
			}

			entity_id* end() const noexcept
			{
				return self._entries + self._dense_occupied;
			}

			const component_table_t& self;
		};

		entries_iterator_t entries_iterator() const noexcept
		{
			return { *this };
		}

		template<typename T>
		void refresh() noexcept
		{
			_vtable = &lifetime_vtable::get<T>();
			event_broadcasters.on_construct.refresh<T>();
			event_broadcasters.on_destruct.refresh<T>();
		}
	};


	struct with_entity_id
	{
		template<std::invocable<entity_id> Fn>
		with_entity_id(Fn&& fn)
			: func(kw_fwd(fn))
		{

		}

		void invoke(entity_id id)
		{
			func(id);
		}

		std::function<void(entity_id)> func;
	};

	static bool is_valid(entity_id id) noexcept
	{
		return id != nullent;
	}

	struct entity_view
	{
		entity_view(entity_id* ptr, u64 c)
			: storage(ptr)
			, count(c)
		{

		}

		entity_view(entity_id entity)
			: storage(std::bit_cast<entity_id*>(entity))
			, count(~((u64)0))
		{

		}

		entity_view(dyn_array<entity_id>& arr)
			: storage(arr.data())
			, count(arr.size())
		{

		}

		bool inplace_unary() const noexcept
		{
			return count == ~((u64)0);
		}

		entity_id get_unary() const noexcept
		{
			return std::bit_cast<entity_id>(storage);
		}

		entity_id* begin() const noexcept
		{
			return storage;
		}

		entity_id* end() const noexcept
		{
			return storage + count;
		}

		entity_id* storage = nullptr;
		u64 count = 0;
	};

	struct component_info
	{
		type_info& type_info;
		void* value_ptr;
	};

	struct _opaque_callback_wrap
	{
		using invoker_fn_t = void(unsized_any&, usize, void*);

		unsized_any _storage;
		invoker_fn_t* invoker = nullptr;

		_opaque_callback_wrap() noexcept = default;
		_opaque_callback_wrap(const _opaque_callback_wrap& other) noexcept = default;
		_opaque_callback_wrap(_opaque_callback_wrap& other) noexcept = default;

		_opaque_callback_wrap& operator=(const _opaque_callback_wrap& other) noexcept = default;
		_opaque_callback_wrap& operator=(_opaque_callback_wrap& other) noexcept = default;

		template<typename Fn, typename...Args>
		void refresh(Args&&...args)
		{
			static_assert(std::is_same_v<typename function_traits<Fn>::template arg_at<0>, entity_id>, "lifetime hook callbacks require first parameter to be entity id");

			using T = std::remove_cvref_t<typename function_traits<Fn>::template arg_at<1>>;

			_storage.refresh<Fn>(std::forward<Args>(args)...);
			invoker = +[](unsized_any& fn, usize e, void* comp)
				{
					fn.unwrap<Fn>()(e, *reinterpret_cast<T*>(comp));
				};
		}

		void release()
		{
			_storage.release();
			invoker = nullptr;
		}

		void try_invoke(usize index, void* comp) noexcept
		{
			if (invoker)
			{
				invoker(_storage, index, comp);
			}
		}
	};

	struct component_construct_broadcaster
	{
		using container_t = sized_any<sizeof(broadcaster<int>)>;

		using invoker_fn_t = void(container_t&, usize, void*);

		invoker_fn_t* invoker = nullptr;
		container_t bcaster;

		component_construct_broadcaster() noexcept = default;
		component_construct_broadcaster(const component_construct_broadcaster& other) noexcept = default;
		component_construct_broadcaster(component_construct_broadcaster&& other) noexcept = default;

		component_construct_broadcaster& operator=(const component_construct_broadcaster& other) noexcept = default;
		component_construct_broadcaster& operator=(component_construct_broadcaster&& other) noexcept = default;

		template<typename value_t>
		void refresh()
		{
			using bcaster_t = broadcaster<component_construct_event<value_t>>;
			bcaster.refresh<bcaster_t>();

			invoker = +[](container_t& bc, usize e, void* comp)
				{

					bc.unwrap<bcaster_t>().emit({ .component = *reinterpret_cast<value_t*>(comp), .entity = (entity_id)e });
				};
		}

		void release()
		{
			invoker = nullptr;
		}

		void try_invoke(usize index, void* comp) noexcept
		{
			if (invoker)
			{
				invoker(bcaster, index, comp);
			}
		}
	};


	struct component_destruct_broadcaster
	{
		using container_t = sized_any<sizeof(broadcaster<int>)>;

		using invoker_fn_t = void(container_t&, usize, void*);

		invoker_fn_t* invoker = nullptr;
		container_t bcaster;

		component_destruct_broadcaster() noexcept = default;
		component_destruct_broadcaster(const component_destruct_broadcaster& other) noexcept = default;
		component_destruct_broadcaster(component_destruct_broadcaster&& other) noexcept = default;

		component_destruct_broadcaster& operator=(const component_destruct_broadcaster& other) noexcept = default;
		component_destruct_broadcaster& operator=(component_destruct_broadcaster&& other) noexcept = default;

		template<typename value_t>
		void refresh()
		{
			using bcaster_t = broadcaster<component_destruct_event<value_t>>;
			bcaster.refresh<bcaster_t>();

			invoker = +[](container_t& bc, usize e, void* comp)
				{
					bc.unwrap<bcaster_t>().emit({ .component = *reinterpret_cast<value_t*>(comp), .entity = (entity_id)e });
				};
		}

		void release()
		{
			invoker = nullptr;
		}

		void try_invoke(usize index, void* comp) noexcept
		{
			if (invoker)
			{
				invoker(bcaster, index, comp);
			}
		}
	};

	struct entity_destroyed_signal
	{
		entity_id e;
	};

	struct entity_added_signal
	{
		entity_id e;
	};

	struct registry
	{
		struct defer_buffer
		{
			struct config
			{
				registry& registry;
				bool flush_on_dtor = true;
				bool fifo = true;
			};
			defer_buffer(const config& cfg) noexcept : _cfg(cfg) {}
			defer_buffer(defer_buffer&& other) noexcept = default;

			~defer_buffer()
			{
				if (_cfg.flush_on_dtor)
				{
					flush();
				}
			}

			template<typename T>
			defer_buffer& add(entity_id index, T&& v)
			{
				tasks.emplace_back(
					[=, this, v = std::forward<T>(v)]() mutable
					{
						_cfg.registry.add(index, std::forward<T>(v));
					}
				);

				return *this;
			}

			template<typename T, typename...Args>
			defer_buffer& emplace(entity_id index, Args&&...args)
			{
				tasks.emplace_back(
					[=, this, ...args = std::forward<Args>(args)]() mutable
					{
						_cfg.registry.emplace<T>(index, std::forward<Args>(args)...);
					}
				);

				return *this;
			}

			template<typename...Args>
			defer_buffer& erase(entity_id e)
			{
				tasks.emplace_back(
					[=, this]() mutable
					{
						_cfg.registry.erase<Args...>(e);
					}
				);

				return *this;
			}

			template<typename...Args>
			defer_buffer& copy(entity_id from, entity_id to)
			{
				tasks.emplace_back(
					[=, this]() mutable
					{
						_cfg.registry.copy<Args...>(from, to);
					}
				);

				return *this;
			}

			template<typename...Args>
			defer_buffer& move(entity_id from, entity_id to)
			{
				tasks.emplace_back(
					[=, this]() mutable
					{
						_cfg.registry.move<Args...>(from, to);
					}
				);

				return *this;
			}

			defer_buffer& clone(entity_id from, entity_id to)
			{
				tasks.emplace_back(
					[=, this]() mutable
					{
						_cfg.registry.clone(from, to);
					}
				);

				return *this;
			}

			defer_buffer& clone(entity_id from)
			{
				tasks.emplace_back(
					[=, this]() mutable
					{
						_cfg.registry.clone(from);
					}
				);

				return *this;
			}

			defer_buffer& destroy(entity_id id)
			{
				tasks.emplace_back(
					[=, this]() mutable
					{
						_cfg.registry.destroy(id);
					}
				);

				return *this;
			}

			void flush()
			{
				if (_cfg.fifo)
				{
					for (auto& t : tasks)
					{
						t();
					}
				}
				else
				{
					for (auto& t : std::views::reverse(tasks))
					{
						t();
					}
				}

				tasks.clear();
			}

			dyn_array<task_fn> tasks;
			config _cfg;
		};

		struct pipeline
		{
			using self_t = pipeline;

			registry& w;

			pipeline(registry& reg)
				: w(reg)
			{

			}

			self_t& from_view(const entity_view& view)
			{
				if (main.size() < view.count)
				{
					main.resize(view.count);
				}

				memcpy(main.data(), view.storage, view.count * sizeof(entity_id));

				return *this;
			}

			self_t& reset()
			{
				main.clear();
				return *this;
			}

			self_t& all()
			{
				w.query_filter(main, []() {return true; });
				return *this;
			}

			self_t& filter(auto&& filter_fn)
			{
				_temp.clear();
				w.query_filter(main, _temp, kw_fwd(filter_fn));
				std::swap(main, _temp);
				return *this;
			}

			template<typename T>
			self_t& with()
			{
				filter([](T&) { return true; });
				return *this;
			}

			self_t& each(auto&& apply_fn)
			{
				return *this;
			}

			entity_view as_view()
			{
				return { main };
			}

			void run()
			{

			}

			dyn_array<entity_id> _temp;
			dyn_array<entity_id> main;
		};

		struct config
		{
			string name = "unnamed";
			usize max_component_count = 128;
		};

		registry(const config& cfg) noexcept
			: _cfg(cfg)
			, _tables({ .capacity = cfg.max_component_count, .collision_depth = 16 })
			, _entries(construct_tag<entity_id>{}, * this)
			//, _free_list(cfg.max_entity_count)
			//, _entries(cfg.max_entity_count)
		{
		}

		registry(const registry& other) noexcept = default;
		registry(registry&& other) noexcept = default;

		registry& operator=(const registry& other) noexcept = default;
		registry& operator=(registry&& other) noexcept = default;


		usize entity_count() noexcept
		{
			//return _entries.occupied();
		}

		template<typename...Args>
		void ensure() noexcept
		{
			((table<Args>()), ...);
		}

		template<typename...Args>
		entity_id entity(Args&&...args) noexcept
		{
			entity_id id;

			if (!_free_list.empty())
			{
				id = _free_list.back();
				_free_list.pop_back();
			}
			else
			{
				id = (entity_id)_id_counter++;
			}

			if (is_valid(id))
			{
				_entries.emplace<entity_id>(id, id);
				((add(id, std::forward<Args>(args))), ...);
			}

			return id;
		}

		template<typename Fn>
		entity_id entity_with(Fn&& func) noexcept
		{
			entity_id id = entity();

			kw_fwd(func)(id);

			return id;
		}

		template<typename T>
		struct is_optional_arg
		{
			constexpr static bool value = std::is_pointer_v<T>;
		};

		template<typename T>
		struct is_required_arg
		{
			constexpr static  bool value = std::is_reference_v<T>;
		};

		template<typename T>
		struct _not_entity_id : std::negation<std::is_same<entity_id, T>> {};

		template<typename Fn>
		struct query_traits
		{
			using dirty_args = function_traits<Fn>::args_tuple;
			using clear_args = transform_each_t<std::remove_cvref_t, transform_each_t<std::remove_pointer_t, dirty_args>>;

			using require_args = filter_tuple_t<_not_entity_id, filter_tuple_t<is_required_arg, dirty_args>>;
			using clean_require_args = transform_each_t<std::remove_cvref_t, require_args>;

			constexpr static bool has_required_components = std::tuple_size_v<clean_require_args> > 0;
		};

		template<std::invocable<entity_id, component_info> Fn>
		void query_info(Fn&& info_func)
		{
			for (auto e : _entries.entries_iterator())
			{
				for (auto& s : _tables.values)
				{
					if (s.contains(e))
					{
						info_func((entity_id)e, component_info{ s._vtable->type_info, ((u8*)s._data) + s._vtable->type_info.size * e });
					}
				}
			}
		}

		template<typename Fn>
		void query_info_with(const entity_view& view, Fn&& info_func)
		{
			if (view.inplace_unary())
			{
				entity_id id = view.get_unary();
				query_info_with(entity_view{ &id, 1 }, kw_fwd(info_func));
			}
			else
			{
				for (auto e : view)
				{					
					for (auto& s : _tables.values)
					{
						if (!s.contains(e)) continue;

						info_func(component_info{ s._vtable->type_info, ((u8*)s._data) + s._vtable->type_info.size * e });
					}
				}
			}
		}

		template<typename T>
		struct lock_guard_t
		{
			T lock_tables;

			~lock_guard_t()
			{
				std::apply([](auto&...v) {((v._mutex->unlock()), ...); }, lock_tables);
			}
		};

		template<typename...Args>
		auto lock_guard()
		{
			auto lock_tables = std::tie(table<Args>()...);

			if constexpr (sizeof...(Args) > 1)
			{
				std::apply([](auto&...v) {std::lock((*v._mutex)...); }, lock_tables);
			}
			else
			{
				std::apply([](auto&...v) { ((v._mutex->lock()), ...); }, lock_tables);
			}

			return lock_guard_t{ lock_tables };
		}

		template<typename...Args>
		void lock()
		{
			auto lock_tables = std::tie(table<Args>()...);

			if constexpr (sizeof...(Args) > 1)
			{
				std::apply([](auto&...v) {std::lock((*v._mutex)...); }, lock_tables);
			}
			else
			{
				std::apply([](auto&...v) { ((v._mutex->lock()), ...); }, lock_tables);
			}
		}

		template<typename...Args>
		void unlock()
		{
			auto lock_tables = std::tie(table<Args>()...);
			std::apply([](auto&...v) {((v.unlock()), ...); }, lock_tables);
		}

		template<typename Fn>
		void query(Fn&& func)
		{
			using q = query_traits<Fn>;

			query_infer<Fn,
				typename q::dirty_args,
				typename q::clean_require_args
			>(
				std::make_index_sequence<std::tuple_size_v<typename q::dirty_args>>{},
				std::make_index_sequence<std::tuple_size_v<typename q::clean_require_args>>{},
				std::forward<Fn>(func)
			);
		}


		template<
			typename Fn,
			typename dirty_args_tuple,
			typename require_tuple,
			usize...args_idxs,
			usize...require_idxs
		>
		void query_infer(
			std::index_sequence<args_idxs...>,
			std::index_sequence<require_idxs...>,
			Fn&& func
		) {
			auto getters = std::make_tuple(
				_make_query_param_getter<std::tuple_element_t<args_idxs, dirty_args_tuple>>(
					*this
				)()...
			);

			array<component_table_t*, sizeof...(require_idxs)> required_tables = {
				&table<std::tuple_element_t<require_idxs, require_tuple>>()...
			};

			if constexpr (sizeof...(require_idxs))
			{
				usize driver_index = 0;

				for (usize i = 0; i < required_tables.size(); i++)
				{
					if (required_tables[i]->_dense_occupied <
						required_tables[driver_index]->_dense_occupied)
					{
						driver_index = i;
					}
				}

				component_table_t& driver = *required_tables[driver_index];

				required_tables[driver_index] = required_tables[sizeof...(require_idxs) - 1];

				array<bool*, sizeof...(require_idxs) - 1> required_storage_masks;

				for (usize i = 0; i < sizeof...(require_idxs) - 1; i++)
				{
					required_storage_masks[i] = required_tables[i]->_mask;
				}

				query_impl<Fn, required_storage_masks.size(), args_idxs...>(kw_fwd(func), entity_view{ driver._entries, driver._dense_occupied }, getters, required_storage_masks);
			}
			else
			{
				query_impl<Fn, 0, args_idxs...>(kw_fwd(func), entity_view{ (entity_id*)_entries._data, _entries.occupied() }, getters, array<bool*, 0>{});
			}

		}

		template<
			typename Fn,
			usize sz,
			usize...args_idxs
		>
		void query_impl(
			Fn&& func,
			const entity_view& driver_view,
			auto& getters,
			const array<bool*, sz>& require_masks
		)
		{
			for (auto i : driver_view)
			{
				if constexpr (sz)
				{
					if ([&]<usize...I>(std::index_sequence<I...>) {
						return (... && require_masks[I][i]);
					}(std::make_index_sequence<sz>{}))
					{
						func(
							std::get<args_idxs>(getters).get(i)...
						);
					}
				}
				else
				{
					func(
						std::get<args_idxs>(getters).get(i)...
					);
				}
			}
		}

		template<typename Fn>
		void query_until(Fn&& func)
		{
			using q = query_traits<Fn>;

			query_until_infer<Fn,
				typename q::dirty_args,
				typename q::clean_require_args
			>(
				std::make_index_sequence<std::tuple_size_v<typename q::dirty_args>>{},
				std::make_index_sequence<std::tuple_size_v<typename q::clean_require_args>>{},
				std::forward<Fn>(func)
			);
		}


		template<
			typename Fn,
			typename dirty_args_tuple,
			typename require_tuple,
			usize...args_idxs,
			usize...require_idxs
		>
		void query_until_infer(
			std::index_sequence<args_idxs...>,
			std::index_sequence<require_idxs...>,
			Fn&& func
		) {
			auto getters = std::make_tuple(
				_make_query_param_getter<std::tuple_element_t<args_idxs, dirty_args_tuple>>(
					*this
				)()...
			);

			array<component_table_t*, sizeof...(require_idxs)> required_tables = {
				&table<std::tuple_element_t<require_idxs, require_tuple>>()...
			};

			if constexpr (sizeof...(require_idxs))
			{
				usize driver_index = 0;

				for (usize i = 0; i < required_tables.size(); i++)
				{
					if (required_tables[i]->_occupied <
						required_tables[driver_index]->_occupied)
					{
						driver_index = i;
					}
				}

				component_table_t& driver = *required_tables[driver_index];

				required_tables[driver_index] = required_tables[sizeof...(require_idxs) - 1];

				array<bool*, sizeof...(require_idxs) - 1> required_storage_masks;

				for (usize i = 0; i < sizeof...(require_idxs) - 1; i++)
				{
					required_storage_masks[i] = required_tables[i]->_mask;
				}

				query_until_impl<Fn, required_storage_masks.size(), args_idxs...>(kw_fwd(func), entity_view{ (entity_id*)driver._entries, driver.occupied() }, getters, required_storage_masks);
			}
			else
			{
				query_until_impl<Fn, 0, args_idxs...>(kw_fwd(func), entity_view{ (entity_id*)_entries._data, _entries.occupied() }, getters, array<bool*, 0>{});
			}

		}

		template<
			typename Fn,
			usize sz,
			usize...args_idxs
		>
		void query_until_impl(
			Fn&& func,
			const entity_view& driver_view,
			auto& getters,
			const array<bool*, sz>& require_masks
		)
		{
			bool res = false;
			for (auto i : driver_view)
			{
				if constexpr (sz)
				{
					if ([&]<usize...I>(std::index_sequence<I...>) {
						return (... && require_masks[I][i]);
					}(std::make_index_sequence<sz>{}))
					{
						res = func(
							std::get<args_idxs>(getters).get(i)...
						);
					}
				}
				else
				{
					res = func(
						std::get<args_idxs>(getters).get(i)...
					);
				}

				if (res) break;
			}
		}

		template<typename Fn>
		void query_with(const entity_view& view, Fn&& func)
		{
			using q = query_traits<Fn>;

			entity_id unary_id = view.get_unary();
			entity_view in_view = view.inplace_unary() ? entity_view(&unary_id, 1) : view;

			query_with_infer<Fn,
				typename q::dirty_args,
				typename q::clear_args,
				typename q::clean_require_args
			>(
				std::make_index_sequence<std::tuple_size_v<typename q::dirty_args>>{},
				std::make_index_sequence<std::tuple_size_v<typename q::clean_require_args>>{},
				in_view,
				std::forward<Fn>(func)
			);
			/*if constexpr (q::has_required_components)
			{
				_query_with_required_impl<Fn,
					typename q::dirty_args,
					typename q::clear_args,
					typename q::clean_require_args
				>(
					std::make_index_sequence<std::tuple_size_v<typename q::dirty_args>>{},
					std::make_index_sequence<std::tuple_size_v<typename q::clean_require_args>>{},
					in_view,
					std::forward<Fn>(func)
				);
			}
			else
			{
				_query_with_no_required_impl<Fn,
					typename q::dirty_args,
					typename q::clear_args
				>(
					std::make_index_sequence<std::tuple_size_v<typename q::dirty_args>>{},
					in_view,
					std::forward<Fn>(func)
				);
			}*/
		}

		template<
			typename Fn,
			typename dirty_args_tuple,
			typename args_tuple,
			typename require_tuple,
			usize...args_idxs,
			usize...require_idxs
		>
		void query_with_infer(
			std::index_sequence<args_idxs...>,
			std::index_sequence<require_idxs...>,
			const entity_view& view,
			Fn&& func
		) {
			auto getters = std::make_tuple(
				_make_query_param_getter<std::tuple_element_t<args_idxs, dirty_args_tuple>>(
					*this
				)()...
			);

			array<component_table_t*, sizeof...(require_idxs)> required_tables = {
				&table<std::tuple_element_t<require_idxs, require_tuple>>()...
			};

			if constexpr (sizeof...(require_idxs))
			{
				usize driver_index = 0;

				for (usize i = 0; i < required_tables.size(); i++)
				{
					if (required_tables[i]->_dense_occupied <
						required_tables[driver_index]->_dense_occupied)
					{
						driver_index = i;
					}
				}

				component_table_t& driver = *required_tables[driver_index];

				required_tables[driver_index] = required_tables[sizeof...(require_idxs) - 1];

				if (view.count <= driver._dense_occupied)
				{
					array<bool*, sizeof...(require_idxs) - 1> required_storage_masks;

					for (usize i = 0; i < sizeof...(require_idxs) - 1; i++)
					{
						required_storage_masks[i] = required_tables[i]->_mask;
					}

					query_with_impl<Fn, required_storage_masks.size(), args_idxs...>(kw_fwd(func), view, getters, required_storage_masks);
				}
				else
				{
					array<bool*, sizeof...(require_idxs)> required_storage_masks = {
						table<std::tuple_element_t<require_idxs, require_tuple>>()._mask...
					};

					query_with_impl<Fn, required_storage_masks.size(), args_idxs...>(kw_fwd(func), entity_view{ (entity_id*)driver._entries, driver._dense_occupied }, getters, required_storage_masks);

				}
			}
			else
			{
				query_with_impl<Fn, 0, args_idxs...>(kw_fwd(func), view, getters, array<bool*, 0>{});
			}
		}

		template<
			typename Fn,
			usize sz,
			usize...args_idxs
		>
		void query_with_impl(
			Fn&& func,
			const entity_view& driver_view,
			auto& getters,
			const array<bool*, sz>& require_masks
		)
		{
			for (auto i : driver_view)
			{
				if constexpr (sz)
				{
					if ([&]<usize...I>(std::index_sequence<I...>) {
						return (... && require_masks[I][i]);
					}(std::make_index_sequence<sz>{}))
					{
						func(
							std::get<args_idxs>(getters).get(i)...
						);
					}
				}
				else
				{
					func(
						std::get<args_idxs>(getters).get(i)...
					);
				}
			}
		}

		template<typename FilterFn>
		void query_filter(dyn_array<entity_id>& out, FilterFn&& func)
		{
			using q = query_traits<FilterFn>;

			if constexpr (q::has_required_components)
			{
				_query_filter_required_impl<FilterFn,
					typename q::dirty_args,
					typename q::clear_args,
					typename q::clean_require_args
				>(
					std::make_index_sequence<std::tuple_size_v<typename q::dirty_args>>{},
					std::make_index_sequence<std::tuple_size_v<typename q::clean_require_args>>{},
					out,
					std::forward<FilterFn>(func)
				);
			}
			else
			{

				if constexpr (std::tuple_size_v<typename q::dirty_args>)
				{
					_query_filter_no_required_impl<FilterFn,
						typename q::dirty_args,
						typename q::clear_args
					>(
						std::make_index_sequence<std::tuple_size_v<typename q::dirty_args>>{},
						out,
						std::forward<FilterFn>(func)
					);
				}
				else
				{
					if (out.size() < _entries.occupied())
					{
						out.resize(_entries.occupied());
					}

					memcpy(out.data(), _entries._data, _entries.occupied() * sizeof(entity_id));
				}

			}
		}

		template<typename FilterFn>
		void query_filter_with(const entity_view& view, dyn_array<entity_id>& out, FilterFn&& func)
		{
			using q = query_traits<FilterFn>;

			entity_id unary_id = view.get_unary();
			entity_view in_view = view.inplace_unary() ? entity_view(&unary_id, 1) : view;

			if constexpr (q::has_required_components)
			{
				_query_filter_with_required_impl<FilterFn,
					typename q::dirty_args,
					typename q::clear_args,
					typename q::clean_require_args
				>(
					std::make_index_sequence<std::tuple_size_v<typename q::dirty_args>>{},
					std::make_index_sequence<std::tuple_size_v<typename q::clean_require_args>>{},
					in_view,
					out,
					std::forward<FilterFn>(func)
				);
			}
			else
			{

				if constexpr (std::tuple_size_v<typename q::dirty_args>)
				{
					_query_filter_with_no_required_impl<FilterFn,
						typename q::dirty_args,
						typename q::clear_args
					>(
						std::make_index_sequence<std::tuple_size_v<typename q::dirty_args>>{},
						in_view,
						out,
						std::forward<FilterFn>(func)
					);
				}
				else
				{
					if (out.size() < _entries.occupied())
					{
						out.resize(_entries.occupied());
					}

					memcpy(out.data(), _entries._data, _entries.occupied() * sizeof(entity_id));
				}

			}
		}

		template<typename Fn>
		void query_par(task_manager& tm, task_schedule_policy policy, usize work_groups, dyn_array<task_handle>& out_handles, Fn&& func)
		{
			using q = query_traits<Fn>;

			if constexpr (q::has_required_components)
			{
				_query_par_with_required_impl<Fn,
					typename q::dirty_args,
					typename q::clear_args,
					typename q::clean_require_args
				>(
					std::make_index_sequence<std::tuple_size_v<typename q::clear_args>>{},
					std::make_index_sequence<std::tuple_size_v<typename q::clean_require_args>>{},
					tm,
					policy,
					work_groups,
					out_handles,
					std::forward<Fn>(func)
				);
			}
			else
			{
				_query_par_with_no_required_impl<Fn,
					typename q::dirty_args,
					typename q::clear_args
				>(
					std::make_index_sequence<std::tuple_size_v<typename q::clear_args>>{},
					tm,
					policy,
					work_groups,
					out_handles,
					std::forward<Fn>(func)
				);
			}
		}

		struct _entity_id_getter
		{
			inline entity_id get(usize i) const noexcept
			{
				return (entity_id)i;
			}
		};

		template<typename T>
		struct _required_getter
		{
			T* _data;
			usize* _indirection;

			inline T& get(usize i) const noexcept
			{
				return _data[_indirection[i]];
			}
		};

		template<typename T>
		struct _optional_getter
		{
			T* _data;
			bool* _mask;
			usize sparse_size;
			usize* _indirection;

			inline T* get(usize i) const noexcept
			{
				if (i < sparse_size && _mask[i])
				{
					return _data + _indirection[i];
				}
				return nullptr;
			}
		};

		template<typename T>
		struct _make_query_param_getter
		{
			registry& r;

			auto operator()() const noexcept
			{
				using CVT = std::remove_reference_t<std::remove_pointer_t<T>>;
				using CleanT = std::remove_cv_t<CVT>;

				if constexpr (std::is_same_v<entity_id, T>)
				{
					return _entity_id_getter{};
				}
				else if constexpr (std::is_pointer_v<T>)
				{
					auto& s = r.table<CleanT>();
					return _optional_getter<CVT>{ (CVT*)s._data, s._mask, s._sparse_capacity, s._indirection};
				}
				else if constexpr (std::is_reference_v<T>)
				{
					auto& s = r.table<CleanT>();
					return _required_getter<CVT>{ (CVT*)s._data, s._indirection };
				}
			}
		};

		template<
			typename Fn,
			typename dirty_args_tuple,
			typename args_tuple,
			usize...args_idxs
		>
		void _query_filter_no_required_impl(
			std::index_sequence<args_idxs...>,
			dyn_array<entity_id>& out,
			Fn&& func
		) {
			auto getters = std::make_tuple(
				_make_query_param_getter<std::tuple_element_t<args_idxs, dirty_args_tuple>>(*this)()...
			);

			for (auto i : _entries)
			{
				if (std::forward<Fn>(func)(
					std::get<args_idxs>(getters).get(i)...
					))
				{
					out.emplace_back((entity_id)i);
				}
			}
		}

		template<
			typename Fn,
			typename dirty_args_tuple,
			typename args_tuple,
			typename require_tuple,
			usize...args_idxs,
			usize...require_idxs
		>
		void _query_filter_required_impl(
			std::index_sequence<args_idxs...>,
			std::index_sequence<require_idxs...>,
			dyn_array<entity_id>& out,
			Fn&& func
		) {
			auto getters = std::make_tuple(
				_make_query_param_getter<std::tuple_element_t<args_idxs, dirty_args_tuple>>(
					*this
				)()...
			);

			array<component_table_t*, sizeof...(require_idxs)> required_tables = {
				&table<std::tuple_element_t<require_idxs, require_tuple>>()...
			};


			usize driver_index = 0;

			for (usize i = 0; i < required_tables.size(); i++)
			{
				if (required_tables[i]->_occupied <
					required_tables[driver_index]->_occupied)
				{
					driver_index = i;
				}
			}

			component_table_t& driver = *required_tables[driver_index];

			required_tables[driver_index] = required_tables[sizeof...(require_idxs) - 1];

			array<bool*, sizeof...(require_idxs) - 1> required_storage_masks;

			for (usize i = 0; i < sizeof...(require_idxs) - 1; i++)
			{
				required_storage_masks[i] = required_tables[i]->_mask;
			}

			for (auto i : driver)
			{
				if ([&]<usize...I>(std::index_sequence<I...>) {
					return (... && required_storage_masks[I][i]);
				}(std::make_index_sequence<sizeof...(require_idxs) - 1>{}))
				{
					if (func(
						std::get<args_idxs>(getters).get(i)...
					))
					{
						out.emplace_back((entity_id)i);
					}
				}
			}
		}

		template<
			typename Fn,
			typename dirty_args_tuple,
			typename args_tuple,
			usize...args_idxs
		>
		void _query_filter_with_no_required_impl(
			std::index_sequence<args_idxs...>,
			const entity_view& view,
			dyn_array<entity_id>& out,
			Fn&& func
		) {
			auto getters = std::make_tuple(
				_make_query_param_getter<std::tuple_element_t<args_idxs, dirty_args_tuple>>(*this)()...
			);

			for (auto i : view)
			{
				if (std::forward<Fn>(func)(
					std::get<args_idxs>(getters).get(i)...
					))
				{
					out.emplace_back((entity_id)i);
				}
			}
		}

		template<
			typename Fn,
			typename dirty_args_tuple,
			typename args_tuple,
			typename require_tuple,
			usize...args_idxs,
			usize...require_idxs
		>
		void _query_filter_with_required_impl(
			std::index_sequence<args_idxs...>,
			std::index_sequence<require_idxs...>,
			const entity_view& view,
			dyn_array<entity_id>& out,
			Fn&& func
		) {
			auto getters = std::make_tuple(
				_make_query_param_getter<std::tuple_element_t<args_idxs, dirty_args_tuple>>(
					*this
				)()...
			);

			array<component_table_t*, sizeof...(require_idxs)> required_tables = {
				&table<std::tuple_element_t<require_idxs, require_tuple>>()...
			};

			array<bool*, sizeof...(require_idxs)> required_storage_masks;

			for (usize i = 0; i < sizeof...(require_idxs); i++)
			{
				required_storage_masks[i] = required_tables[i]->_mask;
			}

			for (auto i : view)
			{
				if ([&]<usize...I>(std::index_sequence<I...>) {
					return (... && required_storage_masks[I][i]);
				}(std::make_index_sequence<sizeof...(require_idxs)>{}))
				{
					if (func(
						std::get<args_idxs>(getters).get(i)...
					))
					{
						out.emplace_back((entity_id)i);
					}
				}
			}
		}


		template<
			typename Fn,
			typename dirty_args_tuple,
			typename args_tuple,
			usize...args_idxs
		>
		void _query_with_no_required_impl(
			std::index_sequence<args_idxs...>,
			const entity_view& view,
			Fn&& func
		) {
			auto getters = std::make_tuple(
				_make_query_param_getter<std::tuple_element_t<args_idxs, dirty_args_tuple>>(*this)()...
			);

			for (auto i : view)
			{
				std::forward<Fn>(func)(
					std::get<args_idxs>(getters).get(i)...
					);
			}

		}

		template<
			typename Fn,
			typename dirty_args_tuple,
			typename args_tuple,
			typename require_tuple,
			usize...args_idxs,
			usize...require_idxs
		>
		void _query_with_required_impl(
			std::index_sequence<args_idxs...>,
			std::index_sequence<require_idxs...>,
			const entity_view& view,
			Fn&& func
		) {
			auto getters = std::make_tuple(
				_make_query_param_getter<std::tuple_element_t<args_idxs, dirty_args_tuple>>(
					*this
				)()...
			);

			//array<component_table_t*, sizeof...(require_idxs)> required_tables = {
			//	&table<std::tuple_element_t<require_idxs, require_tuple>>()...
			//};

			array<bool*, sizeof...(require_idxs)> required_storage_masks = {
				table<std::tuple_element_t<require_idxs, require_tuple>>()._mask...
			};

			/*for (usize i = 0; i < sizeof...(require_idxs); i++)
			{
				required_storage_masks[i] = required_tables[i]->_mask;
			}*/

			for (auto i : view)
			{
				if ([&]<usize...I>(std::index_sequence<I...>) {
					return (... && required_storage_masks[I][i]);
				}(std::make_index_sequence<sizeof...(require_idxs)>{}))
				{
					func(
						std::get<args_idxs>(getters).get(i)...
					);
				}
			}
		}

		template<
			typename Fn,
			typename dirty_args_tuple,
			typename args_tuple,
			usize...args_idxs
		>
		void _query_no_required_impl(
			std::index_sequence<args_idxs...>,
			Fn&& func
		) {
			auto getters = std::make_tuple(
				_make_query_param_getter<std::tuple_element_t<args_idxs, dirty_args_tuple>>(*this)()...
			);

			for (auto i : _entries)
			{
				std::forward<Fn>(func)(
					std::get<args_idxs>(getters).get(i)...
					);
			}
		}

		template<
			typename Fn,
			typename dirty_args_tuple,
			typename args_tuple,
			typename require_tuple,
			usize...args_idxs,
			usize...require_idxs
		>
		void _query_required_impl(
			std::index_sequence<args_idxs...>,
			std::index_sequence<require_idxs...>,
			Fn&& func
		) {
			auto getters = std::make_tuple(
				_make_query_param_getter<std::tuple_element_t<args_idxs, dirty_args_tuple>>(
					*this
				)()...
			);

			array<component_table_t*, sizeof...(require_idxs)> required_tables = {
				&table<std::tuple_element_t<require_idxs, require_tuple>>()...
			};

			usize driver_index = 0;

			for (usize i = 0; i < required_tables.size(); i++)
			{
				if (required_tables[i]->_occupied <
					required_tables[driver_index]->_occupied)
				{
					driver_index = i;
				}
			}

			component_table_t& driver = *required_tables[driver_index];

			required_tables[driver_index] = required_tables[sizeof...(require_idxs) - 1];

			array<bool*, sizeof...(require_idxs) - 1> required_storage_masks;

			for (usize i = 0; i < sizeof...(require_idxs) - 1; i++)
			{
				required_storage_masks[i] = required_tables[i]->_mask;
			}

			for (auto i : driver)
			{
				if ([&]<usize...I>(std::index_sequence<I...>) {
					return (... && required_storage_masks[I][i]);
				}(std::make_index_sequence<sizeof...(require_idxs) - 1>{}))
				{
					func(
						std::get<args_idxs>(getters).get(i)...
					);
				}
			}
		}

		template<
			typename Fn,
			typename dirty_args_tuple,
			typename args_tuple,
			typename require_tuple,
			usize...args_idxs,
			usize...require_idxs
		>
		void _query_par_with_required_impl(
			std::index_sequence<args_idxs...>,
			std::index_sequence<require_idxs...>,
			task_manager& mgr,
			task_schedule_policy policy,
			usize work_gouprs,
			dyn_array<task_handle>& out_handles,
			Fn&& func
		) {
			auto getters = std::make_tuple(
				_make_query_param_getter<std::tuple_element_t<args_idxs, dirty_args_tuple>>(
					*this
				)()...
			);
			array<component_table_t*, sizeof...(require_idxs)> required_tables = { &table<std::tuple_element_t<require_idxs, require_tuple>>()... };

			usize driver_index = 0;

			for (usize i = 0; i < required_tables.size(); i++)
			{
				if (required_tables[i]->_dense_occupied < required_tables[driver_index]->_dense_occupied)
				{
					driver_index = i;
				}
			}

			component_table_t* driver = required_tables[driver_index];

			required_tables[driver_index] = required_tables[sizeof...(require_idxs) - 1];

			array<bool*, sizeof...(require_idxs) - 1> required_storage_masks;

			for (usize i = 0; i < sizeof...(require_idxs) - 1; i++)
			{
				required_storage_masks[i] = required_tables[i]->_mask;
			}

			usize work_reminder = driver->_dense_occupied % work_gouprs;
			usize work_per_group = driver->_dense_occupied / work_gouprs;

			for (usize group_id = 0; group_id < work_gouprs; group_id++)
			{
				usize current_group_work = ((group_id == work_gouprs - 1) ? work_reminder + work_per_group : work_per_group);
				out_handles.emplace_back(
					mgr.schedule(
						[=]()
						{
							auto map = driver->_entries;
							usize i;
							for (usize e = 0; e < current_group_work; e++)
							{
								i = map[(group_id * work_per_group) + e];

								if ([&]<usize...I>(std::index_sequence<I...>)
								{
									return (... && required_storage_masks[I][i]);
								}
								(std::make_index_sequence<sizeof...(require_idxs) - 1>{}))
								{
									func(
										std::get<args_idxs>(getters).get(i)...
									);
								}
							}
						}
						, policy
					)
				);
			}
		}

		template<
			typename Fn,
			typename dirty_args_tuple,
			typename args_tuple,
			usize...args_idxs
		>
		void _query_par_with_no_required_impl(
			std::index_sequence<args_idxs...>,
			task_manager& mgr,
			task_schedule_policy policy,
			usize work_gouprs,
			dyn_array<task_handle>& out_handles,
			Fn&& func
		) {
			auto getters = std::make_tuple(
				_make_query_param_getter<std::tuple_element_t<args_idxs, dirty_args_tuple>>(
					*this
				)()...
			);
			usize work_reminder = _entries.occupied() % work_gouprs;
			usize work_per_group = _entries.occupied() / work_gouprs;

			dyn_array<defer_buffer> defer_buffers;
			defer_buffers.reserve(work_gouprs);

			for (usize group_id = 0; group_id < work_gouprs; group_id++)
			{
				out_handles.emplace_back(
					mgr.schedule(
						[=, this]()
						{
							auto map = _entries._data;

							for (usize e = 0; e < (group_id == work_gouprs - 1) ? work_reminder + work_per_group : work_per_group; e++)
							{
								auto i = map[(group_id + work_per_group) + e];

								func(
									std::get<args_idxs>(getters).get(i)...
								);
							}
						}
						, policy
					)
				);
			}
		}

		template<typename Fn>
		void on_construct(Fn&& func)
		{
			static_assert(std::is_same_v<typename function_traits<Fn>::template arg_at<0>, entity_id>, "lifetime hook callbacks require first parameter to be entity id");

			using T = std::remove_cvref_t<typename function_traits<Fn>::template arg_at<1>>;

			table<T>().template set_on_construct<Fn>(std::forward<Fn>(func));
		}

		template<typename Fn>
		void on_destruct(Fn&& func)
		{
			static_assert(std::is_same_v<typename function_traits<Fn>::template arg_at<0>, entity_id>, "lifetime hook callbacks require first parameter to be entity id");

			using T = std::remove_cvref_t<typename function_traits<Fn>::template arg_at<1>>;

			table<T>().template set_on_destruct<Fn>(std::forward<Fn>(func));
		}

		template<typename T>
		T& add(entity_id index, T&& v)
		{
			if constexpr (std::is_same_v<with_entity_id, T>)
			{
				v.invoke(index);
				return v;
			}
			else
			{
				return table<std::remove_cvref_t<T>>().template emplace<std::remove_cvref_t<T>>(index, std::forward<T>(v));
			}
		}

		template<typename T, typename...Args>
		T& try_emplace(entity_id index, Args&&...args) noexcept
		{
			if (auto v = try_get<T>(index))
			{
				return *v;
			}

			return emplace<T>(index, std::forward<Args>(args)...);
		}

		template<typename T, typename...Args>
		T& emplace(entity_id index, Args&&...args) noexcept
		{
			return table<T>().template emplace<T>(index, std::forward<Args>(args)...);
		}

		template<typename...Args>
		void erase(entity_id e) noexcept
		{
			((table<Args>().erase(e)), ...);
		}

		template<typename...Args>
		void copy(entity_id from, entity_id to) noexcept
		{
			((table<Args>().copy(from, to)), ...);
		}

		template<typename...Args>
		void move(entity_id from, entity_id to)
		{
			((table<Args>().move(from, to)), ...);
		}

		void clone(entity_id from, entity_id to) noexcept
		{
			if (!alive(from) || !alive(to))
				return;

			for (auto& s : _tables.values)
			{
				if (s.contains(from))
				{
					s.copy(from, to);
				}
			}
		}

		entity_id clone(entity_id from) noexcept
		{
			entity_id to;

			if (alive(from))
			{
				to = entity();

				if (is_valid(to))
				{
					clone(from, to);
				}
			}

			return to;
		}


		bool alive(entity_id id) noexcept
		{
			if (id == nullent) return false;
			return _entries.contains(id);
		}

		template<typename T>
		T& get(entity_id e) noexcept
		{
			return table<T>().template get<T>(e);
		}

		template<typename T>
		T* try_get(entity_id e) noexcept
		{
			return table<T>().template try_get<T>(e);
		}

		void destroy(entity_id id) noexcept
		{
			if (!_entries.contains(id)) return;

			for (auto& s : _tables.values)
			{
				s.erase(id);
			}

			_entries.erase(id);

			_free_list.emplace_back(id);
		}

		template<typename...Args>
		bool has(entity_id e) noexcept
		{
			return ((table<Args>().contains(e)) && ...);
		}

		defer_buffer defer(bool flush_on_dtor = true, bool fifo = true) noexcept
		{
			return { {*this, flush_on_dtor, fifo} };
		}

		pipeline pipeline(bool deferred = false) noexcept
		{
			return { *this };
		}

		template<typename T>
		component_table_t& table() noexcept
		{
			static int init_de_ser = [this]()
				{
					ser_map.ensure<T>();
					deser_map.ensure<T>();

					return 42;
				}();

			constexpr auto hash = type_hash<T>;

			if (auto v = _tables.try_get(hash))
			{
				return *v;
			}
			else
			{
				return _tables.insert(hash, construct_tag<T>{}, * this);
			}
		}

		template<typename T>
		broadcaster<component_construct_event<T>>& construct_broadcaster() noexcept
		{
			return table<T>().event_broadcasters.on_construct.container.template unwrap<broadcaster<component_construct_event<T>>>();
		}

		template<typename T>
		broadcaster<component_destruct_event<T>>& destruct_broadcaster() noexcept
		{
			return table<T>().event_broadcasters.on_destruct.container.template unwrap<broadcaster<component_destruct_event<T>>>();
		}

		template<typename T>
		struct serialize_trait
		{
			static void write(registry& reg, entity_id id, std::ofstream& out) {}
		};

		struct serializer_map
		{
			using fn_t = void(registry&, entity_id, std::ofstream&);
			template<typename T>
			static inline fn_t* fn = +[](registry& reg, entity_id id, std::ofstream& out)
				{
					serialize_trait<T>::write(reg, id, out);
				};

			template<typename T>
			void ensure()
			{
				serializers[type_hash<T>] = fn<T>;
			}

			umap<u64, fn_t*> serializers;
		};

		template<typename T>
		struct deserialize_trait
		{
			static void read(registry& reg, entity_id id, std::ifstream& in) {}
		};


		struct deserializer_map
		{
			using fn_t = void(registry&, entity_id, std::ifstream&);
			template<typename T>
			static inline fn_t* fn = +[](registry& reg, entity_id id, std::ifstream& in)
				{
					deserialize_trait<T>::read(reg, id, in);
				};

			template<typename T>
			void ensure()
			{
				deserializers[type_hash<T>] = fn<T>;
			}

			umap<u64, fn_t*> deserializers;
		};



		void serialize(std::ofstream& out)
		{
			for (auto e : _entries.table_iterator<entity_id>())
			{
				query_info_with((entity_id)e,
					[&](component_info info)
					{
						out << info.type_info.hash << '\n';
						ser_map.serializers[info.type_info.hash](*this, (entity_id)e, out);
					}
				);

				out << "#" << '\n';
			}
			out << "$" << '\n';

		}

		void deserialize(std::ifstream& in)
		{
			in >> std::ws;

			while (in.peek() != '$' && in.peek() != std::char_traits<char>::eof())
			{
				auto id = entity();
				in >> std::ws;

				while (in.peek() != '#' && in.peek() != '$' && in.peek() != std::char_traits<char>::eof())
				{
					u64 type_hash;
					in >> type_hash;
					deser_map.deserializers[type_hash](*this, id, in);
					in >> std::ws;
				}
				in.ignore();
				in >> std::ws;
			}

			in.ignore();
		}

		hash_map<component_table_t> _tables;
		dyn_array<entity_id> _free_list;
		component_table_t _entries;
		serializer_map ser_map; 
		deserializer_map deser_map;
		u64 _id_counter = 0;
		config _cfg;
	};
}

#endif // !KAWA_ECS
