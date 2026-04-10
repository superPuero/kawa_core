#ifndef KAWA_ENV
#define KAWA_ENV

#include "ecs.h"

namespace kawa
{
    struct env_t
    {
        env_t(const registry::config& cfg)
            : _native({
                    .name = cfg.name,
                    .max_component_count = cfg.max_component_count
                })
            , _filo_ensure({ .registry = _native, .fifo = false })
        {
            _entity = _native.entity();
        }

        template<typename...Fn>
        void run(Fn&&...fn) noexcept
        {
            ((_native.query_with(_entity, kw_fwd(fn))), ...);
        }

        template<typename T>
        T& get() noexcept
        {
            return _native.get<T>(_entity);
        }

        template<typename T>
        T* try_get() noexcept
        {
            return _native.try_get<T>(_entity);
        }

        template<typename T>
        bool has() noexcept
        {
            return _native.has<T>(_entity);
        }

        template<typename T, typename...Args>
        T& emplace(Args&&...args) noexcept
        {
            _filo_ensure.erase<T>(_entity);

            return _native.emplace<T>(_entity, kw_fwd(args)...);
        }

        template<typename T>
        T& add(T&& value) noexcept
        {
            _filo_ensure.erase<T>(_entity);

            return _native.add<T>(_entity, std::move(value));
        }

        template<typename T, typename...Args>
        T& try_emplace(Args&&...args) noexcept
        {
            if (!has<T>(_entity))
            {
                _filo_ensure.erase<T>(_entity);
                return get<T>(_entity);
            }

            return emplace<T>(_entity, kw_fwd(args)...);
        }

        template<typename T>
        void erase() noexcept
        {
            _native.erase<T>(_entity);
        }

        template<typename T>
        T& require() noexcept
        {
            kw_assert_msg(has<T>(), "resource request not satisfied");
            return get<T>();
        }

        registry& native() noexcept
        {
            return _native;
        }

        entity_id _entity;
        registry _native;
        registry::defer_buffer _filo_ensure;
    };
}

#endif // !KAWA_ENV
