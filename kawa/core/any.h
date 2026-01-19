#ifndef KAWA_ANY
#define KAWA_ANY

#include <concepts>
#include "macros.h"
#include "meta.h"

namespace kawa
{
	struct lifetime_vtable
	{
		using deleter_fn_t = void(void*);
		using dtor_fn_t = void(void*);
		using copy_ctor_fn_t = void(void*, void*);
		using move_ctor_fn_t = void(void*, void*);

		deleter_fn_t* deleter_fn = nullptr;
		dtor_fn_t* dtor_fn = nullptr;
		copy_ctor_fn_t* copy_ctor_fn = nullptr;
		move_ctor_fn_t* move_ctor_fn = nullptr;
		type_info type_info;

		void release()
		{
			type_info.reset();
			deleter_fn = nullptr;
			dtor_fn = nullptr;
			copy_ctor_fn = nullptr;
			move_ctor_fn = nullptr;
		}

		template<typename T>
		void refresh()
		{
			release();

			type_info.refresh<T>();

			deleter_fn = +[](void* ptr)
				{
					delete reinterpret_cast<T*>(ptr);
				};

			dtor_fn = +[](void* ptr)
				{
					reinterpret_cast<T*>(ptr)->~T();
				};

			copy_ctor_fn = nullptr;

			if constexpr (std::is_copy_constructible_v<T>)
			{
				copy_ctor_fn = +[](void* from, void* to)
					{
						new (to) T(*reinterpret_cast<T*>(from));
					};
			}

			move_ctor_fn = nullptr;

			if constexpr (std::is_move_constructible_v<T>)
			{
				move_ctor_fn = +[](void* from, void* to)
					{
						new (to) T(std::move(*reinterpret_cast<T*>(from)));
					};
			}
		}

		void deleter(void* ptr)
		{
			kw_assert(type_info && ptr);
			deleter_fn(ptr);
		}

		void dtor(void* ptr)
		{
			kw_assert(type_info && ptr);
			dtor_fn(ptr);
		}

		void dtor_offset(void* ptr, usize offset)
		{
			dtor((u8*)(ptr)+offset * type_info.size);
		}


		void copy_ctor(void* from, void* to)
		{
			kw_assert(type_info && from && to);
			kw_assert_msg(copy_ctor_fn, "trying to copy uncopyable type");
			copy_ctor_fn(from, to);
		}

		void copy_ctor_offset(void* from, usize from_offset, void* to, usize to_offset)
		{
			copy_ctor((u8*)(from)+from_offset * type_info.size, (u8*)(to)+to_offset * type_info.size);
		}

		void move_ctor(void* from, void* to)
		{
			kw_assert(type_info && from && to);
			kw_assert_msg(move_ctor_fn, "trying to move unmovalbe type");
			move_ctor_fn(from, to);
		}

		void move_ctor_offset(void* from, usize from_offset, void* to, usize to_offset)
		{
			move_ctor((u8*)(from)+from_offset * type_info.size, (u8*)(to)+to_offset * type_info.size);
		}
	};

	template<typename T>
	struct any_construct_tag {};

	template<usize storage_size>
	class sized_any
	{
	public:
		template<typename value_t, typename...args_t>
		static sized_any create(args_t&&...args) noexcept
		{
			return sized_any(any_construct_tag< value_t>{}, kw_fwd(args)...);
		}

		template<typename value_t>
		static sized_any make(const value_t& arg) noexcept
		{
			return sized_any(any_construct_tag<value_t>{}, arg);
		}

		template<typename value_t>
		static sized_any make(value_t&& arg) noexcept
		{
			return sized_any(any_construct_tag<value_t>{}, kw_fwd(arg));
		}
		
		sized_any() = default;

		template<typename T, typename...Args>
			requires (sizeof(T) <= storage_size)
		sized_any(any_construct_tag<T>, Args&&...args) noexcept
		{
			refresh<T>(std::forward<Args>(args)...);
		}

		sized_any(sized_any&& other) noexcept
		{
			*this = std::move(other);
		}

		sized_any& operator=(sized_any&& other) noexcept
		{
			if (this != &other)
			{
				release();
				_vtable = other._vtable;
				if (_vtable.type_info)
				{
					_vtable.move_ctor((void*)other._storage, (void*)_storage);
				}
				other._vtable.release();
				other.release();
			}

			return *this;
		}


		sized_any(const sized_any& other) noexcept
		{
			*this = other;
		}

		sized_any& operator=(const sized_any& other) noexcept
		{
			if (this != &other)
			{
				release();
				_vtable = other._vtable;
				if (_vtable.type_info)
				{
					_vtable.copy_ctor((void*)other._storage, (void*)_storage);
				}
			}

			return *this;
		}

		~sized_any() noexcept
		{
			release();
		}

		void release() noexcept
		{
			if (_vtable.type_info)
			{
				_vtable.dtor(_storage);
				_vtable.release();
			}
		}

		template<typename...Fn>
		bool try_match(Fn&&...funcs) noexcept
		{
			bool matched = false;
			(([&]()
				{
					if (matched) return;

					if constexpr (std::tuple_size_v<typename function_traits<Fn>::args_tuple>)
					{
						using expected_t = std::remove_cvref_t<typename function_traits<Fn>::template arg_at<0>>;

						if (auto v = try_unwrap<expected_t>())
						{
							funcs(*v);
							matched = true;
						}
					}
					else
					{
						funcs();
						matched = true;
					}
				}()), ...);

			return matched;
		}

		template<typename...Fn>
		bool try_match(Fn&&...funcs) const noexcept
		{
			bool matched = false;
			(([&]()
				{
					if (matched) return;

					if constexpr (std::tuple_size_v<typename function_traits<Fn>::args_tuple>)
					{
						using raw_args_t = typename function_traits<Fn>::template arg_at<0>;
						using u_args_t = std::remove_reference_t<raw_args_t>;
						static_assert(std::is_const_v<u_args_t>);

						using expected_t = std::remove_cvref_t<typename function_traits<Fn>::template arg_at<0>>;

						if (auto v = try_unwrap<expected_t>())
						{
							funcs(*v);
							matched = true;
						}
					}
					else
					{
						funcs();
						matched = true;
					}
				}()), ...);

			return matched;
		}

		template<typename...Fn>
		void ensure_match(Fn&&...funcs) noexcept
		{
			bool matched = false;
			(([&]()
				{
					if (matched) return;


					if constexpr (std::tuple_size_v<typename function_traits<Fn>::args_tuple>)
					{
						using raw_args_t = typename function_traits<Fn>::template arg_at<0>;
						using u_args_t = std::remove_reference_t<raw_args_t>;
						static_assert(std::is_const_v<u_args_t>);

						using expected_t = std::remove_cvref_t<typename function_traits<Fn>::template arg_at<0>>;

						if (auto v = try_unwrap<expected_t>())
						{
							funcs(*v);
							matched = true;
						}
					}
					else
					{
						funcs();
						matched = true;
					}

				}()), ...);

			kw_assert_msg(matched, "wasnt able to match");
		}


		template<typename...Fn>
		void ensure_match(Fn&&...funcs) const noexcept
		{
			bool matched = false;
			(([&]()
				{
					if (matched) return;


					if constexpr (std::tuple_size_v<typename function_traits<Fn>::args_tuple>)
					{
						using expected_t = std::remove_cvref_t<typename function_traits<Fn>::template arg_at<0>>;

						if (auto v = try_unwrap<expected_t>())
						{
							funcs(*v);
							matched = true;
						}
					}
					else
					{
						funcs();
						matched = true;
					}

				}()), ...);

			kw_assert_msg(matched, "wasnt able to match");
		}

		template<typename...Fn>
		bool try_poly_match(Fn&&...funcs) noexcept
		{
			bool matched = false;

			(([&]()
				{
					if constexpr (std::tuple_size_v<typename function_traits<Fn>::args_tuple>)
					{
						using expected_t = std::remove_cvref_t<typename function_traits<Fn>::template arg_at<0>>;

						if (auto v = try_unwrap<expected_t>())
						{
							funcs(*v);
							matched = true;
						}
					}
					else
					{
						funcs();
						matched = true;
					}

				}()), ...);

			return matched;
		}

		template<typename...Fn>
		bool try_poly_match(Fn&&...funcs) const noexcept
		{
			bool matched = false;

			(([&]()
				{
					if constexpr (std::tuple_size_v<typename function_traits<Fn>::args_tuple>)
					{

						using raw_args_t = typename function_traits<Fn>::template arg_at<0>;
						using u_args_t = std::remove_reference_t<raw_args_t>;
						static_assert(std::is_const_v<u_args_t>);

						using expected_t = std::remove_cvref_t<typename function_traits<Fn>::template arg_at<0>>;

						if (auto v = try_unwrap<expected_t>())
						{
							funcs(*v);
							matched = true;
						}
					}
					else
					{
						funcs();
						matched = true;
					}

				}()), ...);

			return matched;
		}


		template<typename T, typename...Args>
			requires (sizeof(T) <= storage_size)
		T& refresh(Args&&...args) noexcept
		{
			release();

			auto v = new(_storage) T(std::forward<Args>(args)...);
			_vtable.refresh<T>();

			return *v;
		}

		template<typename T>
			requires (sizeof(T) <= storage_size)
		T& unwrap()	noexcept
		{
			kw_assert(_vtable.type_info.is<T>());
			return *reinterpret_cast<T*>(_storage);
		}

		template<typename T>
			requires (sizeof(T) <= storage_size)
		const T& unwrap() const noexcept
		{
			kw_assert(_vtable.type_info.is<T>());
			return *reinterpret_cast<const T*>(_storage);
		}

		template<typename T>
			requires (sizeof(T) <= storage_size)
		T* try_unwrap()	noexcept
		{
			if (_vtable.type_info.is<T>())
			{
				return reinterpret_cast<T*>(_storage);
			}
			else
			{
				return nullptr;
			}
		}

		template<typename T>
			requires (sizeof(T) <= storage_size)
		const T* try_unwrap() const noexcept
		{
			if (_vtable.type_info.is<T>())
			{
				return reinterpret_cast<const T*>(_storage);
			}
			else
			{
				return nullptr;
			}
		}

		template<typename T>
			requires (sizeof(T) <= storage_size)
		bool is() noexcept { return _vtable.type_info.is<T>(); }

		const lifetime_vtable& vtable() const { return _vtable; }

	private:
		alignas(std::max_align_t) u8 _storage[storage_size];
		lifetime_vtable _vtable{};
	};

	template<typename...variants_t>
	class scoped_any
	{
	public:
		template<typename value_t, typename...args_t>
			requires (any_of<value_t, variants_t...>)
		static scoped_any create(args_t&&...args) noexcept
		{
			return sized_any(any_construct_tag< value_t>{}, kw_fwd(args)...);
		}

		template<typename value_t>
			requires (any_of<value_t, variants_t...>)
		static scoped_any make(const value_t& arg) noexcept
		{
			return sized_any(any_construct_tag<value_t>{}, arg);
		}

		template<typename value_t>
			requires (any_of<value_t, variants_t...>)
		static scoped_any make(value_t&& arg) noexcept
		{
			return sized_any(any_construct_tag<value_t>{}, kw_fwd(arg));
		}

		scoped_any() = default;

		template<typename T, typename...Args>
			requires (any_of<T, variants_t...>)
		scoped_any(any_construct_tag<T>, Args&&...args) noexcept
		{
			refresh<T>(std::forward<Args>(args)...);
		}

		scoped_any(scoped_any&& other) noexcept
		{
			*this = std::move(other);
		}

		scoped_any& operator=(scoped_any&& other) noexcept
		{
			if (this != &other)
			{
				release();
				_vtable = other._vtable;
				if (_vtable.type_info)
				{
					_vtable.move_ctor((void*)other._storage, (void*)_storage);
				}
				other._vtable.release();
				other.release();
			}

			return *this;
		}


		scoped_any(const scoped_any& other) noexcept
		{
			*this = other;
		}

		scoped_any& operator=(const scoped_any& other) noexcept
		{
			if (this != &other)
			{
				release();
				_vtable = other._vtable;
				if (_vtable.type_info)
				{
					_vtable.copy_ctor((void*)other._storage, (void*)_storage);
				}
			}

			return *this;
		}

		~scoped_any() noexcept
		{
			release();
		}

		void release() noexcept
		{
			if (_vtable.type_info)
			{
				_vtable.dtor(_storage);
				_vtable.release();
			}
		}

		template<typename...Fn>
		bool try_match(Fn&&...funcs) noexcept
		{
			bool matched = false;
			(([&]()
				{
					if (matched) return;

					if constexpr (std::tuple_size_v<typename function_traits<Fn>::args_tuple>)
					{
						using expected_t = std::remove_cvref_t<typename function_traits<Fn>::template arg_at<0>>;

						if (auto v = try_unwrap<expected_t>())
						{
							funcs(*v);
							matched = true;
						}
					}
					else
					{
						funcs();
						matched = true;
					}
				}()), ...);

			return matched;
		}

		template<typename...Fn>
		bool try_match(Fn&&...funcs) const noexcept
		{
			bool matched = false;
			(([&]()
				{
					if (matched) return;

					if constexpr (std::tuple_size_v<typename function_traits<Fn>::args_tuple>)
					{
						using raw_args_t = typename function_traits<Fn>::template arg_at<0>;
						using u_args_t = std::remove_reference_t<raw_args_t>;
						static_assert(std::is_const_v<u_args_t>);

						using expected_t = std::remove_cvref_t<typename function_traits<Fn>::template arg_at<0>>;

						if (auto v = try_unwrap<expected_t>())
						{
							funcs(*v);
							matched = true;
						}
					}
					else
					{
						funcs();
						matched = true;
					}
				}()), ...);

			return matched;
		}

		template<typename...Fn>
		void ensure_match(Fn&&...funcs) noexcept
		{
			bool matched = false;
			(([&]()
				{
					if (matched) return;


					if constexpr (std::tuple_size_v<typename function_traits<Fn>::args_tuple>)
					{
						using raw_args_t = typename function_traits<Fn>::template arg_at<0>;
						using u_args_t = std::remove_reference_t<raw_args_t>;
						static_assert(std::is_const_v<u_args_t>);

						using expected_t = std::remove_cvref_t<typename function_traits<Fn>::template arg_at<0>>;

						if (auto v = try_unwrap<expected_t>())
						{
							funcs(*v);
							matched = true;
						}
					}
					else
					{
						funcs();
						matched = true;
					}

				}()), ...);

			kw_assert_msg(matched, "wasnt able to match");
		}


		template<typename...Fn>
		void ensure_match(Fn&&...funcs) const noexcept
		{
			bool matched = false;
			(([&]()
				{
					if (matched) return;


					if constexpr (std::tuple_size_v<typename function_traits<Fn>::args_tuple>)
					{
						using expected_t = std::remove_cvref_t<typename function_traits<Fn>::template arg_at<0>>;

						if (auto v = try_unwrap<expected_t>())
						{
							funcs(*v);
							matched = true;
						}
					}
					else
					{
						funcs();
						matched = true;
					}

				}()), ...);

			kw_assert_msg(matched, "wasnt able to match");
		}

		template<typename...Fn>
		bool try_poly_match(Fn&&...funcs) noexcept
		{
			bool matched = false;

			(([&]()
				{
					if constexpr (std::tuple_size_v<typename function_traits<Fn>::args_tuple>)
					{
						using expected_t = std::remove_cvref_t<typename function_traits<Fn>::template arg_at<0>>;

						if (auto v = try_unwrap<expected_t>())
						{
							funcs(*v);
							matched = true;
						}
					}
					else
					{
						funcs();
						matched = true;
					}

				}()), ...);

			return matched;
		}

		template<typename...Fn>
		bool try_poly_match(Fn&&...funcs) const noexcept
		{
			bool matched = false;

			(([&]()
				{
					if constexpr (std::tuple_size_v<typename function_traits<Fn>::args_tuple>)
					{

						using raw_args_t = typename function_traits<Fn>::template arg_at<0>;
						using u_args_t = std::remove_reference_t<raw_args_t>;
						static_assert(std::is_const_v<u_args_t>);

						using expected_t = std::remove_cvref_t<typename function_traits<Fn>::template arg_at<0>>;

						if (auto v = try_unwrap<expected_t>())
						{
							funcs(*v);
							matched = true;
						}
					}
					else
					{
						funcs();
						matched = true;
					}

				}()), ...);

			return matched;
		}


		template<typename T, typename...Args>
			requires (any_of<T, variants_t...>)
		T& refresh(Args&&...args) noexcept
		{
			release();

			auto v = new(_storage) T(std::forward<Args>(args)...);
			_vtable.refresh<T>();

			return *v;
		}

		template<typename T>
			requires (any_of<T, variants_t...>)
		T& unwrap()	noexcept
		{
			kw_assert(_vtable.type_info.is<T>());
			return *reinterpret_cast<T*>(_storage);
		}

		template<typename T>
			requires (any_of<T, variants_t...>)
		const T& unwrap() const noexcept
		{
			kw_assert(_vtable.type_info.is<T>());
			return *reinterpret_cast<const T*>(_storage);
		}

		template<typename T>
			requires (any_of<T, variants_t...>)
		T* try_unwrap()	noexcept
		{
			if (_vtable.type_info.is<T>())
			{
				return reinterpret_cast<T*>(_storage);
			}
			else
			{
				return nullptr;
			}
		}

		template<typename T>
			requires (any_of<T, variants_t...>)
		const T* try_unwrap() const noexcept
		{
			if (_vtable.type_info.is<T>())
			{
				return reinterpret_cast<const T*>(_storage);
			}
			else
			{
				return nullptr;
			}
		}

		template<typename T>
			requires (any_of<T, variants_t...>)
		bool is() noexcept { return _vtable.type_info.is<T>(); }

		const lifetime_vtable& vtable() const { return _vtable; }

	private:
		alignas(std::max_align_t) u8 _storage[union_size<variants_t...>];
		lifetime_vtable _vtable{};

	};

	class unsized_any
	{
	public:

		unsized_any() noexcept = default;

		template<typename T, typename...Args>
		unsized_any(any_construct_tag<T>, Args&&...args) noexcept
		{
			refresh<T>(std::forward<Args>(args)...);
		}

		unsized_any& operator=(unsized_any&& other) noexcept
		{
			if (this != &other)
			{
				_vtable.type_info = other._vtable.type_info;
				if (_vtable.type_info)
				{
					_vtable.move_ctor(other._storage, _storage);
				}
				other._vtable.release();
				other.release();
			}

			return *this;
		}

		unsized_any(unsized_any&& other) noexcept
		{
			*this = std::move(other);
		}

		unsized_any& operator=(const unsized_any& other) noexcept
		{
			if (this != &other)
			{
				_vtable = other._vtable;
				if (_vtable.type_info)
				{
					_vtable.copy_ctor(other._storage, _storage);
				}
			}

			return *this;
		}

		unsized_any(const unsized_any& other) noexcept
		{
			*this = other;
		}

		~unsized_any() noexcept
		{
			release();
		}

		template<typename value_t, typename...args_t>
		static unsized_any create(args_t&&...args) noexcept
		{
			return unsized_any(any_construct_tag< value_t>{}, kw_fwd(args)...);
		}

		template<typename value_t>
		static unsized_any make(const value_t& arg) noexcept
		{
			return unsized_any(any_construct_tag<value_t>{}, arg);
		}

		template<typename value_t>
		static unsized_any make(value_t&& arg) noexcept
		{
			return unsized_any(any_construct_tag<value_t>{}, kw_fwd(arg));
		}


		void release() noexcept
		{
			if (_storage)
			{
				_vtable.deleter(_storage);
				_storage = nullptr;
			}
		}

			template<typename...Fn>
		bool try_match(Fn&&...funcs) noexcept
		{
			bool matched = false;
			(([&]()
				{
					if (matched) return;

					if constexpr (std::tuple_size_v<typename function_traits<Fn>::args_tuple>)
					{
						using expected_t = std::remove_cvref_t<typename function_traits<Fn>::template arg_at<0>>;

						if (auto v = try_unwrap<expected_t>())
						{
							funcs(*v);
							matched = true;
						}
					}
					else
					{
						funcs();
						matched = true;
					}
				}()), ...);

			return matched;
		}

		template<typename...Fn>
		bool try_match(Fn&&...funcs) const noexcept
		{
			bool matched = false;
			(([&]()
				{
					if (matched) return;

					if constexpr (std::tuple_size_v<typename function_traits<Fn>::args_tuple>)
					{
						using raw_args_t = typename function_traits<Fn>::template arg_at<0>;
						using u_args_t = std::remove_reference_t<raw_args_t>;
						static_assert(std::is_const_v<u_args_t>);

						using expected_t = std::remove_cvref_t<typename function_traits<Fn>::template arg_at<0>>;

						if (auto v = try_unwrap<expected_t>())
						{
							funcs(*v);
							matched = true;
						}
					}
					else
					{
						funcs();
						matched = true;
					}
				}()), ...);

			return matched;
		}

		template<typename...Fn>
		void ensure_match(Fn&&...funcs) noexcept
		{
			bool matched = false;
			(([&]()
				{
					if (matched) return;


					if constexpr (std::tuple_size_v<typename function_traits<Fn>::args_tuple>)
					{
						using expected_t = std::remove_cvref_t<typename function_traits<Fn>::template arg_at<0>>;

						if (auto v = try_unwrap<expected_t>())
						{
							funcs(*v);
							matched = true;
						}
					}
					else
					{
						funcs();
						matched = true;
					}

				}()), ...);

			kw_assert_msg(matched, "wasnt able to match");
		}


		template<typename...Fn>
		void ensure_match(Fn&&...funcs) const noexcept
		{
			bool matched = false;
			(([&]()
				{
					if (matched) return;


					if constexpr (std::tuple_size_v<typename function_traits<Fn>::args_tuple>)
					{

						using raw_args_t = typename function_traits<Fn>::template arg_at<0>;
						using u_args_t = std::remove_reference_t<raw_args_t>;
						static_assert(std::is_const_v<u_args_t>);

						using expected_t = std::remove_cvref_t<typename function_traits<Fn>::template arg_at<0>>;

						if (auto v = try_unwrap<expected_t>())
						{
							funcs(*v);
							matched = true;
						}
					}
					else
					{
						funcs();
						matched = true;
					}

				}()), ...);

			kw_assert_msg(matched, "wasnt able to match");
		}

		template<typename...Fn>
		bool try_poly_match(Fn&&...funcs) noexcept
		{
			bool matched = false;

			(([&]()
				{
					if constexpr (std::tuple_size_v<typename function_traits<Fn>::args_tuple>)
					{
						using expected_t = std::remove_cvref_t<typename function_traits<Fn>::template arg_at<0>>;

						if (auto v = try_unwrap<expected_t>())
						{
							funcs(*v);
							matched = true;
						}
					}
					else
					{
						funcs();
						matched = true;
					}

				}()), ...);

			return matched;
		}

		template<typename...Fn>
		bool try_poly_match(Fn&&...funcs) const noexcept
		{
			bool matched = false;

			(([&]()
				{
					if constexpr (std::tuple_size_v<typename function_traits<Fn>::args_tuple>)
					{

						using raw_args_t = typename function_traits<Fn>::template arg_at<0>;
						using u_args_t = std::remove_reference_t<raw_args_t>;
						static_assert(std::is_const_v<u_args_t>);

						using expected_t = std::remove_cvref_t<typename function_traits<Fn>::template arg_at<0>>;

						if (auto v = try_unwrap<expected_t>())
						{
							funcs(*v);
							matched = true;
						}
					}
					else
					{
						funcs();
						matched = true;
					}

				}()), ...);

			return matched;
		}

		template<typename T, typename...Args>
		T& refresh(Args&&...args) noexcept
		{
			release();

			auto v = new T(std::forward<Args>(args)...);
			_storage = v;
			_vtable.refresh<T>();

			return *v;
		}

		template<typename T>
		T& unwrap()	noexcept
		{
			kw_assert(_storage);
			kw_assert(_vtable.type_info.is<T>());
			return *reinterpret_cast<T*>(_storage);
		}

		template<typename T>
		T* try_unwrap()	noexcept
		{
			kw_assert(_storage);

			if (_vtable.type_info.is<T>())
			{
				return reinterpret_cast<T*>(_storage);
			}
			else
			{
				return nullptr;
			}
		}

		template<typename T>
		bool is() const noexcept { return _vtable.type_info.is<T>(); }

	private:
		void* _storage = nullptr;
		lifetime_vtable _vtable{};
	};

}
#endif // !KAWA_ANY
