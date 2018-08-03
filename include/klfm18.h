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

		#ifdef _REFACTORING_
		class KLFM : public NetlistHypergraph
		{
			std::vector<Cell*> C(const npvec&);
			void CreateGraph(const Novorado::Netlist::NetList*,const part&, part&);

			public:
				// Partition result is returned in last reference, it is also used as initial solution
				KLFM(
						const Novorado::Netlist::NetList*, // Source netlist
						const part& fixed, // Fixed part of bins (pre-allocated)
						part& initial // Initial distribution & resulting partition
						);
		};
		#endif//_REFACTORING_
	};
};
#endif//_KLFM18_H

