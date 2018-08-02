#ifndef _BIN_H
#define _BIN_H

#include "cell.h"

namespace Novorado
{
	namespace Partition
	{
				
		/*! Partition bin */
		struct part
		{
			CutLine cut;
			std::vector<Cell*,Bridge::Id> bin1, bin2;
			
			constexpr void reserve(size_t sz) noexcept
			{ 
				bin1.reserve(sz), bin2.reserve(sz); 
			}
			
			constexpr void setRect(const Rect& r1,
				const Rect& r2) noexcept
			{ 
				bin1.box=r1;bin2.box=r2; 
			}
		};

	}
}
#endif//_BIN_H
