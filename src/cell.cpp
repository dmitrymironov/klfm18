#include "cell.h"
#include "partition.h"

using namespace Novorado::Partition;

Cell::Cell()
{
	//ctor
	SetId(-1);
}

Cell::~Cell()
{
	//dtor
}

Cell::Cell(Cell& rhs):Bridge::Id(rhs)
{
	m_Pins=rhs.m_Pins;
	SetGain(rhs.GetGain());
	SetPartition(rhs.GetPartition());
	SetSquare(rhs.GetSquare());
	MoveToLocker(rhs.IsInLocker());
	SetFixed(rhs.IsFixed());
}

Cell::Cell(const Cell& rhs):Bridge::Id(rhs)
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
	Bridge::Id::operator=(rhs);
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

std::shared_ptr<Partition> Cell::GetPartition() {
	return m_PartitionPtr;
}

void Cell::SetPartition(std::shared_ptr<Partition> p)
{
	m_PartitionPtr=p;
}

void Cell::IncrementGain(Weight w)
{
#ifdef  ALGORITHM_VERBOSE
	std::cout << "cell " << (IsInLocker()?"L":"B") << " " << GetName() << " Cell::IncrementGain(" << w << ") " << GetGain() << " => " << (GetGain()+w) << std::endl;
#endif
	m_Gain+=w;
	if(!IsInLocker()) GetPartition()->m_Bucket.IncrementGain(w);
		else GetPartition()->m_Locker.IncrementSumGain(w);
}

