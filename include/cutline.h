#ifndef _CUTLINE_H
#define _CUTLINE_H

#include <bridge.h>

#include <atomic>

namespace Novorado
{
	namespace Partition
	{

		/*! Cutline class to facilitate hierarchial cuts */
		struct CutLine
		{
			/** Cut enum type
			 * Control bisection direction
			 */
			enum struct Direction
			{
				Horizontal, /**< horizontal cuts */
				Vertical /**< vertical cuts */
			};

			std::atomic<Direction> dir{Direction::Vertical}; /*!< cut
															direction */
			std::atomic<Coordinate> l{-1}; /*!< Cut line coordinate */

			//! Default cutline ctor: vertical with invalid coordinate
			/*!
			 \dir cutline direction (default is vertical)
			 \l corodinate of the cut line (default is invalid, negative
			 */
			CutLine(Direction dir=Direction::Vertical,
					Coordinate l=-1) noexcept: dir{dir},l{l}{}

			//! cutline ctor: cut rectangle in half(vertical by default)
			/*!
			 \r rectangle to cut
			 \dir cut direction
			 */
			CutLine(const Bridge::Rect&& r,
				Direction dir=Direction::Vertical) noexcept:
					dir{dir},
					l(dir==Direction::Vertical?
						r.hCenter():r.vCenter()){}

			//! Flip cut direction for any rectangle
			void switchDir() noexcept
			{
				if(dir==Direction::Vertical) dir=Direction::Horizontal;
					else dir=Direction::Horizontal;
			}

			//! Split
			inline void split(const Bridge::Rect&& s,
				Bridge::Rect&& r1,Bridge::Rect&& r2) noexcept
			{
				r1=r2=s;
				if(dir==Direction::Vertical) l=r1.right()=r2.left()=l;
					else l=r1.top()=r2.bottom()=l;
			}
		};
	}
}

#endif//_CUTLINE_H
