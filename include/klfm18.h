#ifndef _PARTITION_H
#define _PARTITION_H

#include <string>
#include <vector>
#include <memory>
#include <atomic>

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

		template <class Id, class Pin, class Partition> class Net : 
			public Id
		{
			public:
				using Weight = long;
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

		class Cell : public Novorado::Object
		{
			public:
				typedef long Square;
				Cell();
				Cell(Cell& other);
				Cell(const Cell& other);
				Cell& operator=(Cell&);
				virtual ~Cell();
				Square GetSquare() const { return m_Square; }
				void SetSquare(Square val) { m_Square = val; }
				Partition* GetPartition();
				void SetPartition(Partition*);
				Net::Weight GetGain() const { return m_Gain; }
				void SetGain(Net::Weight w) { m_Gain=w; }
				void IncrementGain(Net::Weight w);
				bool operator==(const Cell& c) const { return c.GetId()==GetId(); }
				void MoveToLocker(bool f=true);
				bool IsInLocker() const { return flags.inLocker; }
				bool IsFixed() const { return flags.fixed; }
				void SetFixed(bool f=true) { flags.fixed=f; }

				void SetNode(const Novorado::Netlist::Node* n) { node=n; }
				operator const Novorado::Netlist::Node*() { return node; }

				std::list<Pin> m_Pins;

			private:
				struct Flags
				{
					Flags():inLocker(false),fixed(false){}
					bool inLocker	: 1;
					bool fixed	: 1;
				} flags;

				Net::Weight m_Gain=0;
				std::shared_ptr<Partition> m_PartitionPtr;
				Square m_Square=0;
				std::shared_ptr<const Novorado::Netlist::Node> node;
		};

		class CellList /* vectorize : public std::list<Cell> */
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
				Cell::Square GetSquare();
				std::string dbg();
				void SetCellGain(Net::Weight gain);
				Net::Weight GetSumGain();
				Net::Weight IncrementSumGain(Net::Weight g);
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
				Net::Weight m_SumGain;
				Cell::Square m_Square;
		};

		class Bucket : public std::map<Net::Weight,CellList>
		{
			public:
				virtual ~Bucket();
				Bucket(const Bucket& other);
				Bucket& operator=(const Bucket& other);
				void SetPartition(Partition* p) { m_Partition=p; }
				void FillByGain(CellList&);
				void dbg(long);
				Cell::Square GetSquare() const { return m_Square; }
				void SubtractSquare(Cell::Square s) { m_Square-=s; }
				Net::Weight GetGain() const { return m_SumGain;}
				void IncrementGain(Net::Weight g);
			protected:
			private:
				Cell::Square m_Square;
				Net::Weight m_SumGain;
				std::shared_ptr<Partition> m_Partition;
				Bucket();
				friend class Partition;
		};

		class Partition : public Novorado::Object
		{
			public:
				Partition();
				virtual ~Partition();

				CellList m_Locker;
				Bucket m_Bucket;

				Cell::Square GetSquare();
				Net::Weight GetGain();

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
					Cell* cell;
					Net::Weight gain;
					Partition* p;
					};
				Solution(Partition&,Partition&,std::vector<Cell>&);
				Solution(Partition&,Partition&);
				virtual ~Solution();

				Solution& operator=(const Solution&);

				// The closer to 1 the better
				bool IsInitial() const { return !s1||!s2; }
				float Ratio() const { return float(std::max(s1,s2)) / float(std::min(s1,s2)); }

				// Quality of the solution
				Net::Weight Cut() const { return g1+g2; }

				void AddCell(Cell*);

				static bool SolutionImproved(
					Solution&,

					Cell::Square s2_0,
					Cell::Square s2_1,
					Net::Weight g2);

				// Interchanges cells in lockers and initializes gain
				void WriteLockers(CellList& l0, CellList& l1);

			protected:
				Partition *p1, *p2;
				Net::Weight g1,g2;
				Cell::Square s1,s2;
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
				Net::Weight UpdateGains(Cell&);

				struct CutStat {
					long m_NetCut;
					Net::Weight m_totWeigth;
					CutStat() { m_NetCut=0; m_totWeigth=0; }
					};

				CutStat GetStats(std::ofstream&,bool fWrite=true);

			private:
		};

		class Iteration : public CellMove
		{
			public:
				Iteration(NetlistHypergraph*);
				virtual ~Iteration();
				Net::Weight GetImprovement() const { return m_Improvement; }
				void moveCell(Partition*,Partition*);

				void moveLeft() { moveCell(&p1,&p0); }
				void moveRight() { moveCell(&p0,&p1); }

				void run();

		   protected:

			private:
				Net::Weight m_Improvement;
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

