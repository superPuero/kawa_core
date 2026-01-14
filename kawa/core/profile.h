#ifndef KAWA_PROFILING
#define KAWA_PROFILING

#include "core_types.h"

#define kw_profile_scope(name)\
auto _kw_scoped_profiler = kawa::profiler_map::instance().thread_entry().create_scoped(name);\

#define kw_profile_scope_auto() kw_profile_scope(string(std::source_location::current().function_name()))

#define kw_profiler() kawa::profiler_map::instance()

namespace kawa
{
    struct profiler_map
    {       
        struct per_thread
        {
            umap<string, pair<f32, u32>> entry_map;

            struct scoped
            {
                scoped(per_thread& entry, const string& name)
                    : _entry(entry.entry_map[name])
                    , _start(std::chrono::high_resolution_clock::now())
                {
                }

                ~scoped()
                {
                    _entry.first = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - _start).count() / 1000.f;
                    _entry.second++;
                }

                pair<f32, u32>& _entry;
                std::chrono::time_point<std::chrono::high_resolution_clock> _start;
            };

            scoped create_scoped(const string& name)
            {
                return scoped(*this, name);
            }
        };


        static inline profiler_map& instance() noexcept
        {
            static profiler_map pm;
            return pm;
        }

        inline per_thread& thread_entry() noexcept
        {            
            static thread_local per_thread& entry = [this]() -> per_thread&
                {
                    std::lock_guard lock(mutex);
                    return thread_map[std::this_thread::get_id()];
                }();

            return entry;
        }

        mutex mutex;
        umap<thread::id, per_thread> thread_map;
    };
   
}



#endif // !KAWA_PROFILING
