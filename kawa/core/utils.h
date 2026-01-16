#ifndef KAWA_UTILS
#define KAWA_UTILS

#include "macros.h"
#include "core_types.h"

#include <fstream>

#define kw_fwd(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)
#define kw_alias(alias, type) using alias = ::kawa::strong_alias<type, #alias>

namespace kawa
{
    struct ansi_codes
    {
        enum class text_color
        {
            default_ = 39,
            black = 30,
            red = 31,
            green = 32,
            yellow = 33,
            blue = 34,
            magenta = 35,
            cyan = 36,
            white = 37,
        };
        enum class background_color
        {
            default_ = 49,
            black = 40,
            red = 41,
            green = 42,
            yellow = 43,
            blue = 44,
            magenta = 45,
            cyan = 46,
            white = 47,
        };
    };


    struct into_any
    {
        template<typename T>
        operator T& () noexcept
        {
            return _::fake_object<T>;
        }

        template<typename T>
        operator T&& () noexcept
        {
            return _::fake_object<T>;
        }
    };

    template <usize N>
    struct static_string
    {
        char value[N];
        usize len = N;

        consteval static_string() = default;

        consteval static_string(const char(&str)[N])
        {
            std::copy_n(str, N, value);
        }

        template<usize i>
        consteval bool operator==(const static_string<i>& other) const
        {
            for (usize i = 0; i < N; ++i)
                if (value[i] != other.value[i]) return false;
            return true;
        }
    };

    template<usize i>
    static_string(const char(&)[i]) -> static_string<i>;

	inline string read_file(const string& path)
	{
		std::ifstream file(path);

		kw_verify_msg(file.is_open(), "failed to open file: {}", path);

		std::ostringstream buffer;
		buffer << file.rdbuf();  

		return buffer.str();
	}

    inline dyn_array<u8> read_file_bytes(const string& path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);

        kw_verify_msg(file.is_open(), "failed to open file: {}", path);

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        dyn_array<u8> buffer(static_cast<size_t>(size));
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
        {
            kw_panic_msg("failed to read file: {}", path);
        }

        return buffer;
    }

    template <typename T, usize N>
    constexpr usize array_size(const T(&)[N]) noexcept { return N; }

    template <typename T>
    usize dyn_array_byte_size(const dyn_array<T>& arr) noexcept { return arr.size() * sizeof(T); }

    template<typename T, static_string alias_tag_>
    struct strong_alias
    {
        T* operator->() noexcept
        {
            return &value;
        }

        const T* operator->() const noexcept
        {
            return &value;
        }

        T& operator*() noexcept
        {
            return value;
        }

        const T& operator*() const noexcept
        {
            return value;
        }

        T value;
    };
}
#endif // !KAWA_UTILS
