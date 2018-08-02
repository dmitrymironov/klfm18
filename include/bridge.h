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

		namespace Bridge
		{
			struct Rect
			{
				Rect() = default;
				Rect(const Rect&) = default;

				constexpr Coordinate vCenter() const noexcept
				{
					return (m_top+m_bottom)/2;
				}

				constexpr Coordinate hCenter() const noexcept
				{
					return (m_left+m_right)/2;
				}

				constexpr Coordinate& left() noexcept
				{
					return m_left;
				}

				constexpr Coordinate& bottom() noexcept
				{
					return m_bottom;
				}

				constexpr Coordinate& right() noexcept
				{
					return m_right;
				}

				constexpr Coordinate& top() noexcept
				{
					return m_top;
				}

				private:

					Coordinate
						m_left{0},m_right{-1},m_top{0},m_bottom{-1};
			};

			struct Id
			{

				Id(Index index=-1, string name=string())
				{
					SetId(index);
					SetName(name);
				}

				Id(const Id&) = default;

				constexpr auto GetId() const noexcept
				{
					return m_index;
				}

				constexpr unsigned int GetUnsignedId() const noexcept
				{
					return static_cast<unsigned int>(m_index);
				}

				auto GetName() const noexcept
				{
					return m_name;
				}

				constexpr void SetId(Index index) noexcept
				{
					m_index = index;
				}

				void SetName(string name) noexcept
				{
					m_name = name;
				}
				private:
					// Quick implementation, need to store those
					// in optimized containers
					Index m_index;
					string m_name;
			};
		}
	}
}
#endif//_BRIDGE_H
