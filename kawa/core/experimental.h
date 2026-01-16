#ifndef KAWA_EXPERIMENTAL
#define KAWA_EXPERIMENTAL

#include "utils.h"
#include "event_manager.h"
#include "core_types.h"

#undef kw_info
#undef kw_warn
#undef kw_error

#define kw_info(fmt, ...) ::kawa::logger::print(::kawa::info(std::format(fmt, __VA_ARGS__)))
#define kw_warn(fmt, ...) ::kawa::logger::print(::kawa::warn(std::format(fmt, __VA_ARGS__)))
#define kw_error(fmt, ...) ::kawa::logger::print(::kawa::error(std::format(fmt, __VA_ARGS__)))

#define kw_logger_prelude(log_manager_name, logger_name)\
::kawa::log_event_manager log_manager_name;\
::kawa::logger logger_name(log_manager_name)\

namespace kawa 
{
	using log_event_manager = basic_event_manager<64>;
	using log_event_t = log_event_manager::event_t;

	template<
		static_string label_,
		ansi_codes::text_color text_color = ansi_codes::text_color::default_,
		ansi_codes::background_color bg_color = ansi_codes::background_color::default_
	>
	struct logger_entry
	{
		constexpr static auto label = label_;
		constexpr static auto text_col = text_color;
		constexpr static auto bg_col = bg_color;

		logger_entry(string_view m)
			: msg(m)
		{
		}

		string msg;
	};

	using info = logger_entry<"info", ansi_codes::text_color::green>;
	using warn = logger_entry<"warn", ansi_codes::text_color::yellow>;
	using error = logger_entry<"error", ansi_codes::text_color::red>;

	using verbose = logger_entry<"verbose", ansi_codes::text_color::red>;

	template<typename type>
	concept is_logger_entry = requires()
	{
		type::label;
		type::text_col;
		type::bg_col;
	};

	struct logger
	{
		logger(log_event_manager& mgr)
			: sub_guard(*this, mgr)
		{
		}

		template<is_logger_entry log_entry_t>
		static void print(const log_entry_t& e)
		{
			std::fputs(
				std::format(
					kw_ansi_fmt_block "{}" kw_ansi_reset_fmt_block ": {}\n",
					(u32)log_entry_t::text_col,
					(u32)log_entry_t::bg_col,
					log_entry_t::label.value,
					e.msg
				).c_str(),
				stdout
			);
		}

		void dispatch(const log_event_t& e)
		{
			e.try_match(
				[&](const info& i) { print(i); },
				[&](const warn& i) { print(i); },
				[&](const error& i) { print(i); }				
			);
		}

		log_event_manager::subscribe_guard sub_guard;
	};

	struct file_logger
	{
		file_logger(log_event_manager& mgr)
			: sub_guard(*this, mgr)
		{
			fopen_s(&out, "log.txt", "w");
		}
		template<is_logger_entry log_entry_t>
		void write(const log_entry_t& e)
		{
			std::fputs(
				std::format(
					"{}: {}\n",
					log_entry_t::label.value,
					e.msg
				).c_str(),
				out
			);
		}

		void dispatch(const log_event_t& e)
		{
			e.try_match(
				[&](const info& i) { write(i); },
				[&](const warn& i) { write(i); },
				[&](const error& i) { write(i); }
			);
		}

		FILE* out;
		log_event_manager::subscribe_guard sub_guard;
	};	
}

#endif // !KAWA_EXPERIMENTAL
