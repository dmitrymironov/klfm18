#ifndef _CELLLIST_H
#define _CELLLIST_H

#include "cell.h"

namespace Novorado
{
	namespace Partition
	{
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
						bool operator!=(const Iterator& i) const { if(L->dirty) throw std::logic_error("integrity"); return idx!=i.idx; }
						bool operator==(const Iterator& i) const { if(L->dirty) throw std::logic_error("integrity"); return idx==i.idx; }
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
			protected:
			private:
				struct {
					bool GainComputed: 1;
					bool SquareComputed: 1;
					} flags;
				Weight m_SumGain;
				Square m_Square;
		};
	}
}
#endif//_CELLLIST_H
