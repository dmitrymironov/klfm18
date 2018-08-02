#ifndef _BIN_H
#define _BIN_H

#include "cell.h"
#include "cutline.h"

namespace Novorado
{
	namespace Partition
	{
		// Temporary proxy class that will be refactored out
		struct npvec : public std::vector<void*>
		{
			Bridge::Rect box;
			npvec& operator=(const std::vector<void*>& v)
			{
				std::vector<void*>::operator=(v);
				return *this;
			}
		};

		/*! Partition bin */
		struct part
		{
			CutLine cut;
			npvec bin1, bin2;

			void reserve(size_t sz) noexcept
			{
				bin1.reserve(sz), bin2.reserve(sz);
			}

			void setRect(const Bridge::Rect& r1,const Bridge::Rect& r2) noexcept
			{
				bin1.box=r1;bin2.box=r2;
			}
		};

	}
}
#endif//_BIN_H
