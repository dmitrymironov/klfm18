#include "pin.h"

using namespace Novorado::Partition;

Pin::Pin(const Pin& other) noexcept:Bridge::Id(other)
{
	//copy ctor
	// SetCell(other.GetCell());
	// SetNet(other.GetNet());
}

Pin& Pin::operator=(const Pin& rhs) noexcept
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

void Pin::SetNet(std::shared_ptr<Net> val) noexcept
{
	m_Net=val;
}
