#ifndef _CELL_H
#define _CELL_H

#include "bridge.h"
#include "net.h"
#include <list>

namespace Novorado
{
	namespace Partition
	{
		class Partition;
		class Pin;

		using Square = long;

		class Cell : public Bridge::Id
		{
			public:
				Cell();
				Cell(Cell& other);
				Cell(const Cell& other);
				Cell& operator=(Cell&);
				virtual ~Cell();
				Square GetSquare() const { return m_Square; }
				void SetSquare(Square val) { m_Square = val; }
				Partition* GetPartition();
				void SetPartition(Partition*);
				Weight GetGain() const { return m_Gain; }
				void SetGain(Weight w) { m_Gain=w; }
				void IncrementGain(Weight w);
				bool operator==(const Cell& c) const
				{
					return c.GetId()==GetId();
				}
				void MoveToLocker(bool f=true);
				bool IsInLocker() const { return flags.inLocker; }
				bool IsFixed() const { return flags.fixed; }
				void SetFixed(bool f=true) { flags.fixed=f; }

				std::list<Pin> m_Pins;

			private:
				struct Flags
				{
					Flags():inLocker(false),fixed(false){}
					bool inLocker	: 1;
					bool fixed	: 1;
				} flags;

				Weight m_Gain=0;
				Partition* m_PartitionPtr{nullptr};
				Square m_Square=0;
		};
	}
}
#endif//_CELL_H
