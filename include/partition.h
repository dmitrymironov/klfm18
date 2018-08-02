#ifndef _PARTITION_H
#define _PARTITION_H

#include "bridge.h"
#include "celllist.h"
#include "bucket.h"

namespace Novorado
{
	namespace Partition
	{
		class Partition : public Bridge::Id
		{
			public:
				Partition();
				virtual ~Partition();

				CellList m_Locker;
				Bucket m_Bucket;

				Square GetSquare();
				Weight GetGain();

				void preset(const std::vector<Cell*>& fix,const std::vector<Cell*>& ini);

			protected:
			private:
				friend class NetlistHypergraph;
		};
	}
}
#endif//_PARTITION_H
