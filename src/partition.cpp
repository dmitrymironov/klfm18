#include "partition.h"

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

Partition::Partition()
{
	//ctor
	m_Bucket.SetPartition(this);
}

Partition::~Partition()
{
	//dtor
}

Square Partition::GetSquare()
{
	return m_Bucket.GetSquare()+m_Locker.GetSquare();
}

Weight Partition::GetGain()
{
	return m_Bucket.GetGain()+m_Locker.GetSumGain();
}
