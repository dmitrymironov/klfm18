#ifndef _BUCKET_H
#define _BUCKET_H

#include "celllist.h"
#include "net.h"
#include <map>

namespace Novorado
{
	namespace Partition
	{
		class Bucket : public std::map<Weight,CellList>
		{
			public:
				virtual ~Bucket();
				Bucket(const Bucket& other);
				Bucket& operator=(const Bucket& other);
				void SetPartition(std::shared_ptr<Partition> p) 
				{ 
					m_Partition=p; 
				}
				void FillByGain(CellList&);
				void dbg(long);
				Square GetSquare() const { return m_Square; }
				void SubtractSquare(Square s) { m_Square-=s; }
				Weight GetGain() const { return m_SumGain;}
				void IncrementGain(Weight g);
			protected:
			private:
				Square m_Square;
				Weight m_SumGain;
				std::shared_ptr<Partition> m_Partition;
				Bucket();
				friend class Partition;
		};
	}
}
#endif//_BUCKET_H
