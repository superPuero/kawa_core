#ifndef KW_DYN_MODULE
#define KW_DYN_MODULE

#include "core_types.h"

#if defined(kw_platform_windows)
    #include <windows.h>
#elif defined(kw_platform_linux | kw_platform_apple)
    #include <dlfcn.h>
#endif

namespace kawa
{
    struct dyn_module
    {
        void* _handle = nullptr;
        string path;

        dyn_module(const string& p)
        {
            refresh(p);
        }

        dyn_module(const dyn_module&) = delete;
        dyn_module& operator=(const dyn_module&) = delete;

        dyn_module(dyn_module&& other) noexcept 
        {
            *this = std::move(other);
        }

        dyn_module& operator=(dyn_module&& other) noexcept 
        {
            if (this != &other) 
            {
                release();
                _handle = other._handle;
                other._handle = nullptr;
            }
            return *this;
        }

        ~dyn_module() 
        {
            if (_handle) 
            {
                FreeLibrary(static_cast<HMODULE>(_handle));
            }
        }

        void release()
        {
            if (_handle)
            {
                FreeLibrary(static_cast<HMODULE>(_handle));
                _handle = nullptr;
            }
        }

        void refresh(const string& p)
        {
            release();
            path = p;
            _handle = LoadLibraryA(path.c_str());
        }

        void refresh()
        {
            release();
            _handle = LoadLibraryA(path.c_str());
        }

        template<typename Fn>
        Fn* get(const string& symbol) const
        {
            void* fn = nullptr;
            fn = reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(_handle), symbol.c_str()));

            return reinterpret_cast<Fn*>(fn);
        }
    };

}

#endif