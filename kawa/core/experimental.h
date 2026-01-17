#ifndef KAWA_EXPERIMENTAL
#define KAWA_EXPERIMENTAL

#include "utils.h"
#include "broadcaster.h"
#include "core_types.h"


#define kw_global_logging_broadcaster() ::kawa::_g_logger::lazy_get_broadcaster()
#define kw_global_stdout_logger() ::kawa::_g_logger::lazy_get_logger()

#undef kw_info
#undef kw_warn
#undef kw_error

#define kw_info(fmt, ...) kw_global_logging_broadcaster().emit(logging_message_t::make(::kawa::info(std::format(fmt, __VA_ARGS__))))
#define kw_warn(fmt, ...) kw_global_logging_broadcaster().emit(logging_message_t::make(::kawa::warn(std::format(fmt, __VA_ARGS__))))
#define kw_error(fmt, ...) kw_global_logging_broadcaster().emit(logging_message_t::make(::kawa::error(std::format(fmt, __VA_ARGS__))))
#define kw_verbose(fmt, ...) kw_global_logging_broadcaster().emit(logging_message_t::make(::kawa::verbose(std::format(fmt, __VA_ARGS__))))

namespace kawa 
{
	using logging_broadcaster = broadcaster<sized_any<64>>;
	using logging_message_t = logging_broadcaster::message_t;

	template<static_string qualifier, ansi_codes::text_color text_color>
	struct qualified_message
	{
		constexpr static auto label = qualifier;
		constexpr static auto colour = text_color;
		qualified_message(string_view m)
			: msg(m) {
		}

		string msg;
	};

	using info = qualified_message<"info", ansi_codes::text_color::green>;
	using warn = qualified_message<"warn", ansi_codes::text_color::yellow>;
	using error = qualified_message<"error", ansi_codes::text_color::red>;

	using verbose = qualified_message<"verbose", ansi_codes::text_color::cyan>;

	template<typename type>
	concept is_qualified_message = requires
	{		
		type::label;
		type::colour;
	};

	struct basic_stdout_logger
	{
		basic_stdout_logger(logging_broadcaster& mgr)
			: sub_guard(*this, mgr)
		{
		}

		template<is_qualified_message message_t>
		void write(const message_t& e)
		{
			std::fputs(
				std::format(
					kw_ansi_fmt_block "{}" kw_ansi_reset_fmt_block ": {}\n",
					(u32)message_t::colour,
					(u32)ansi_codes::background_color::default_,
					message_t::label.value,
					e.msg
				).c_str(),
				stdout
			);
		}

		void dispatch(const logging_message_t& e)
		{
			e.try_match(
				[&](const info& i) { write(i); },
				[&](const warn& i) { write(i); },
				[&](const error& i) { write(i); },
				[&](const verbose& i) { write(i); },
				[&]() { write(warn{std::format("unrecognized logging event type {}", e.vtable().type_info.name)}); }
			);
		}

		logging_broadcaster::subscribe_guard sub_guard;
	};

	struct file_logger
	{
		file_logger(logging_broadcaster& mgr, string_view path)
			: sub_guard(*this, mgr)
		{
			fopen_s(&out, path.data(), "w");
		}

		~file_logger()
		{
			fclose(out);
		}

		template<is_qualified_message message_t>
		void write(const message_t& e)
		{
			std::fputs(
				std::format(
					"{}: {}\n",
					message_t::label.value,
					e.msg
				).c_str(),
				out
			);
		}

		void dispatch(const logging_message_t& e)
		{
			e.try_match(
				[&](const info& i) { write(i); },
				[&](const warn& i) { write(i); },
				[&](const error& i) { write(i); },
				[&](const verbose& i) { write(i); },
				[&](){write(warn{ std::format("unrecognized logging event type {}", e.vtable().type_info.name) }); }
			);
		}

		FILE* out;
		logging_broadcaster::subscribe_guard sub_guard;
	};	


	struct _g_logger
	{
		static basic_stdout_logger& lazy_get_logger() noexcept
		{
			if (!logger)
			{
				mgr = new logging_broadcaster();
				logger = new basic_stdout_logger(*mgr);
			}

			return *logger;
		}

		static logging_broadcaster& lazy_get_broadcaster()
		{
			if (!logger)
			{
				mgr = new logging_broadcaster();
				logger = new basic_stdout_logger(*mgr);
			}

			return *mgr;
		}

		~_g_logger()
		{
			if (logger)
			{
				delete mgr;
				delete logger;
			}
		}

		static inline logging_broadcaster* mgr = nullptr;
		static inline basic_stdout_logger* logger = nullptr;
	};
}

#endif // !KAWA_EXPERIMENTAL
