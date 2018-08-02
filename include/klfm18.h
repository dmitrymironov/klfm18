#ifndef _PARTITION_H
#define _PARTITION_H

#include "cutline.h"
#include "pin.h"
#include "net.h"
#include "cell.h"
#include <vector>
#include <list>

namespace Novorado
{
	namespace Partition
	{
		// Partitioner will not proceed for small bins
		constexpr auto MIN_BIN_SIZE = 2;
		// Square treshhold 0.1=10%
		constexpr auto SQUARE_TOLERANCE = 0.1;
				
		/*! Partition bin */
		struct part
		{
			CutLine cut;
			std::vector<Cell*,RectBridge> bin1, bin2;
			
			constexpr void reserve(size_t sz) noexcept
			{ 
				bin1.reserve(sz), bin2.reserve(sz); 
			}
			
			constexpr void setRect(const Rect& r1,
				const Rect& r2) noexcept
			{ 
				bin1.box=r1;bin2.box=r2; 
			}
		};

		class CellList
		{
			std::vector<long> cellId2cells;
			std::vector<Cell*> cells;
			bool dirty=false; // dirty when there are NULLs in cells() array
			void pack();
			public:
				class Iterator
				{
					std::shared_ptr<CellList> L;
					long idx=-1;
					friend CellList;
					public:
						Iterator(CellList* l,long _idx):L(l),idx(_idx){}
						bool operator!=(const Iterator& i) const { if(L.getConst()->dirty) throw std::logic_error("integrity"); return idx!=i.idx; }
						bool operator==(const Iterator& i) const { if(L.getConst()->dirty) throw std::logic_error("integrity"); return idx==i.idx; }
						Iterator operator++() { if(L->dirty)L->pack(); idx++; return *this; }
						Iterator operator++(int /* mark postfix*/) { if(L->dirty)L->pack(); Iterator rv(*this); idx++; return rv; }
						Iterator operator--(int) { if(L->dirty)L->pack(); Iterator rv(*this); idx--; return rv; }
						Cell* operator->() { if(L->dirty)L->pack(); return L->cells[idx]; }
						Cell& operator*() { if(L->dirty)L->pack(); return *L->cells[idx]; }
				};
				friend Iterator;
				Iterator begin() { return Iterator(this,0); }
				Iterator end() { return Iterator(this,cells.size());}
				bool empty() { if(dirty) pack(); return cells.empty(); }
				bool size() { if(dirty) pack(); return cells.size(); }
				// Move cell in position <posInFrom> to list <from> in <posTo>
				void splice(Iterator posTo,CellList& from,Iterator posInFrom);
				// Move all cells <from> to <posTo>
				void splice(Iterator posTo,CellList& from);
				void insertCell(Iterator,Cell&);
				void removeCell(Cell&);
				Iterator find(Cell&);

				CellList();
				virtual ~CellList();
				void TransferTo(Iterator,CellList&,bool UpdateGain=true);
				void TransferAllFrom(CellList&);
				Square GetSquare();
				std::string dbg();
				void SetCellGain(Weight gain);
				Weight GetSumGain();
				Weight IncrementSumGain(Weight g);
				void InvalidateGain() { flags.GainComputed=false; m_SumGain=0; }
				void init(const std::vector<Cell*>&);
				operator std::vector<const Novorado::Netlist::Node*>();
				// Will clear target vector
				void copyWithoutFixed(npvec& v);
			protected:
			private:
				struct {
					bool GainComputed: 1;
					bool SquareComputed: 1;
					} flags;
				Weight m_SumGain;
				Square m_Square;
		};

		template<class Cell,class Partition,
			typename Square=long,typename Weight=long>
		class Bucket : public std::map<Weight,CellList>
		{
			public:
				virtual ~Bucket();
				Bucket(const Bucket& other);
				Bucket& operator=(const Bucket& other);
				void SetPartition(std::shared_ptr<Partition> p) 
				{ 
					m_Partition=p; 
				}
				void FillByGain(CellList&);
				void dbg(long);
				Square GetSquare() const { return m_Square; }
				void SubtractSquare(Square s) { m_Square-=s; }
				Weight GetGain() const { return m_SumGain;}
				void IncrementGain(Weight g);
			protected:
			private:
				Square m_Square;
				Weight m_SumGain;
				std::shared_ptr<Partition> m_Partition;
				Bucket();
				friend class Partition;
		};

		template <class Id> 
		class Partition : public Id
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

