#ifndef _PARTITION_H
#define _PARTITION_H

#include	"abstract_netlist.h"
#include 	"vector2d.h"

// Partitioner will not proceed for small bins
#define MIN_BIN_SIZE 2
// Square treshhold 0.1=10%
#define SQUARE_TOLERANCE 0.1

namespace Novorado
{
	namespace Partition
	{
		class Partition;
		class Cell;
		class Net;

		struct npvec : public std::vector<const Novorado::Netlist::Node*>
		{
			Novorado::Rect box;
			npvec& operator=(const std::vector<const Novorado::Netlist::Node*>& v)
			{
				std::vector<const Novorado::Netlist::Node*>::operator=(v);
				return *this;
			}
		};

		struct CutLine
		{
			enum DIR { HOR, VER } dir = VER;
			Novorado::Coord l=-1;

			CutLine(DIR d=VER, Novorado::Coord loc=-1):dir(d),l(loc){}

			CutLine(const Novorado::Rect& r,DIR d=VER):dir(d),l(dir==VER?r.hCenter():r.vCenter()){}

			void switchDir() { if(dir==VER) dir=HOR; else dir=VER; }

			void split(const Novorado::Rect& s,Novorado::Rect& r1,Novorado::Rect& r2)
			{
				r1=r2=s;
				if(dir==VER) l=r1.right()=r2.left()=l;
				else l=r1.top()=r2.bottom()=l;
			}
		};

		struct part
		{
			CutLine cut;
			npvec bin1, bin2;
			void reserve(size_t sz)  { bin1.reserve(sz), bin2.reserve(sz); }
			void setRect(const Novorado::Rect& r1,const Novorado::Rect& r2) { bin1.box=r1;bin2.box=r2; }
		};

		class Pin : public Novorado::Object
		{
		public:
			Pin();
			virtual ~Pin();

			// ATTENTION! Does not copy cell or net pointers
			Pin(const Pin& other);
			Pin& operator=(const Pin& other);

			Cell* GetCell(){return m_Cell;}
			void SetCell(Cell* val){m_Cell = val;}

			Net* GetNet(){return m_Net;}
			void SetNet(Net* val);

			#ifdef CHECK_LOGIC
			// Overloading object method for consistency checking
			virtual void SetName(const std::string&);
			#endif // CHECK_LOGIC

		protected:
		private:
			Novorado::Ptr<Cell> m_Cell;
			Novorado::Ptr<Net> m_Net;
		};

		class Net : public Novorado::Object
		{
			public:
				typedef long Weight;
				Net();
				virtual ~Net();
				Net(const Net& other);
				Net& operator=(const Net& other);
				void SetWeight(Weight w) { m_Weight = w; }
				Weight GetWeight() const { return m_Weight; }
				void AddPin(Pin*);
				unsigned int Dim() const { return m_Pins.size(); }
				unsigned int Dim(Partition*);
				std::vector<Pin*> m_Pins;

				void SetAbstractNet(const Novorado::Netlist::Net* p) { aNet=p; }
				operator const Novorado::Netlist::Net*() { return aNet; }

			protected:
			private:
				Novorado::Ptr<const Novorado::Netlist::Net> aNet;
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
				Novorado::Ptr<Partition> m_PartitionPtr;
				Square m_Square=0;
				Novorado::Ptr<const Novorado::Netlist::Node> node;
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
					Novorado::Ptr<CellList> L;
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
				Novorado::Ptr<Partition> m_Partition;
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
				Novorado::Ptr<std::vector<Cell>> m_AllCells;
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
				Novorado::Ptr<NetlistHypergraph> graph;
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

