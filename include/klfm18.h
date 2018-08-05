#ifndef _KLFM18_H
#define _KLFM18_H

#include "bin.h"
#include "hypergraph.h"

namespace Novorado
{
	namespace Partition
	{
		// Partitioner will not proceed for small bins
		constexpr auto MIN_BIN_SIZE = 2;
		// Square treshhold 0.1=10%
		constexpr auto SQUARE_TOLERANCE = 0.1;

		//////////////////////////////////////////////////////////////////////////////////////////
		//
		// Algorihtm interface
		//
		//////////////////////////////////////////////////////////////////////////////////////////

		class KLFM : public NetlistHypergraph
		{
			public:
				// Partition result is returned in last reference, it is also used as initial solution
				void Partition();
		};
	};
};
#endif//_KLFM18_H
