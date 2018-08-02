//#define ALGORITHM_VERBOSE

// Uncomment following line so exectuble becomes unit test for partitioning algorithm
//#define KLFM_TEST 1

#ifdef KLFM_TEST
#define CHECK_LOGIC 1
#define PRINT_PROGRESS 1
#define ALGORITHM_VERBOSE 1
#endif // KLFM_TEST

#include "klfm18.h"
#include <sstream>
#include <fstream>

using namespace Novorado::Partition;

KLFM::KLFM(const Novorado::Netlist::NetList* netlist,const part& fixed, part& initial)
{
	CreateGraph(netlist,fixed,initial);

	FillBuckets();

	for(int iter_cnt=0;;iter_cnt++){

		Iteration step(this);

		step.run();

		#ifdef PRINT_PROGRESS
		std::cout << std::endl;
		#endif // PRINT_PROGRESS

		if(step.GetImprovement()<=0) break;

		bestSolution.WriteLockers(p0.m_Locker,p1.m_Locker);

		#ifdef PRINT_PROGRESS
		std::cout << "ITERATION " << iter_cnt << ", IMPROVEMENT " << step.GetImprovement() << std::endl;
		#endif
		}

	// Copy over the best soltion
	p0.m_Locker.copyWithoutFixed(initial.bin1);
	p1.m_Locker.copyWithoutFixed(initial.bin2);
}

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

void KLFM::CreateGraph(const Novorado::Netlist::NetList* netlist,const part& fixed, part& initial)
{
	// Allocate data, init arrays
	m_AllCells=new std::vector<Cell>(netlist->GetNumInstances()+netlist->GetNumPins());
	instances.init(&(*m_AllCells)[0],netlist->GetNumInstances());
	pins.init(&(*m_AllCells)[netlist->GetNumInstances()],netlist->GetNumPins());
	nets.resize(netlist->GetNumNets());

	// Set id's and pointers
	Novorado::ID idx=0;
	idx=0;for(auto& cell: *m_AllCells) cell.SetId(idx++);
	idx=0;for(auto& net: nets) net.SetId(idx++);
	idx=0;for(auto& cell:instances) cell.SetNode(netlist->instance(idx++));
	idx=0;for(auto& pin:pins) pin.SetNode(netlist->externalPin(idx++));

	// Pre-set fixed pins and initial distribution
	p0.preset(C(fixed.bin1),C(initial.bin1)),p1.preset(C(fixed.bin2),C(initial.bin2));

	// Build netlist and create pins
	for(auto const* aNet:Novorado::Netlist::nets(netlist))
	{
		auto& net=nets[aNet->GetId()];
		for(long t=0;t<aNet->GetNumTerminals();t++)
		{
			const Novorado::Netlist::Terminal* T=aNet->terminal(t);
			std::shared_ptr<Cell> klfm_cell;
			if(T->isExternalPin()) klfm_cell=&pins[T->externalPin()->GetId()];
				else klfm_cell=&instances[T->instance()->GetId()];
			klfm_cell->m_Pins.emplace_back();
			Pin& pin=klfm_cell->m_Pins.back();
			pin.SetNet(&net),pin.SetCell(klfm_cell);
			net.AddPin(&pin);
		}
	}
}

std::vector<Cell*> KLFM::C(const npvec& v)
{
	std::vector<Cell*> rv(v.size());
	long idx=0;
	for(auto* node:v)
	{
		if(node->isInstance())
		{
			rv[idx++]=&instances[node->GetId()];
		}
		else if(node->isExternalPin())
		{
			rv[idx++]=&pins[node->GetId()];
		}
	}
	return rv;
}

void Partition::preset(const std::vector<Cell*>& fix,const std::vector<Cell*>& ini)
{
	for(auto* pcell:fix) pcell->SetFixed(true), pcell->SetSquare(0), pcell->SetPartition(this);
	for(auto* pcell:ini) pcell->SetFixed(false), pcell->SetSquare(1), pcell->SetPartition(this);
	std::vector<Cell*> all(fix);
	all.reserve(fix.size()+ini.size());
	std::copy(ini.begin(),ini.end(),std::back_inserter(all));
	m_Locker.init(all);
	for(auto* c:all) c->MoveToLocker(true);
}

void CellList::init(const std::vector<Cell*>& v)
{
	for(auto* cell:v) insertCell(end(),*cell);
}

Pin::Pin()
{
	SetId(-1);
	//ctor
	SetCell(NULL);
	SetNet(NULL);
}

Pin::~Pin()
{
	//dtor
}

Pin::Pin(const Pin& other):Novorado::Object(other)
{
	//copy ctor
	// SetCell(other.GetCell());
	// SetNet(other.GetNet());
}

Pin& Pin::operator=(const Pin& rhs)
{
	Novorado::Object::operator=(rhs);
	//SetCell(rhs.GetCell());
	//SetNet(rhs.GetNet());
	return *this;
}

#ifdef CHECK_LOGIC
void Pin::SetName(const std::string& n)
{
	Novorado::Object::SetName(n);
	for(auto& p:m_Cell->m_Pins)
	{
		if(p.m_Net.isSet() && p.GetName()==n)
		{
			throw std::logic_error(std::logic_error(
				std::string("Attempting to connect ")+m_Cell->GetName()+":"+
					GetName()+" which is already connected to "+p.GetNet()->GetName()
				));
		}
	}
}
#endif // CHECK_LOGIC

void Pin::SetNet(Net* val)
{
	m_Net=val;
}

Cell::Cell()
{
	//ctor
	SetId(-1);
}

Cell::~Cell()
{
	//dtor
}

Cell::Cell(Cell& rhs):Novorado::Object(rhs)
{
	m_Pins=rhs.m_Pins;
	SetGain(rhs.GetGain());
	SetPartition(rhs.GetPartition());
	SetSquare(rhs.GetSquare());
	MoveToLocker(rhs.IsInLocker());
	SetFixed(rhs.IsFixed());
}

Cell::Cell(const Cell& rhs):Novorado::Object(rhs)
{
	m_Pins=rhs.m_Pins;
	SetGain(rhs.GetGain());
	//SetPartition(rhs.GetPartition());
	SetSquare(rhs.GetSquare());
	MoveToLocker(rhs.IsInLocker());
	SetFixed(rhs.IsFixed());
}

Cell& Cell::operator=(Cell& rhs)
{
	Novorado::Object::operator=(rhs);
	m_Pins=rhs.m_Pins;
	SetGain(rhs.GetGain());
	SetPartition(rhs.GetPartition());
	SetSquare(rhs.GetSquare());
	MoveToLocker(rhs.IsInLocker());
	SetFixed(rhs.IsFixed());

	return *this;
}

void Cell::MoveToLocker(bool f)
{
	#ifdef	CHECK_LOGIC
	if(!f && IsFixed())
	{
		throw std::logic_error(std::string("Fixed cell ")+GetName()+ " can't be moved out of locker");
	}
	#endif // CHECK_LOGIC

	flags.inLocker=f;
}

Partition* Cell::GetPartition() {
	return m_PartitionPtr;
}

void Cell::SetPartition(Partition* p)
{
	if(m_PartitionPtr.isSet()) m_PartitionPtr.reset();
	m_PartitionPtr=p;
}

void Cell::IncrementGain(Net::Weight w)
{
#ifdef  ALGORITHM_VERBOSE
	std::cout << "cell " << (IsInLocker()?"L":"B") << " " << GetName() << " Cell::IncrementGain(" << w << ") " << GetGain() << " => " << (GetGain()+w) << std::endl;
#endif
	m_Gain+=w;
	if(!IsInLocker()) GetPartition()->m_Bucket.IncrementGain(w);
		else GetPartition()->m_Locker.IncrementSumGain(w);
}

Net::Net()
{
	//ctor
	SetId(-1);
	SetWeight(1.0);
}

Net::~Net()
{
	//dtor
}

Net::Net(const Net& other)
{
	//copy ctor
	*this=other;
}

Net& Net::operator=(const Net& rhs)
{
	Novorado::Object::operator=(rhs);

	SetWeight(rhs.GetWeight());
	m_Pins=rhs.m_Pins;
	//assignment operator
	return *this;
}

void Net::AddPin(Pin* p)
{
	m_Pins.push_back(p);
}

unsigned int Net::Dim(Partition* p)
{
	unsigned int rv=0;
	for(Pin* pin:m_Pins){
		if(pin->GetCell()->GetPartition()==p) rv++;
		}
	return rv;
}

Bucket::Bucket()
{
	//ctor
	m_Square=0;
	m_SumGain=0;
}

Bucket::~Bucket()
{
	//dtor
}

Bucket::Bucket(const Bucket& other)
{
	#ifdef CHECK_LOGIC
	//copy ctor
	throw std::logic_error("Bucket must not be copied");
	#endif
}

Bucket& Bucket::operator=(const Bucket& rhs)
{
	#ifdef CHECK_LOGIC
	throw std::logic_error("Bucket must not be copied");
	#endif // CHECK_LOGIC

	if (this == &rhs) return *this; // handle self assignment
	//assignment operator
	return *this;
}

void Bucket::FillByGain(CellList& cl)
{
	#ifdef CHECK_LOGIC
	if(!empty()) throw std::logic_error("Unable to start moving cells into a non-empty bucket");
	#endif // CHECK_LOGIC

	m_Square += cl.GetSquare();
	m_SumGain=0;
	for(auto cellIt=cl.begin();cellIt!=cl.end();cellIt++)
	{
		// We're updating bucket gain, thus fixed cells are not counted
		// as fixed cells stay in locker
		if(cellIt->IsFixed()) continue;
		Net::Weight g=cellIt->GetGain();
		m_SumGain+=(g);
		cellIt->MoveToLocker(false); // remove from locker. Failure to do so will result
			// in wrong updated gains later
		CellList& bl=(*this)[g];
		cl.TransferTo(cellIt--,bl);
	}
	cl.InvalidateGain();
}

#ifdef  ALGORITHM_VERBOSE
void Bucket::dbg(long id)
{
	std::cout << "BUCKET #" << id << " SQ=" << m_Square  << " GAIN=" << GetGain() << std::endl;
	for(std::map<Net::Weight,CellList>::iterator i=begin();i!=end();i++){
		Net::Weight g=i->first;
		CellList& bl=i->second;
		std::cout << " Gain " << g << " - " << std::flush;
		for(const Cell& j:bl){
			std::cout << j.GetName() << " ";
			}
		std::cout << std::endl;
		}
}
#endif

void Bucket::IncrementGain(Net::Weight g)
{
#ifdef  ALGORITHM_VERBOSE
	std::cout << "Bucket::IncrementGain g=" << g << std::endl;
#endif
	m_SumGain+=g;
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

Net::Weight CellList::GetSumGain()
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

Cell::Square CellList::GetSquare()
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

void CellList::SetCellGain(Net::Weight g)
{
 	//std::for_each(begin(),end(),std::bind2nd(std::mem_fun_ref(&Cell::SetGain), g));
 	for(auto& cell:*this) cell.SetGain(g);
}

Net::Weight CellList::IncrementSumGain(Net::Weight g)
{
	#if 0 && defined(ALGORITHM_VERBOSE)
	std::cout << "CellList::IncrementSumGain(" << g << ") " << GetSumGain() << " => " << (GetSumGain()+g) << std::endl;
	#endif
	return m_SumGain=GetSumGain()+g;
}


CellMove::CellMove(Partition& _p0,Partition& _p1):p0(_p0),p1(_p1)
{
	//ctor
}

CellMove::~CellMove()
{
	//dtor
}


Iteration::Iteration(NetlistHypergraph* _graph):CellMove(_graph->p0,_graph->p1)
{
	m_Improvement=-1;

	//ctor
	graph=_graph;
}

Iteration::~Iteration()
{
	//dtor
}

//
// SUM OF CELL GAINS IN LEFT AND RIGHT LOCKER == NUMBER OF CUTTING NETS
// We want to minimize locker0.gain+locker1.gain
//
void Iteration::run()
{
	// Gains are incrementally updated
	p0.m_Bucket.FillByGain(p0.m_Locker);
	p1.m_Bucket.FillByGain(p1.m_Locker);

#ifdef  ALGORITHM_VERBOSE
	std::cout << "*** STARTING ITERATIONS ****" << std::endl;
	p0.m_Bucket.dbg(0);
		 std::cout << "LOCKER0 " << p0.m_Locker.dbg() << std::endl;
	p1.m_Bucket.dbg(1);
		std::cout << "LOCKER1 " << p1.m_Locker.dbg() << std::endl;
	std::cout << "P0sq=" << p0.GetSquare() << " P0gn=" << p0.GetGain() <<  " P1sq=" << p1.GetSquare() << " P1gn=" << p1.GetGain() << "\n" << std::endl;
#endif

	// Only save solution after setting an initial gain
	graph->bestSolution=Solution(p0,p1,*graph->m_AllCells);

	m_Improvement=0;
#ifdef  ALGORITHM_VERBOSE
	int cnt=0;
#endif
#ifdef  PRINT_PROGRESS
	long Ll=p0.m_Locker.size(),Lr=p1.m_Locker.size();
#endif //PRINT_PROGRESS

	while(!p0.m_Bucket.empty() || !p1.m_Bucket.empty()){

#ifdef  ALGORITHM_VERBOSE
		std::cout << "STEP #" << ++cnt << std::endl;
#endif

		if(p0.m_Bucket.empty())
		{
			// Left side is empty
			if(!p1.m_Bucket.empty()) moveLeft();
		}
		else
		{
			if(p1.m_Bucket.empty())
			{
				// Right side is empty
				if(!p0.m_Bucket.empty()) moveRight();
			}
			else
			{
				// Both left and right side are not empty
				if(p0.GetSquare()>(1.0+SQUARE_TOLERANCE)*p1.GetSquare())
				{
					// Due to square balance, we can only move right
					moveRight();

				}
				else if(p1.GetSquare()>(1.0+SQUARE_TOLERANCE)*p0.GetSquare())
				{
					moveLeft();
				}
				else
				{
					struct {
						Net::Weight left, right;
						} gain;

					gain.left=p0.m_Bucket.rbegin()->first;
					gain.right=p1.m_Bucket.rbegin()->first;

					// Move by gain
					if(gain.left>gain.right) moveRight(); else moveLeft();
				}
			}
		}




 #ifdef  ALGORITHM_VERBOSE
		p0.m_Bucket.dbg(0);
		std::cout << "LOCKER0 " << p0.m_Locker.dbg() << std::endl;

		p1.m_Bucket.dbg(1);
		std::cout << "LOCKER1 " << p1.m_Locker.dbg() << std::endl;

		std::cout << "P0sq=" << p0.GetSquare() << " P0gn=" << p0.GetGain() <<  " P1sq=" << p1.GetSquare() << " P1gn=" << p1.GetGain() << std::endl;
#endif
#ifdef  PRINT_PROGRESS
		std::cout << "\rLl=" << Ll << " Lr=" << Lr << " T=" << (Ll+Lr) << std::flush;
#endif//PRINT_PROGRESS
		if(Solution::SolutionImproved(graph->bestSolution,p0.GetSquare(),p1.GetSquare(),p0.GetGain()+p1.GetGain())) {

			m_Improvement+=graph->bestSolution.Cut();

			// Store best solution as current
			graph->bestSolution=Solution(p0,p1,*graph->m_AllCells);

			m_Improvement-=graph->bestSolution.Cut();
			}

#ifdef  ALGORITHM_VERBOSE
		std::cout << std::endl;
#endif
		}
}

void Iteration::moveCell(Partition* from,Partition* to)
{
	CellList& tgCL = from->m_Bucket.rbegin()->second;
	Net::Weight topGain=from->m_Bucket.rbegin()->first;

	Cell& cell=*tgCL.begin();

	#ifdef CHECK_LOGIC
	if(cell.GetPartition()!=from)
	{
		throw std::logic_error("Cell does not belong to a partition");
	}

	if(cell.IsFixed())
	{
		throw std::logic_error("algorithm integrity");
	}
	#endif // CHECK_LOGIC

	from->m_Bucket.SubtractSquare(cell.GetSquare());
	from->m_Bucket.IncrementGain(-cell.GetGain());

	cell.SetPartition(to);
	cell.MoveToLocker();

#ifdef  ALGORITHM_VERBOSE
	std::cout << "MOVE "<< (from==&p0?"RIGHT":"LEFT");
	std::cout << " Cell " << cell.GetName() << " Cell Gain=" << cell.GetGain() << " Bucket higher gain=" << topGain << std::endl;
#endif
	/*m_Improvement+=*/graph->UpdateGains(cell);

	// cell reference is not valid after moving the cell in this line
	tgCL.TransferTo(tgCL.begin(),to->m_Locker,false);

	if(tgCL.empty()) {
		// Remove empty gain lists
		from->m_Bucket.erase(topGain);
		}
}


Partition::Partition()
{
	//ctor
	m_Bucket.SetPartition(this);
}

Partition::~Partition()
{
	//dtor
}

Cell::Square Partition::GetSquare()
{
	return m_Bucket.GetSquare()+m_Locker.GetSquare();
}

Net::Weight Partition::GetGain()
{
	return m_Bucket.GetGain()+m_Locker.GetSumGain();
}


NetlistHypergraph::NetlistHypergraph():bestSolution(p0,p1)
{
	//ctop
	p0.SetId(0);
	p1.SetId(1);
}

NetlistHypergraph::~NetlistHypergraph()
{
	//dtor
	m_AllCells.DestructiveReset();
}


void NetlistHypergraph::FillBuckets()
{
	Net::Weight left=0,right=0;

	for(Net& net:nets) {
		// Set gain for all cells beloging to this partition, with other net cells in other partition
		left=right=0;

		#ifdef  ALGORITHM_VERBOSE
		std::cout << "======== NET " << net.GetName() << std::endl;
		#endif
		// compute gains
		for(Pin* p:net.m_Pins){
			Cell& cell = *p->GetCell();

			if(cell.GetPartition()==&p0) left+=net.GetWeight();
			else if(cell.GetPartition()==&p1) right+=net.GetWeight();
			#ifdef CHECK_LOGIC
			else throw std::logic_error("cell does not belong to ANY partition");
			#endif // CHECK_LOGIC
			}

		// set gains
		for(Pin* p:net.m_Pins){
			Cell& cell = *p->GetCell();

			if(cell.GetPartition()==&p0){
				// cell on the left
				cell.SetGain(cell.GetGain()+right-left+net.GetWeight());

				} else if(cell.GetPartition()==&p1){
					// cell on the right
					cell.SetGain(cell.GetGain()+left-right+net.GetWeight());
					}

			#ifdef  ALGORITHM_VERBOSE
			std::cout << "Cell " << cell.GetName() <<":" << p->GetName() << " gain " << cell.GetGain() << std::endl;
			#endif
			}
		}

	#ifdef  ALGORITHM_VERBOSE
	std::cout << "P0 " << p0.m_Locker.dbg() << "\nP1 " << p1.m_Locker.dbg() << std::endl;
	#endif
}

// This function is called after changing partition in the cell
/*
Table of gains N=24 Weight=2, moving left to right
* dMC - delta gain on the cell being moved
L	R	dLR	Gl	Gr	dMC
12	12	0	1W	1W	-
11	13	-2	3W	-1W	2W
10	14	-4	5W	-3W	2W
9	15	-6	7W	-5W	2W
8	16	-8	9W	-7W	2W
7	17	-10	11W	-9W	2W
6	18	-12	13W	-11W	2W
5	19	-14	15W	-13W	2W
4	20	-16	17W	-15W	2W
3	21	-18	19W	-17W	2W
2	22	-20	21W	-19W	2W
1	23	-22	23W	-21W	2W
0	24	-24	25W	-23W	2W
*/
Net::Weight NetlistHypergraph::UpdateGains(Cell& c)
{
	Net::Weight rv=0;

	Partition* newP=c.GetPartition(), *oldP=NULL;

	if(newP==&p0) oldP=&p1;
	else if(newP==&p1) oldP=&p0;
	#ifdef CHECK_LOGIC
	else throw std::logic_error("Wrong parition pointer");
	#endif // CHECK_LOGIC

	std::map<Cell*,Net::Weight> prevGain;

	c.SetGain(0);

	// Find all connected nets
	for(Pin& p:c.m_Pins){

		Net& net = *p.GetNet();

#ifdef  ALGORITHM_VERBOSE
		std::cout << "UPDATE GAIN NET " << net.GetName() << std::endl;
#endif
		long newPcnt=0,oldPcnt=0;

		// Adjust gain for all cells affected
		// It'll work for the cell moved if you do the math, same dG altough different formula
		for(Pin* p:net.m_Pins) {
			Cell& cell=*p->GetCell();

			if(cell==c) continue;

			Net::Weight dG=0;
			if(cell.GetPartition()==newP) { newPcnt++; dG=-2*net.GetWeight(); }
			else {
					if(cell.GetPartition()==oldP) { oldPcnt++; dG=2*net.GetWeight(); }
					#ifdef CHECK_LOGIC
					else { throw std::logic_error("Wrong parition pointer in cell"); }
					#endif // CHECK_LOGIC
			}

			// Store previous gain when we hit the cell for the first time
			if(!cell.IsInLocker() && prevGain.find(&cell)==prevGain.end())
			{
				prevGain[&cell]=cell.GetGain();
			}

			cell.IncrementGain(dG);
			rv+=dG;
			}

		c.IncrementGain((oldPcnt-newPcnt)*net.GetWeight());
		}

	// Move cells to new buckets accordingly to the updated gain
	for(std::pair<Cell*,Net::Weight> k:prevGain){

		Cell& cell=*k.first;
		Net::Weight& prevGain=k.second;

		#ifdef CHECK_LOGIC
		if(cell.IsInLocker())
		{
			throw std::logic_error("Cells in locker do not receive gain updates, needed for bucket sorting");
		}
		#endif // CHECK_LOGIC

		CellList
			& previousBucket=cell.GetPartition()->m_Bucket[prevGain],
			& newBucket=cell.GetPartition()->m_Bucket[cell.GetGain()];

		auto it=previousBucket.find(cell);

		#ifdef CHECK_LOGIC
		if(it==previousBucket.end())
		{
			throw std::logic_error(std::string("Cannot find cell ")+cell.GetName()+" in a bucket list");
		}
		#endif // CHECK_LOGIC

		previousBucket.TransferTo(it,newBucket);

		// If no more cells for that gain in bucket, remove the entire bucket
		if(previousBucket.empty())  cell.GetPartition()->m_Bucket.erase(prevGain);
		}

	return rv;
}

NetlistHypergraph::CutStat NetlistHypergraph::GetStats(std::ofstream& o,bool fWrite)
{
	CutStat rv;
	std::stringstream msg;
	msg << "Cutting nets ";
	if(fWrite) o << "Nets cutting: " << std::endl;
	for(std::vector<Net>::iterator i=nets.begin();i!=nets.end();i++)
	{
		Net& net=*i;
		Partition* p=NULL;
		for(std::vector<Pin*>::iterator j=net.m_Pins.begin();j!=net.m_Pins.end();j++){
			if(!p) {
					p=(*j)->GetCell()->GetPartition();
			}
			else if(p!=(*j)->GetCell()->GetPartition()){
				if(fWrite) o << net.GetName() << std::endl;
				msg << net.GetName() << " ";
				rv.m_NetCut++;
				rv.m_totWeigth+=net.GetWeight();
				break;
				}
			}
	}
	msg << "=" << rv.m_NetCut << ", total weight is " << rv.m_totWeigth;
	std::cout << msg.str()  << std::endl;
	if(fWrite) o << msg.str()  << std::endl;
	return rv;
}

Solution::Solution(Partition& _p1,Partition& _p2):p1(&_p1),p2(&_p2)
{
	//ctor
	g1=g2=0;
	s1=s2=0;
	m_Recs.resize(0);
}

Solution::Solution(Partition& _p1,Partition& _p2,std::vector<Cell>& cells):p1(&_p1),p2(&_p2)
{
	//ctor
	g1=g2=0;
	s1=s2=0;
	m_Recs.resize(cells.size());
	for(int i=cells.size()-1;i>=0;i--) AddCell(&cells[i]);
}

Solution::~Solution()
{
	//dtor
}

Solution& Solution::operator=(const Solution& s)
{
	m_Recs=s.m_Recs;
	g1=s.g1;s1=s.s1;p1=s.p1;
	g2=s.g2;s2=s.s2;p2=s.p2;
	return *this;
}

void Solution::AddCell(Cell* c)
{
	m_Recs[c->GetId()].gain=c->GetGain();
	m_Recs[c->GetId()].p=c->GetPartition();
	m_Recs[c->GetId()].cell=c;
	if(c->GetPartition()==p1) {
		g1+=c->GetGain();
		s1+=c->GetSquare();
		} else { if(c->GetPartition()==p2) {
			g2+=c->GetGain();
			s2+=c->GetSquare();
			}
			#ifdef CHECK_LOGIC
			else throw std::logic_error("Wrong partition");
			#endif // CHECK_LOGIC
		}
}

bool Solution::SolutionImproved(
	Solution& s, // existing solution

	Cell::Square s2_0, // new solution
	Cell::Square s2_1,
	Net::Weight g2) // minimizing gain
{
	// Corner case, empty bin on either side
	if(s2_0==0 || s2_1==0) return false;

	float newRatio=std::max(s2_0,s2_1)/std::min(s2_0,s2_1);

	struct
	{
		bool initial, ratio, cut;
		operator bool() const { return (ratio && cut) || initial; }
	} conds;

	conds.initial = s.IsInitial();
	conds.ratio = newRatio<=s.Ratio()*(1.0+SQUARE_TOLERANCE);
	conds.cut = g2 < s.Cut();

#ifdef  ALGORITHM_VERBOSE
	std::cout
		<< " new ratio " << newRatio << " " << s.Ratio() << " "
		<< "Solution " << (conds?"":"not ")<< "improved: Gain " << g2 << ", previous gain " << s.Cut() << std::endl;
#endif
	return conds;
}

void Solution::WriteLockers(CellList& l0, CellList& l1)
{
	for(int i=m_Recs.size()-1;i>=0;i--) {
		Cell& cell=*m_Recs[i].cell;
		cell.SetPartition(m_Recs[i].p);
		cell.SetGain(m_Recs[i].gain);
		}

	for(CellList::Iterator j=l0.begin();j!=l0.end();j++)
		if(j->GetPartition()!=p1)
			l0.TransferTo(j--,l1,false);

	for(CellList::Iterator j=l1.begin();j!=l1.end();j++)
		if(j->GetPartition()!=p2)
			l1.TransferTo(j--,l0,false);
}

RandomDistribution::RandomDistribution(Partition& _p0,Partition& _p1):CellMove(_p0,_p1)
{
   //ctor
	run();
}

RandomDistribution::~RandomDistribution()
{
	//dtor
}

void RandomDistribution::run()
{
	const long HRM = RAND_MAX / 2;
	// Move all non-fixed elements to first partition p0
	for(auto i=p1.m_Locker.begin();i!=p1.m_Locker.end();i++)
	{
        if(!i->IsFixed()) p0.m_Locker.splice(p0.m_Locker.end(),p1.m_Locker,i--);
	}

	for(auto& cell:p0.m_Locker) cell.SetPartition(&p0);

	while(p0.m_Locker.GetSquare()>p1.m_Locker.GetSquare())
	{
		for(auto i=p0.m_Locker.begin();
			i!=p0.m_Locker.end() && p0.m_Locker.GetSquare()>p1.m_Locker.GetSquare();
			i++)
		{
			if(!i->IsFixed() && std::rand()>HRM)
			{
				i->SetPartition(&p1);
				p0.m_Locker.TransferTo(i--,p1.m_Locker);
		   }
		}
	}
}


#ifdef KLFM_TEST
class Test6 : public NetlistHypergraph
{
    class Files {
        std::string m_input, m_left, m_right, m_stat;
        std::ofstream m_l,m_r,m_s;
        public:
        Files(std::string i){
            m_input=i;
            m_left=i+"_left";
            m_right=i+"_right";
            m_stat=i+="_stat";
            }

        std::ofstream& GetLeft()
        {
            if(!m_l.is_open()) m_l.open(m_left.c_str());
            return m_l;
        }

        std::ofstream& GetRight()
        {
            if(!m_r.is_open()) m_r.open(m_right.c_str());
            return m_r;
        }

        std::ofstream& GetStat()
        {
            if(!m_s.is_open()) m_s.open(m_stat.c_str());
            return m_s;
        }
        } m_Files;

    std::map<std::string,Cell*> m_name2cell;
    long current_ln;

    Pin& getNextPin(Cell& c1, const std::string& name=""){
        c1.m_Pins.emplace_back();
        Pin& newPin=c1.m_Pins.back();
        newPin.SetId(c1.m_Pins.size());
        newPin.SetCell(&c1);
        newPin.SetName(name); // that will trigger consistency check
        return newPin;
        }

 #ifdef  ALGORITHM_VERBOSE
   void printNetlist(){
        for(Cell& c:*m_AllCells) {
            std::cout << "Cell '"<< c.GetName() << "' #" << c.GetId() << " connects to " << std::flush;
            for(std::list<Pin>::iterator j=c.m_Pins.begin();j!=c.m_Pins.end();j++){
                if(!j->GetNet()) throw std::logic_error("No net is initialized");
                Net& net = *j->GetNet();
                std::cout << " pin " << j->GetName() << "-> net " << net.GetName() << " ";
                for(std::vector<Pin*>::iterator ll=net.m_Pins.begin();ll!=net.m_Pins.end();ll++) {
                    if(!(*ll)->GetCell()) std::logic_error("No cell is initialized");
                    if(c.GetId()!=(*ll)->GetCell()->GetId())
                        std::cout << (*ll)->GetCell()->GetName() << ":" << (*ll)->GetName() << " ";
                    }
                }
            std::cout << std::endl;
            }
        }
#endif
    void ReadGraphFromFile(std::string fn)
    {
        std::cout << "Reading from '" << fn << "' .. " << std::endl;
        std::ifstream f(fn.c_str());
        current_ln=0;
        unsigned int cell_idx=0;
        std::vector< std::vector<std::string> > tmpNets;
        while(!f.eof())
        {
            current_ln++;
            std::string l;
            std::vector<std::string> words;
            std::getline(f,l);
            if(f.eof() && l=="") break;
            std::stringstream ss(l);
            while (ss>>l) words.push_back(l);
            #ifdef CHECK_LOGIC
            if(words.size()<2) {
                std::cout << "Error at " << fn << ":" << current_ln << " '" << l << "'" << std::endl;
                throw std::logic_error("Parsing error");
                }
            #endif
            if(words.size()==2)
            {

				if(words.front()=="fixedleft")
				{
					Cell* c=(m_name2cell[words.back()]);
					c->SetFixed();
					c->SetPartition(&p0);
					continue;
				} else if(words.front()=="fixedright")
					{
						Cell* c=(m_name2cell[words.back()]);
						c->SetFixed();
						c->SetPartition(&p1);
						continue;
					}

            	MakeCell(words[0],words[1],cell_idx);
            } else tmpNets.push_back(words);
        }
        f.close();

        unsigned int netIdx=0;
        nets.resize(tmpNets.size());
        for(std::vector< std::vector<std::string> >::iterator k=tmpNets.begin();k!=tmpNets.end();k++,netIdx++)
            MakeNet(nets[netIdx],*k,netIdx);

        std::cout << " done" << std::endl;
    }

    void MakeCell(const std::string & cellName, const std::string& ssq, unsigned int& idx )
    {
        #ifdef CHECK_LOGIC
        if(m_name2cell.find(cellName)!=m_name2cell.end()) {
            std::stringstream msg;
            msg << "Duplicated cell '" << cellName << "' at line " << current_ln;
            throw std::logic_error(msg.str());
            }
        #endif
        Cell::Square sq=0;
        std::stringstream s(ssq);
        s >> sq;
        Cell tmpCell;
        tmpCell.SetId(idx++);
        tmpCell.SetName(cellName);
        tmpCell.SetSquare(sq);
        m_AllCells->push_back(tmpCell);
        instances.init(&(*m_AllCells)[0],m_AllCells->size());

        m_AllCells->back().SetPartition(&p0);
        #ifdef CHECK_LOGIC
        if(!sq) {
            std::stringstream msg;
            msg << "Cell '" << cellName << "' has illegal square '" << ssq << "'";
            throw std::logic_error(msg.str());
            }
        #endif
        m_name2cell[cellName]=&m_AllCells->back();
    }

    void MakeNet(Net& net,const std::vector<std::string>& words, int idx)
    {
        net.SetId(idx);
        int cnt=0;
        Net::Weight w;
        std::stringstream s;
        std::string str1;
        for(std::string str:words)
        {
            switch(cnt++)
            {
                case 0:
                        net.SetName(str);
#ifdef  ALGORITHM_VERBOSE
                        std::cout << "Creating net '" << str << "' weigth="  << std::flush;
#endif
                        break;
                case 1: s.str(str);
                        s >> w;
 #ifdef  ALGORITHM_VERBOSE
                       std::cout << w << " " << std::flush;
#endif
                        net.SetWeight(w);
                      break;
                default:
                		if(!str1.length()) str1=str;
                        	else MakePin(str1,str,net),str1.clear();
                        break;
            }
        }
#ifdef  ALGORITHM_VERBOSE
        std::cout << std::endl;
#endif
    }

    void MakePin(const std::string& cellName,const std::string& pinName, Net& net)
    {
        std::map<std::string,Cell*>::iterator i=m_name2cell.find(cellName);
        #ifdef CHECK_LOGIC
        if(i==m_name2cell.end()) {
            throw std::logic_error("Incorrect cell name");
            }
        #endif
        Cell& cell=*(i->second);
        Pin& pin=getNextPin(cell,pinName);
        pin.SetNet(&net);
        net.AddPin(&pin);
#ifdef  ALGORITHM_VERBOSE
        std::cout << cell.GetName() << ":" << pin.GetName() << " ";
#endif
    }

public:
    Test6(std::string fn):m_Files(fn){
        std::srand(2015);

        // Should be enough for testing to prevent reallocation and memory corruption
        m_AllCells=new std::vector<Cell>;
        m_AllCells->reserve(1000);
        nets.reserve(1000);

        ReadGraphFromFile(fn);

		#if 0 && defined(ALGORITHM_VERBOSE)
        printNetlist();
		#endif
		// Buckets get fill from the lockers
		for(Cell& c:*m_AllCells)
		{
			if(c.GetPartition()==&p1) p1.m_Locker.insertCell(p1.m_Locker.end(),c);
				else p0.m_Locker.insertCell(p0.m_Locker.end(),c);

			c.MoveToLocker();// Mark that cell is moved to the locker
		}

        RandomDistribution(p0,p1);

		// Need updated gains
        FillBuckets();

		#ifdef  ALGORITHM_VERBOSE
        std::cout << "LOCKER0 SQ=" << p0.m_Locker.GetSquare() << " " << p0.m_Locker.dbg() << std::endl;
        std::cout << "LOCKER1 SQ=" << p1.m_Locker.GetSquare() << " " << p1.m_Locker.dbg() << std::endl;
		#endif

        for(int iter_cnt=0;;iter_cnt++){

            Iteration step(this);

            step.run();

            #ifdef PRINT_PROGRESS
            std::cout << std::endl;
            #endif // PRINT_PROGRESS

            bestSolution.WriteLockers(p0.m_Locker,p1.m_Locker);

            if(step.GetImprovement()<=0) break;

            std::cout << "ITERATION " << iter_cnt << ", IMPROVEMENT " << step.GetImprovement() << std::endl;
            }

        std::ofstream& l= m_Files.GetLeft(), & r = m_Files.GetRight();
        for(auto&& cell: p0.m_Locker) l << cell.GetName() << std::endl;
        for(auto&& cell: p1.m_Locker) r << cell.GetName() << std::endl;
		#ifdef  ALGORITHM_VERBOSE
		std::cout << "P0 ";
        for(auto&& cell: p0.m_Locker) std::cout << cell.GetName() << " ";
        std::cout << std::endl << "P1 ";
        for(auto&& cell: p1.m_Locker) std::cout << cell.GetName() << " ";
        std::cout << std::endl;
		#endif // ALGORITHM_VERBOSE

        // write best solution
        GetStats(m_Files.GetStat(),true);

		// Exit entire app after the unit test
        exit(0);
        }
};

Test6 t_6("/home/dmi/soft/shelby/test/6.net");
#endif
