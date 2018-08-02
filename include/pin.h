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
		
			constexpr Pin() noexcept = default;
			virtual ~Pin() noexcept = default;

			constexpr Pin(const Pin&& other) noexcept;
			constexpr Pin& operator=(const Pin&& other) noexcept;

			auto GetCell() noexcept 
			{
				return m_Cell;
			}
			
			void SetCell(std::shared_ptr<Cell> val) noexcept 
			{
				m_Cell = val;
			}

			auto GetNet() noexcept {return m_Net;}
			
			constexpr void SetNet(std::shared_ptr<Net> val) noexcept;

			#ifdef CHECK_LOGIC
			// Overloading object method for consistency checking
			virtual void SetName(const std::string&);
			#endif // CHECK_LOGIC

		protected:
		private:
			std::shared_ptr<Cell> m_Cell;
			std::shared_ptr<Net> m_Net;
		};
	}
}
#endif//_PIN_H
