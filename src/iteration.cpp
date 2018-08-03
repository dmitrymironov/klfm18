#include "iteration.h"
#include "klfm18.h"

using namespace Novorado::Partition;

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
						Weight left, right;
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
	Weight topGain=from->m_Bucket.rbegin()->first;

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

