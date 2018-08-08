#ifndef _PIN_H
#define _PIN_H

#include "bridge.h"

namespace Novorado
{
	namespace Partition
	{
		class Cell;
		class Net;

		class Pin : public Bridge::Id
		{
		public:

			Pin() = default;
			virtual ~Pin() = default;

			Pin(const Pin& other) noexcept;
			Pin& operator=(const Pin& other) noexcept;

            Cell* GetCell() noexcept;

            void SetCell(Cell* val) noexcept;

			auto GetNet() noexcept {return m_Net;}

			void SetNet(Net* val) noexcept;

			#ifdef CHECK_LOGIC
			// Overloading object method for consistency checking
			virtual void SetName(const std::string&);
			#endif // CHECK_LOGIC

		protected:
		private:
			Cell* m_Cell;
			Net* m_Net;
		};
	}
}
#endif//_PIN_H
