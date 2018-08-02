#ifndef _NET_H
#define _NET_H

#include "bridge.h"
#include <vector>

namespace Novorado
{
	namespace Partition
	{
		class Partition;
		class Pin;

		using Weight = long;

		class Net : public Bridge::Id
		{
			public:
				Net();
				virtual ~Net();
				Net(const Net& other);
				Net& operator=(const Net& other);
				void SetWeight(Weight w)
				{
					m_Weight = w;
				}
				Weight GetWeight() const
				{
					return m_Weight;
				}
				void AddPin(Pin*);
				auto Dim() const
				{
					return m_Pins.size();
				}
				auto Dim(Partition*);
				std::vector<Pin*> m_Pins;

			protected:
			private:
				Weight m_Weight;
		};
	}
}
#endif//_NET_H
