#ifndef _PARTITION_H
#define _PARTITION_H

#include "celllist.h"
#include "bucket.h"
#include "bin.h"
#include "cutline.h"

namespace Novorado
{
	namespace Partition
	{
		// Partitioner will not proceed for small bins
		constexpr auto MIN_BIN_SIZE = 2;
		// Square treshhold 0.1=10%
		constexpr auto SQUARE_TOLERANCE = 0.1;

		class Partition : public IdBridge
		{
			public:
				Partition();
				virtual ~Partition();

				CellList m_Locker;
				Bucket m_Bucket;

				Square GetSquare();
				Weight GetGain();

				void preset(const std::vector<Cell*>& fix,const std::vector<Cell*>& ini);

			protected:
			private:
				friend class NetlistHypergraph;
		};

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

		class Solution
		{
			public:
				struct CellRecord {
					CellRecord():cell(NULL),gain(0),p(NULL){}
					std::shared_ptr<Cell> cell;
					Weight gain;
					std::shared_ptr<Partition> p;
					};
				Solution(Partition&,Partition&,std::vector<Cell>&);
				Solution(Partition&,Partition&);
				virtual ~Solution();

				Solution& operator=(const Solution&);

				// The closer to 1 the better
				bool IsInitial() const { return !s1||!s2; }
				float Ratio() const { return float(std::max(s1,s2)) / float(std::min(s1,s2)); }

				// Quality of the solution
				Weight Cut() const { return g1+g2; }

				void AddCell(std::shared_ptr<Cell>);

				static bool SolutionImproved(
					Solution&,

					Square s2_0,
					Square s2_1,
					Weight g2);

				// Interchanges cells in lockers and initializes gain
				void WriteLockers(CellList& l0, CellList& l1);

			protected:
				std::shared_ptr<Partition> p1, p2;
				Weight g1,g2;
				Square s1,s2;
				std::vector<CellRecord> m_Recs;
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
#endif//_PARTITION_H

