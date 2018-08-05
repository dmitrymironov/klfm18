
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

//#endif//_REFACTORING_
