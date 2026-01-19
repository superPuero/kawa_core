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

#define kw_info(fmt, ...) kw_global_logging_broadcaster().emit({.qual = log::qualifier::info, .msg = std::format(fmt, __VA_ARGS__)})
#define kw_warn(fmt, ...) kw_global_logging_broadcaster().emit({.qual = log::qualifier::warn, .msg = std::format(fmt, __VA_ARGS__)})
#define kw_error(fmt, ...) kw_global_logging_broadcaster().emit({.qual = log::qualifier::error, .msg = std::format(fmt, __VA_ARGS__)})
#define kw_verbose(fmt, ...) kw_global_logging_broadcaster().emit({.qual = log::qualifier::verbose, .msg = std::format(fmt, __VA_ARGS__)})

namespace kawa 
{
	struct log
	{
		enum class qualifier
		{
			info = ansi_codes::text_color::green,
			warn = ansi_codes::text_color::yellow,
			error = ansi_codes::text_color::red,
			verbose = ansi_codes::text_color::cyan
		} qual;

		static string_view to_str(log::qualifier q) noexcept
		{
			switch (q)
			{
			case kawa::log::qualifier::info:
				return "info";
				break;
			case kawa::log::qualifier::warn:
				return "warn";
				break;
			case kawa::log::qualifier::error:
				return "error";
				break;
			case kawa::log::qualifier::verbose:
				return "verbose";
				break;
			default:
				return "unspecified";
				break;
			}
		}

		string msg;
	};

	using log_broadcaster = broadcaster<log>;

	struct basic_stdout_logger
	{
		basic_stdout_logger(log_broadcaster& mgr)
			: listner(this, mgr)
		{
		}

		void recieve(const log& l)
		{
			std::fputs(
				std::format(
					kw_ansi_fmt_block "{}" kw_ansi_reset_fmt_block ": {}\n",
					(u32)l.qual,
					(u32)ansi_codes::background_color::default_,
					log::to_str(l.qual),
					l.msg
				).c_str(),
				stdout
			);
		}

		log_broadcaster::listner listner;
	};

	struct _g_logger
	{
		static basic_stdout_logger& lazy_get_logger() noexcept
		{
			if (!logger)
			{
				mgr = new log_broadcaster();
				logger = new basic_stdout_logger(*mgr);
			}

			return *logger;
		}

		static log_broadcaster& lazy_get_broadcaster()
		{
			if (!logger)
			{
				mgr = new log_broadcaster();
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

		static inline log_broadcaster* mgr = nullptr;
		static inline basic_stdout_logger* logger = nullptr;
	};
}

#endif // !KAWA_EXPERIMENTAL
