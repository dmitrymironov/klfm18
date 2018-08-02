#ifndef _SOLUTION_H
#define _SOLUTION_H

#include "partition.h"

namespace Novorado
{
	namespace Partition
	{
		class Solution
		{
			public:
				struct CellRecord {
					CellRecord():cell(NULL),gain(0),p(NULL){}
					Cell* cell;
					Weight gain;
					Partition* p;
					};
				Solution(Partition&,Partition&,std::vector<Cell>&);
				Solution(Partition&,Partition&);
				virtual ~Solution();

				Solution& operator=(const Solution&);

				// The closer to 1 the better
				bool IsInitial() const { return !s1||!s2; }

				float Ratio() const
				{
					return float(std::max(s1,s2)) / float(std::min(s1,s2));
				}

				// Quality of the solution
				Weight Cut() const { return g1+g2; }

				void AddCell(Cell*);

				static bool SolutionImproved(
					Solution&,

					Square s2_0,
					Square s2_1,
					Weight g2);

				// Interchanges cells in lockers and initializes gain
				void WriteLockers(CellList& l0, CellList& l1);

			protected:
				Partition *p1, *p2;
				Weight g1,g2;
				Square s1,s2;
				std::vector<CellRecord> m_Recs;
		};
	}
}

#endif//_SOLUTION_H
