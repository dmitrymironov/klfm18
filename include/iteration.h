#ifndef _ITERATION_H
#define _ITERATION_H

#include "partition.h"
#include "hypergraph.h"

namespace Novorado
{
	namespace Partition
	{
		class CellMove
		{
			public:
				CellMove(Partition&,Partition&);
				virtual ~CellMove();
			protected:
				virtual void run() = 0;
				Partition& p0;
				Partition& p1;
		};

		class RandomDistribution : public CellMove
		{
			public:
				RandomDistribution(Partition&,Partition&);
				virtual ~RandomDistribution();
			protected:
				void run();
			private:
		};

		class Iteration : public CellMove
		{
			public:
				Iteration(NetlistHypergraph*);
				virtual ~Iteration();
				Weight GetImprovement() const { return m_Improvement; }
				void moveCell(Partition*,Partition*);

				void moveLeft() { moveCell(&p1,&p0); }
				void moveRight() { moveCell(&p0,&p1); }

				void run();

		   protected:

			private:
				Weight m_Improvement;
				NetlistHypergraph* graph;
		};
	}
}
#endif//_ITERATION_H
