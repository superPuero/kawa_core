#ifndef KAWA_MANAGED
#define KAWA_MANAGED

#include <new>
#include "core_types.h"

namespace kawa
{
	template<typename T>
	struct managed
	{
		managed() noexcept = default;

		template<typename...Args>
		managed(Args&&...args) noexcept
		{
			refresh(kw_fwd(args)...);
		}

		managed(const managed& other) noexcept 
		{
			*this = other;
		};

		managed& operator=(const managed& other) noexcept
		{
			if (this != &other)
			{
				if (!other.empty())
				{
					refresh(other.unwrap());
					other.release();
				}
			}

			return *this;
		};

		managed(managed&& other) noexcept
		{
			*this = std::move(other);
		}

		managed& operator=(managed&& other) noexcept
		{
			if (this != &other)
			{
				value = other.value;
				other.release();
			}
			return *this;

		}

		~managed() noexcept
		{
			release();
		}

		T* get() noexcept
		{
			return value;
		}

		const T* get() const noexcept
		{
			return value;
		}

		T& unwrap() noexcept
		{
			kw_assert(value);
			return *value;
		}

		const T& unwrap() const noexcept
		{
			kw_assert(value);
			return *value;
		}

		T& operator->() noexcept
		{
			kw_assert(value);
			return *value;
		}

		const T& operator->() const noexcept
		{
			kw_assert(value);
			return *value;
		}

		template<typename...Args> 
		void refresh(Args&&...args) noexcept
		{
			release();

			value = new T(kw_fwd(args)...);
		}

		void release() noexcept
		{
			if (value)
			{		
				delete value;
				value = nullptr;
			}
		}

		bool empty() const noexcept
		{
			return value;
		}

		T* value = nullptr;
	};

	template<typename T>
	struct managed_array
	{
		managed_array() noexcept = default;

		template<typename...Args>
		managed_array(usize sz) noexcept
		{
			refresh(sz);
		}

		template<typename...Args>
		managed_array(usize sz, Args&...args) noexcept
		{
			refresh(sz, args...);
		}

		managed_array(const managed_array& other) noexcept
		{
			*this = other;
		}

		managed_array& operator=(const managed_array& other) noexcept
		{
			if (this != &other)
			{
				_infer_copy_assign(other);
			}

			return *this;
		};

		void _infer_copy_assign(const managed_array& other) noexcept
		{
			if constexpr (std::is_copy_constructible_v<T>)
			{
				release();

				_size = other._size;
				_capacity = other._capacity;
				_storage = static_cast<byte*>(::operator new(sizeof(T) * _size, std::align_val_t{ alignof(T) }, std::nothrow_t{}));

				T* ptr = data();

				for (usize i = 0; i < _size; i++)
				{
					new (ptr + i) T(other[i]);
				}
			}
			else
			{
				static_assert(false, "trying to copy uncopyable type");
			}
		}

		managed_array(managed_array&& other) noexcept
		{
			*this = std::move(other);
		}

		managed_array& operator=(managed_array&& other) noexcept
		{
			if (this != &other)
			{
				_storage = other._storage;
				_storage = other._size;
				_storage = other._capacity;

				other._storage = nullptr;
				other._size = 0;
				other._capacity = 0;
			}
			return *this;

		}

		~managed_array() noexcept
		{
			release();
		}

		T& at(usize i) noexcept
		{
			kw_assert(_storage);
			return *(data() + i);
		}

		const T& at(usize i) const noexcept
		{
			kw_assert(_storage);
			return *(data() + i);
		}

		T& operator[](usize i) noexcept
		{
			return at(i);
		}

		const T& operator[](usize i) const noexcept
		{
			return at(i);
		}


		template<typename...Args>
		void refresh(usize sz, Args&...args) noexcept
		{
			release();
			//maybe return bool, and rename to try_refresh()?
			_size = sz;
			_capacity = sz;
			_storage = static_cast<byte*>(::operator new(sizeof(T) * _capacity, std::align_val_t{alignof(T)}, std::nothrow_t{}));
			for_each_raw([&](T* ptr) { new(ptr) T(args...); });
		}

		template<typename...Args>
		void refresh_and_keep(usize sz, Args&...args) noexcept
		{
			if (sz == _size) return;

			if (sz > _capacity)
			{
				byte* new_storage = static_cast<byte*>(::operator new(sizeof(T) * sz, std::align_val_t{ alignof(T) }, std::nothrow_t{}));
				T* ptr = reinterpret_cast<T*>(new_storage);

				for (usize i = 0; i < _size; i++)
				{
					if constexpr (std::is_move_constructible_v<T>)
					{
						new(ptr + i) T(std::move(at(i)));
					}
					else if (std::is_copy_constructible_v<T>)
					{
						new(ptr + i) T(at(i));
					}
					else
					{
						new(ptr + i) T();
					}
				}

				for (usize i = _size; i < sz; i++)
				{
					new(ptr + i) T(args...);
				}

				release();
				_size = sz;
				_capacity = sz;
				_storage = new_storage;
			}
			else
			{
				_size = sz;
					
				T* ptr = data();

				for (usize i = _size; i < _capacity; i++)
				{
					(ptr + i)->~T();
				}
			}


		}

		template<std::invocable<T*> Fn>
		void for_each_raw(Fn&& func) noexcept
		{
			T* ptr = data();
			for (usize i = 0; i < _size; i++)
			{
				func(ptr + i);
			}
		}

		template<std::invocable<const T*> Fn>
		void for_each_raw(Fn&& func) const noexcept
		{
			const T* ptr = data();
			for (usize i = 0; i < _size; i++)
			{
				func(ptr + i);
			}
		}

		template<std::invocable<T&> Fn>
		void for_each(Fn&& func) noexcept
		{
			T* ptr = data();
			for (usize i = 0; i < _size; i++)
			{
				func(ptr[i]);
			}
		}

		template<std::invocable<const T&> Fn>
		void for_each(Fn&& func) const noexcept
		{
			const T* ptr = data();
			for (usize i = 0; i < _size; i++)
			{
				func(ptr[i]);
			}
		}


		void release() noexcept
		{
			if (_storage)
			{
				for_each_raw([](T* ptr) { ptr->~T(); });

				::operator delete(_storage, std::align_val_t{ alignof(T) });;
				_storage = nullptr;
				_size = 0;
				_capacity = 0;
			}
		}

		usize size() const noexcept
		{
			return _size;
		}

		usize capacity() const noexcept
		{
			return _capacity;
		}

		bool empty() const noexcept
		{
			return _size;
		}
	
		T* data() noexcept
		{
			return reinterpret_cast<T*>(_storage);
		}

		const T* data() const noexcept
		{
			return reinterpret_cast<T*>(_storage);
		}



		byte* _storage = nullptr;
		usize _size = 0;
		usize _capacity = 0;
	};

}

#endif