#ifndef _KLFM18_H
#define _KLFM18_H

#include "bin.h"
#include "solution.h"

namespace Novorado
{
	namespace Partition
	{
		// Partitioner will not proceed for small bins
		constexpr auto MIN_BIN_SIZE = 2;
		// Square treshhold 0.1=10%
		constexpr auto SQUARE_TOLERANCE = 0.1;

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
				std::shared_ptr<NetlistHypergraph> graph;
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


		//////////////////////////////////////////////////////////////////////////////////////////
		//
		// Algorihtm interface
		//
		//////////////////////////////////////////////////////////////////////////////////////////

		class KLFM : public NetlistHypergraph
		{
			std::vector<Cell*> C(const npvec&);
			void CreateGraph(const Novorado::Netlist::NetList*,const part&, part&);

			public:
				// Partition result is returned in last reference, it is also used as initial solution
				KLFM(
						const Novorado::Netlist::NetList*, // Source netlist
						const part& fixed, // Fixed part of bins (pre-allocated)
						part& initial // Initial distribution & resulting partition
						);
		};
	};
};
#endif//_KLFM18_H

