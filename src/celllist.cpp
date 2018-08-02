#include "celllist.h"

using namespace Novorado::Partition;

void CellList::copyWithoutFixed(npvec& v)
{
	v.clear();
	v.reserve(size());
	for(auto& i:*this) if(!i.IsFixed()) v.push_back(i);
}

CellList::operator std::vector<const Novorado::Netlist::Node*>()
{
	std::vector<const Novorado::Netlist::Node*> rv;
	rv.resize(cells.size());
	long idx=0;for(auto* cell:cells) rv[idx++]=*cell;
	return rv;
}

void CellList::init(const std::vector<Cell*>& v)
{
	for(auto* cell:v) insertCell(end(),*cell);
}


CellList::CellList()
{
	//ctor
	flags.SquareComputed=false;
	flags.GainComputed=false;
	m_Square=0;
	m_SumGain=0;
}

CellList::~CellList()
{
	//dtor
}

Weight CellList::GetSumGain()
{
	if(flags.GainComputed) return m_SumGain;
	flags.GainComputed=true;
	m_SumGain=0;
	for(auto& cell:*this) m_SumGain+=cell.GetGain();
	return m_SumGain;
}

CellList::Iterator CellList::find(Cell& cell)
{
	if(cell.GetId()<cellId2cells.size() && cellId2cells[cell.GetId()]!=-1)
	{
		return Iterator(this,cellId2cells[cell.GetId()]);
	}

	return end();
}

void CellList::removeCell(Cell& cell)
{

	long idx=cellId2cells[cell.GetId()];
	#ifdef CHECK_LOGIC
	if(cells[idx]!=&cell)
	{
		throw std::logic_error("data corruption");
	}
	#endif
	cellId2cells[cell.GetId()]=-1;
	cells[idx]=NULL;

	dirty=true;
}

void CellList::insertCell(Iterator pos,Cell& cell)
{
	// Enlarge cells vector if needed
	if(pos.idx>=cells.size())
	{
		// Should add 1 element
		if(pos.idx!=cells.size()) dirty=true;
		size_t cur=cells.size();
		cells.resize(pos.idx+1);
		for(;cur<cells.size();cur++) cells[cur]=NULL;
	}

	// Enlarge id vector if needed
	if(cell.GetId()>=cellId2cells.size())
	{
		size_t cur=cellId2cells.size();
		cellId2cells.resize(cell.GetId()+1);
		for(;cur<cellId2cells.size();cur++) cellId2cells[cur]=-1;
	}

	#ifdef CHECK_LOGIC
	if(cells[pos.idx]!=NULL)
	{
		throw std::logic_error("Cell already initialized");
	}
	#endif // CHECK_LOGIC

	cells[pos.idx]=&cell;

	#ifdef CHECK_LOGIC
	if(cellId2cells[cell.GetId()]!=-1)
	{
		throw std::logic_error("Data integrity violation");
	}
	#endif // CHECK_LOGIC

	cellId2cells[cell.GetId()]=pos.idx;
}

void CellList::pack()
{
	#if 0 && defined(ALGORITHM_VERBOSE)
	std::cout << "CellList::pack() initial size=" << cells.size() ;
	#endif // ALGORITHM_VERBOSE

	// remove all empty cells
    cells.erase(
    	std::remove_if(
			cells.begin(),
			cells.end(),
			[](Cell* pt) { return pt==NULL; }
			)
    	);

	Novorado::ID i=0;
	for(Cell* pt:cells) cellId2cells[pt->GetId()]=i++;

	dirty=false;

	#if 0 && defined(ALGORITHM_VERBOSE)
	std::cout << " size=" << cells.size() << std::endl;
	#endif // ALGORITHM_VERBOSE
}

void CellList::splice(Iterator posTo,CellList& from,Iterator posInFrom)
{
	Cell& cell=*posInFrom;
	#ifdef CHECK_LOGIC
	if(cell.IsFixed())
	{
		throw std::logic_error("Attempting to move fixed cell");
	}
	#endif // CHECK_LOGIC

	from.removeCell(cell);

	insertCell(posTo,cell);
}

void CellList::splice(Iterator posTo,CellList& from)
{
	while(!from.empty()) splice(posTo++,from,from.begin());
}

void CellList::TransferTo(CellList::Iterator it, CellList& cl,bool UpdateGain)
{
	Cell& cell=*it;

	#ifdef CHECK_LOGIC
	if(cell.IsFixed())
	{
		throw std::logic_error("Attempting to move fixed cell");
	}
	#endif // CHECK_LOGIC

	#if 0 && defined(ALGORITHM_VERBOSE)
	std::cout << "CellList::TransferTo() cell " << cell.GetName() << std::endl;
	#endif

	if(UpdateGain){
		IncrementSumGain(-cell.GetGain());
		cl.IncrementSumGain(cell.GetGain());
		}

	m_Square=GetSquare()-cell.GetSquare();
	cl.m_Square=cl.GetSquare()+cell.GetSquare();

	cl.splice(cl.end(),*this,it);
}

Square CellList::GetSquare()
{
	if(flags.SquareComputed) return m_Square;
	m_Square=0;
	flags.SquareComputed=true;
	for(auto i:cells) m_Square+=i->GetSquare();
	return m_Square;
}

void CellList::TransferAllFrom(CellList& cl)
{
	m_Square=GetSquare()+cl.GetSquare();
	cl.m_Square=0;
	splice(end(),cl);
}

std::string CellList::dbg()
{
	std::stringstream ss;
	ss << " SQ=" << GetSquare() << " GAIN=" << GetSumGain();
	for(auto i=begin();i!=end();i++)
	   ss << " " << i->GetName() << (i->IsInLocker()?"*LOCK ":"") << " G" << i->GetGain() << "_p" << i->GetPartition()->GetId();
	return ss.str();
}

void CellList::SetCellGain(Weight g)
{
 	//std::for_each(begin(),end(),std::bind2nd(std::mem_fun_ref(&Cell::SetGain), g));
 	for(auto& cell:*this) cell.SetGain(g);
}

Weight CellList::IncrementSumGain(Weight g)
{
	#if 0 && defined(ALGORITHM_VERBOSE)
	std::cout << "CellList::IncrementSumGain(" << g << ") " << GetSumGain() << " => " << (GetSumGain()+g) << std::endl;
	#endif
	return m_SumGain=GetSumGain()+g;
}
