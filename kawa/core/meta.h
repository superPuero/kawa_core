#ifndef KAWA_META
#define KAWA_META

#include <string>
#include <string_view>
#include <cstdint>
#include <utility>
#include <tuple>
#include <type_traits>

#include "core_types.h"

#if defined(kw_compiler_clang)
#	define kw_typename_prefix "kawa::_type_name(void) [T = "
#	define kw_typename_suffix "]"
#elif defined(kw_compiler_msvc)
#	define kw_typename_prefix "kawa::_type_name<"
#	define kw_typename_suffix ">(void) noexcept"
#elif defined(kw_compiler_gnu) 
#	define kw_typename_prefix "kawa::_type_name() [with T = "
#	define kw_typename_suffix "; std::string_view = std::basic_string_view<char>]"
#else
#	define kw_typename_prefix ""
#	define kw_typename_suffix ""
#endif

namespace kawa
{
	namespace _
	{
		template<typename T>
		extern const T fake_object;

		inline constexpr kawa::string_view type_name_helper(kawa::string_view decorated_name)
		{
			decorated_name.remove_prefix(decorated_name.find(kw_typename_prefix) + sizeof(kw_typename_prefix) - 1);
			decorated_name.remove_suffix(sizeof(kw_typename_suffix) - 1);				

			return decorated_name;
		}

		constexpr u64 fnv1a_hash(kawa::string_view str) noexcept
		{
			constexpr u64 fnv_offset_basis = 14695981039346656037ull;
			constexpr u64 fnv_prime = 1099511628211ull;

			u64 hash = fnv_offset_basis;

			for (char c : str)
			{
				hash ^= static_cast<u64>(c);
				hash *= fnv_prime;
			}

			return hash;
		}
	}

	template<typename T>
	inline consteval kawa::string_view _type_name() noexcept
	{
		return _::type_name_helper(std::source_location::current().function_name());
	}

	constexpr u64 string_hash(string_view str) noexcept					
	{
		return _::fnv1a_hash(str);
	}

	template<typename T>
	inline consteval u64 _type_hash() noexcept
	{
		return string_hash(_type_name<T>());
	}

	template<typename T>
	constexpr u64 type_hash = _type_hash<T>();

	template<typename T>
	constexpr string_view type_name = _type_name<T>();

	template<typename T>
	constexpr auto static_type_name = static_string<type_name<T>.size()>(type_name<T>.data());

	template<typename T>
	struct construct_tag{};
				
	struct type_info
	{			
		inline constexpr type_info() noexcept
			: name("")
			, hash(0)
			, size(0)
			, alignment(0)
		{
		}

		template<typename T>
		inline constexpr type_info(construct_tag<T>)
			: name(type_name<T>)
			, hash(type_hash<T>)
			, size(sizeof(T))
			, alignment(alignof(T))
		{
		}

		template<typename T>
		void refresh() noexcept
		{
			*this = type_info(construct_tag<T>{});
		}

		constexpr operator bool() const noexcept
		{
			return hash;
		}

		constexpr bool operator==(const type_info& other) const noexcept
		{
			return (name == other.name && hash == other.hash);
		}

		template<typename T>
		constexpr inline bool is() const noexcept
		{
			return hash == type_hash<T>;
		}

		void reset()
		{
			*this = type_info();
		}

		kawa::string_view name;
		u64 hash;
		usize size;
		usize alignment;			

	};

	template<typename T>
	constexpr type_info type_info_of = type_info(construct_tag<T>{});

	template <template <typename> class Predicate, typename Tuple>
	struct filter_tuple;

	template <template <typename> class Predicate, typename... Ts>
	struct filter_tuple<Predicate, std::tuple<Ts...>> {
			
		using type = decltype(std::tuple_cat(
			std::declval<
			typename std::conditional<
			Predicate<Ts>::value, 
			std::tuple<Ts>,       
			std::tuple<>          
			>::type
			>()...
		));
	};

	template <template <typename> class Predicate, typename Tuple>
	using filter_tuple_t = typename filter_tuple<Predicate, Tuple>::type;

	template<typename RetTy, typename...ArgTy>
	struct function_traits {};

	template<typename RetTy, typename...ArgTy>
	struct function_traits<RetTy(*)(ArgTy...)>
	{
		using return_type = RetTy;
		using args_tuple = typename std::tuple<ArgTy...>;
		template<usize i>
		using arg_at = typename std::tuple_element_t<i, args_tuple>;
	};

	template<typename RetTy, typename...ArgTy>
	struct function_traits<RetTy(&)(ArgTy...)>
	{
		using return_type = RetTy;
		using args_tuple = typename std::tuple<ArgTy...>;
		template<usize i>
		using arg_at = typename std::tuple_element_t<i, args_tuple>;
	};

	template<typename RetTy, typename...ArgTy>
	struct function_traits<RetTy(ArgTy...)>
	{
		using return_type = RetTy;
		using args_tuple = typename std::tuple<ArgTy...>;
		template<usize i>
		using arg_at = typename std::tuple_element_t<i, args_tuple>;
	};

	template<typename RetTy, typename ObjTy, typename...ArgTy>
	struct function_traits<RetTy(ObjTy::*)(ArgTy...) const>
	{
		using return_type = RetTy;
		using args_tuple = typename std::tuple<ArgTy...>;
		template<usize i>
		using arg_at = typename std::tuple_element_t<i, args_tuple>;
	};

	template<typename T>
	struct function_traits<T> : function_traits<decltype(&T::operator())> {};

	template<typename Tuple, usize start, usize end, typename = void>
	struct sub_tuple
	{
		using tuple = decltype([]<usize... I>(std::index_sequence<I...>) -> std::tuple<std::tuple_element_t<start + I, Tuple>...>
		{
		}(std::make_index_sequence<end - start>{}));
	};

	template<typename Tuple, usize start, usize end>
	struct sub_tuple<Tuple, start, end, std::enable_if_t<((end - start) > std::tuple_size_v<Tuple>)>>
	{
		using tuple = typename std::tuple<>;
	};

	template <template <typename> typename F, typename T>
	struct transform_each;

	template <template <typename> typename F, typename...Types>
	struct transform_each<F, std::tuple<Types...>>
	{
		using type = typename std::tuple<F<Types>...>;
	};

	template<template <typename...> typename F, typename T>
	using transform_each_t = typename transform_each<F, T>::type;

	template <typename...types>
	constexpr usize union_size = std::max({ sizeof(types)... });

	template<typename...args_t>
	constexpr bool any_of = false;

	template<typename type, typename head_t>
	constexpr bool any_of<type, head_t> = std::is_same_v<type, head_t>;

	template<typename type, typename head_t, typename...args_t>
	constexpr bool any_of<type, head_t, args_t...> = std::is_same_v<type, head_t> || any_of<type, args_t...>;
}

#endif