#include "klfm18.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "pin.h"
#include "testbuilder.h"
#include <set>
#include <algorithm>
#include <gtest/gtest.h>

using namespace Novorado::Partition;

struct GraphWriter
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

	GraphWriter(NetlistHypergraph& hg, const std::string& i):m_Files{i}
	{
        std::ofstream& l= m_Files.GetLeft(), & r = m_Files.GetRight();
        for(auto& cell: hg.p0.m_Locker) l << cell.GetName() << std::endl;
        for(auto& cell: hg.p1.m_Locker) r << cell.GetName() << std::endl;
		#ifdef  ALGORITHM_VERBOSE
		std::cout << "P0 ";
        for(auto&& cell: hg.p0.m_Locker) std::cout << cell.GetName() << " ";
        std::cout << std::endl << "P1 ";
        for(auto&& cell: hg.p1.m_Locker) std::cout << cell.GetName() << " ";
        std::cout << std::endl;
		#endif // ALGORITHM_VERBOSE

        // write best solution
        hg.GetStats(m_Files.GetStat(),true);
	}
};

struct GraphCompare
{
	explicit GraphCompare(NetlistHypergraph& hg, const std::string& i)
	{
		fGood =
				CompareSet(hg.p0.m_Locker,std::ifstream(i+"_left"))
								&&
				CompareSet(hg.p1.m_Locker,std::ifstream(i+"_right"));
	}

	constexpr operator bool() const noexcept { return fGood; }

	private:

		bool CompareSet(CellList& l, std::ifstream&& f)
		{
			std::set<std::string> allCells;
			for(auto& cell: l)
			{
				allCells.insert(cell.GetName());
			}

			while(f.good())
			{
				std::string nextCell;
				f >> nextCell;
				if(!nextCell.empty() && allCells.find(nextCell)==allCells.end())
				{
					std::cerr << nextCell << " is in wrong partition"
							  << std::endl;
					return false;
				}
				if(f.eof()) break;
			}
			return true;
		}

		bool fGood{false};
};

bool graph_test(std::string&& name)
{

	std::srand(2018);

	// Load hypergraph
	std::string path="test/graph"+name+"/";
	auto Graph = std::move(TestBuilder(path+name+".net").H);

	Graph->Partition();

	// Write out resulted graph
	GraphWriter(*Graph,path+name);

	return GraphCompare(*Graph,path+"/GOLDEN/"+name);
}

TEST(GRAPH6,Ttt)
{
	ASSERT_TRUE(graph_test("6"));
}

int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
