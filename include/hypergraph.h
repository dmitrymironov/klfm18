#ifndef _HYPERGRAPH_H
#define _HYPERGRAPH_H

#include "solution.h"

namespace Novorado
{
	namespace Partition
	{
		class NetlistHypergraph
		{
			protected:
				friend class Iteration;
				std::shared_ptr<std::vector<Cell>> m_AllCells;
				Novorado::Bracket<Cell> pins, instances;

			public:
				NetlistHypergraph();
				virtual ~NetlistHypergraph();
				std::vector<Net> nets;
				Partition p0,p1;
				Solution bestSolution;

				void FillBuckets();
				Weight UpdateGains(Cell&);

				struct CutStat {
					long m_NetCut;
					Weight m_totWeight;
					CutStat() { m_NetCut=0; m_totWeight=0; }
					};

				CutStat GetStats(std::ofstream&,bool fWrite=true);

			private:
		};
	}
}
#endif//_HYPERGRAPH_H
