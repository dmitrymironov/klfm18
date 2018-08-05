#ifndef _HYPERGRAPH_H
#define _HYPERGRAPH_H

#include "solution.h"
#include "bracket.h"

namespace Novorado
{
	namespace Partition
	{
		struct NetlistHypergraph
		{

				std::shared_ptr<std::vector<Cell>> m_AllCells;
				Novorado::Bracket<Cell> pins, instances;

				NetlistHypergraph();
				virtual ~NetlistHypergraph();
				std::vector<Net> nets;
				Partition p0,p1;
				Solution bestSolution;

				void InitializeLockers();
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
