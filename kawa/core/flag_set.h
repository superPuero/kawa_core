#ifndef KAWA_FLAG_SET
#define KAWA_FLAG_SET

template<typename enum_t>
	requires std::is_enum_v<enum_t>
struct flag_set
{
	using underlying_t = std::underlying_type_t<enum_t>;

	flag_set() = default;

	flag_set(std::initializer_list<enum_t> list)
	{
		for (auto v : list)
		{
			set(v);
		}
	}

	flag_set(std::initializer_list<underlying_t> list)
	{
		for (auto v : list)
		{
			set(v);
		}
	}

	void set(enum_t v)
	{
		value |= static_cast<underlying_t>(v);
	}

	void set(underlying_t v)
	{
		value |= v;
	}

	enum_t as_enum() noexcept
	{
		return static_cast<enum_t>(value);
	}

	underlying_t as_underlying() noexcept
	{
		return value;
	}

	underlying_t value;
};

#endif