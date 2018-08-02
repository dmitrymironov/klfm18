#ifndef _BRACKET_H
#define _BRACKET_H

#include <stdexcept>
#include "bridge.h"

namespace Novorado
{
	template <class V> class Bracket
	{
		V* head;
		size_t m_size=0;
		public:

			using Index = long;

			size_t size() const
			{
				return m_size;
			}

			void check(Index idx)
			{
				if(idx<0 || idx>=m_size) throw
					std::out_of_range("Wrong bracket addressing");
			}

			class Iterator
			{
				Bracket<V>* p_b;
				Index idx;
				public:
					Iterator(Bracket<V>* b, Index _idx):
						p_b(b),idx(_idx)
						{
						}

					bool operator!=(Iterator& i) { return idx!=i.idx; }
					Iterator& operator++() { p_b->check(idx++); return *this; }
					V& operator*() { return (*p_b)[idx]; }
				};

			Bracket(){}

			Bracket(V* v,size_t sz):head(v),m_size(sz) {}

			void init(V* v,size_t sz){ head.reset();head=(v);m_size=(sz); }

			Bracket(Bracket& ano):head(ano.head),m_size(ano.m_size) { }

			Bracket(const Bracket& ano)
			{
				throw std::logic_error("Calling dummy");
			}

			V& operator[](Index idx)
			{
				check(idx);
				return head.get()[idx];
			}

			Iterator begin() { return Iterator(this,0); }
			Iterator end() { return Iterator(this,m_size); }
	};
}

#endif//_BRACKET_H
