#ifndef _PARTITION_H
#define _PARTITION_H

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <list>

namespace Novorado
{
	namespace Partition
	{
		// Partitioner will not proceed for small bins
		constexpr auto MIN_BIN_SIZE = 2;
		// Square treshhold 0.1=10%
		constexpr auto SQUARE_TOLERANCE = 0.1;

		/*! Cutline class to facilitate hierarchial cuts */
		template <class Rect, typename Coordinate = long> struct CutLine
		{
			/** Cut enum type 
			 * Control bisection direction
			 */
			enum struct Direction 
			{ 
				Horizontal, /**< horizontal cuts */
				Vertical /**< vertical cuts */
			};
			
			std::atomic<Direction> dir{Direction::Vertical}; /*!< cut
															direction */
			std::atomic<Coordinate> l{-1}; /*!< Cut line coordinate */

			//! Default cutline ctor: vertical with invalid coordinate
			/*!
			 \dir cutline direction (default is vertical)
			 \l corodinate of the cut line (default is invalid, negative
			 */
			constexpr CutLine(Direction dir=Direction::Vertical, 
					Coordinate l=-1) noexcept: dir{dir},l{l}{}

			//! cutline ctor: cut rectangle in half(vertical by default)
			/*!
			 \r rectangle to cut
			 \dir cut direction
			 */
			constexpr CutLine(const Rect&& r,
				Direction dir=Direction::Vertical) noexcept:
					dir{dir},
					l(dir==Direction::Vertical?
						r.hCenter():r.vCenter()){}

			//! Flip cut direction for any rectangle
			constexpr void switchDir() noexcept 
			{ 
				if(dir==Direction::Vertical) dir=Direction::Horizontal; 
					else dir=Direction::Horizontal; 
			}

			//! Split 
			inline constexpr void split(const Rect&& s,
				Rect&& r1,Rect&& r2) noexcept 
			{
				r1=r2=s;
				if(dir==Direction::Vertical) l=r1.right()=r2.left()=l;
					else l=r1.top()=r2.bottom()=l;
			}
		};
		
		/*! Partition bin */
		template <class Cell, class Rect> struct part
		{
			CutLine<Rect> cut;
			std::vector<Cell*,Rect> bin1, bin2;
			
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

		template <class Id, class Net, class Cell> class Pin : public Id
		{
		public:
		
			constexpr Pin() noexcept = default;
			virtual ~Pin() noexcept = default;

			constexpr Pin(const Pin&& other) noexcept;
			constexpr Pin& operator=(const Pin&& other) noexcept;

			constexpr auto GetCell() noexcept 
			{
				return m_Cell;
			}
			
			constexpr void SetCell(std::shared_ptr<Cell> val) noexcept 
			{
				m_Cell = val;
			}

			constexpr auto GetNet() noexcept {return m_Net;}
			
			constexpr void SetNet(std::shared_ptr<Net> val) noexcept;

			#ifdef CHECK_LOGIC
			// Overloading object method for consistency checking
			virtual void SetName(const std::string&);
			#endif // CHECK_LOGIC

		protected:
		private:
			std::shared_ptr<Cell> m_Cell;
			std::shared_ptr<Net> m_Net;
		};

		template <class Id,class Pin,class Partition,
			typename Weight=long> 
		class Net : public Id
		{
			public:
				Net();
				virtual ~Net();
				Net(const Net&& other);
				Net& operator=(const Net&& other);
				void SetWeight(Weight w) { m_Weight = w; }
				Weight GetWeight() const { return m_Weight; }
				void AddPin(std::shared_ptr<Pin>);
				auto Dim() const { return m_Pins.size(); }
				auto Dim(std::shared_ptr<Partition>);
				std::vector<Pin*> m_Pins;

			protected:
			private:
				Weight m_Weight;
		};

		template <class Id,class Partition,class Pin,
			typename Square=long,typename Weight=long> 
		class Cell : public Id
		{
			public:
				Cell();
				Cell(Cell& other);
				Cell(const Cell& other);
				Cell& operator=(Cell&);
				virtual ~Cell();
				Square GetSquare() const { return m_Square; }
				void SetSquare(Square val) { m_Square = val; }
				std::shared_ptr<Partition> GetPartition();
				void SetPartition(std::shared_ptr<Partition>);
				Weight GetGain() const { return m_Gain; }
				void SetGain(Weight w) { m_Gain=w; }
				void IncrementGain(Weight w);
				bool operator==(const Cell& c) const 
				{ 
					return c.Id::GetId()==Id::GetId(); 
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
				std::shared_ptr<Partition> m_PartitionPtr;
				Square m_Square=0;
		};

		template<class Cell,typename Square=long,typename Weight=long> 
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

