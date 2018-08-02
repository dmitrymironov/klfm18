#include "solution.h"
#include "klfm18.h"

using namespace Novorado::Partition;

Solution::Solution(Partition& _p1,Partition& _p2):p1(&_p1),p2(&_p2)
{
	//ctor
	g1=g2=0;
	s1=s2=0;
	m_Recs.resize(0);
}

Solution::Solution(Partition& _p1,Partition& _p2,std::vector<Cell>& cells):
	p1(&_p1),
	p2(&_p2)
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

	Square s2_0, // new solution
	Square s2_1,
	Weight g2) // minimizing gain
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

