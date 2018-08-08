#ifndef TESTBUILDER_H
#define TESTBUILDER_H

#include <memory>
#include <klfm18.h>

namespace Novorado {

	namespace Partition {

		struct TestBuilder
		{
				std::shared_ptr<KLFM> H;

				TestBuilder(const std::string &fn);

				Pin &getNextPin(Cell &c1, const std::string &name="");

				void MakeCell(const std::string &cellName,
							  const std::string &ssq,
							  unsigned int &idx );

				void MakeNet(Net &net,
							 const std::vector<std::string> &words,
							 int idx);

				void MakePin(const std::string &cellName,
							 const std::string &pinName,
							 Net &net);

				void ReadGraphFromFile(const std::string &fn);

			private:
				std::map<std::string, Cell *> m_name2cell;
				long current_ln{-1};
		};
	}
}

#endif // TESTBUILDER_H
