
//#ifdef _REFACTORING_

//#define ALGORITHM_VERBOSE

// Uncomment following line so exectuble becomes unit test for partitioning algorithm
//#define KLFM_TEST 1

#ifdef KLFM_TEST
#define CHECK_LOGIC 1
#define PRINT_PROGRESS 1
#define ALGORITHM_VERBOSE 1
#endif // KLFM_TEST

#include "klfm18.h"
#include "iteration.h"
#include <sstream>

using namespace Novorado::Partition;

void KLFM::Partition()
{
	InitializeLockers();

	RandomDistribution(p0,p1);

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
}

#ifdef KLFM_TEST
class Test6 : public NetlistHypergraph
{

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
};
#endif

//#endif//_REFACTORING_
