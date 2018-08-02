#include "pin.h"

using namespace Novorado::Partition;

Pin::Pin()
{
	SetId(-1);
	//ctor
	SetCell(NULL);
	SetNet(NULL);
}

Pin::~Pin()
{
	//dtor
}

Pin::Pin(const Pin& other):Bridge::Id(other)
{
	//copy ctor
	// SetCell(other.GetCell());
	// SetNet(other.GetNet());
}

Pin& Pin::operator=(const Pin& rhs)
{
	Bridge::Id::operator=(rhs);
	//SetCell(rhs.GetCell());
	//SetNet(rhs.GetNet());
	return *this;
}

#ifdef CHECK_LOGIC
void Pin::SetName(const std::string& n)
{
	Bridge::Id::SetName(n);
	for(auto& p:m_Cell->m_Pins)
	{
		if(p.m_Net.isSet() && p.GetName()==n)
		{
			throw std::logic_error(std::logic_error(
				std::string("Attempting to connect ")+m_Cell->GetName()+":"+
					GetName()+" which is already connected to "+p.GetNet()->GetName()
				));
		}
	}
}
#endif // CHECK_LOGIC

void Pin::SetNet(Net* val)
{
	m_Net=val;
}
