#include "net.h"

using namespace Novorado::Partition;

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
	Bridge::Id::operator=(rhs);

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

