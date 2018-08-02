#ifndef _NET_H
#define _NET_H

#include "bridge.h"
#include <vector>

namespace Novorado
{
	namespace Partition
	{
		class Partition;
		
		using Weight = long;

		class Net : public IdBridge
		{
			public:
				Net();
				virtual ~Net();
				Net(const Net&& other);
				Net& operator=(const Net&& other);
				void SetWeight(Weight w) { m_Weight = w; }
				Weight GetWeight() const { return m_Weight; }
				void AddPin(std::shared_ptr<Pin>);
				auto Dim() const { return m_Pins.size(); }
				auto Dim(std::shared_ptr<Partition>);
				std::vector<Pin*> m_Pins;

			protected:
			private:
				Weight m_Weight;
		};
	}
}
#endif//_NET_H
