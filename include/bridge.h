#ifndef _BRIDGE_H
#define _BRIDGE_H

#include <string>
#include <memory>

namespace Novorado
{
	namespace Partition
	{
		using string = std::string;
		using Coordinate = long;
		using Index = long;
		
		struct RectBridge
		{
			virtual Coordinate vCenter() const noexcept = 0;
			virtual Coordinate hCenter() const noexcept = 0;
			virtual Coordinate& left() noexcept = 0;
			virtual Coordinate& bottom() noexcept = 0;
			virtual Coordinate& right() noexcept = 0;
			virtual Coordinate& top() noexcept = 0;
		};
		
		using RectPt = std::shared_ptr<RectBridge>;
		using CRectPt = std::shared_ptr<const RectBridge>;
		
		struct IdBridge
		{
			virtual Index GetId() const noexcept = 0;
			virtual string GetName() const noexcept = 0;
		};
	}
}
#endif//_BRIDGE_H
