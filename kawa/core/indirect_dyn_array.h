#ifndef KAWA_INDIRECT_DYN_ARRAY
#define KAWA_INDIRECT_DYN_ARRAY

#include "core_types.h"

namespace kawa
{
	struct table : indirect_array_base
	{
		table() noexcept = default;

		template<typename...Args>
		table(usize sz, Args&...args)
		{

		}		
	
		template<typename...Args>
		T& emplace_at(usize i, Args&&...args) noexcept
		{
			ensure_size(i);

			bool& cell = _mask[i];
						
			if (!cell)
			{
				_indirect_map[i] = _dense_indirect_map.size();
				_dense_indirect_map.emplace_back(i);
				auto& out = _data.emplace_back(kw_fwd(args)...);
				cell = true;
			}
			else
			{
				auto& out = _data[i];
				out.~T();
				new (&out) T(kw_fwd(args)...);
			}

			return _data[i];
		}

		void ensure_size(usize i)
		{
			_mask.resize(i, false);
			_indirect_map.resize(i);
		}
		
	};
}

#endif