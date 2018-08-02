#include "bucket.h"

using namespace Novorado::Partition;

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
		Weight g=cellIt->GetGain();
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
	for(std::map<Weight,CellList>::iterator i=begin();i!=end();i++){
		Weight g=i->first;
		CellList& bl=i->second;
		std::cout << " Gain " << g << " - " << std::flush;
		for(const Cell& j:bl){
			std::cout << j.GetName() << " ";
			}
		std::cout << std::endl;
		}
}
#endif

void Bucket::IncrementGain(Weight g)
{
#ifdef  ALGORITHM_VERBOSE
	std::cout << "Bucket::IncrementGain g=" << g << std::endl;
#endif
	m_SumGain+=g;
}
