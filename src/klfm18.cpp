
#ifdef _REFACTORING_

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

void KLFM::CreateGraph(const Novorado::Netlist::NetList* netlist,const part& fixed, part& initial)
{
	// Allocate data, init arrays
	m_AllCells=new std::vector<Cell>(netlist->GetNumInstances()+netlist->GetNumPins());
	instances.init(&(*m_AllCells)[0],netlist->GetNumInstances());
	pins.init(&(*m_AllCells)[netlist->GetNumInstances()],netlist->GetNumPins());
	nets.resize(netlist->GetNumNets());

	// Set id's and pointers
	Bridge::Id idx=0;
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


CellMove::CellMove(Partition& _p0,Partition& _p1):p0(_p0),p1(_p1)
{
	//ctor
}

CellMove::~CellMove()
{
	//dtor
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

#endif//_REFACTORING_
