/*
 * BSPIndexSet.hpp
 *
 *  Created on: 2014-8-14
 *      Author: junfeng
 */

#ifndef BSPINDEXSET_HPP_
#define BSPINDEXSET_HPP_
#include <stdint.h>
#include "BSPException.hpp"
namespace BSP {
class LocalArray;
class IndexSet {
public:
	IndexSet(unsigned numberOfDimensions, uint64_t numberOfIndices);
	virtual ~IndexSet();
	class Iterator {
		friend class IndexSetPointSequence;
		friend class IndexSetPointTensor;
		friend class IndexSetRegionSequence;
		friend class IndexSetRegionTensor;
	public:
		Iterator(IndexSet *indexSet, unsigned numberOfComponents,
				unsigned numberOfMainComponents,
				uint64_t initialComponentBound[], bool isBegin = true);
		Iterator(const Iterator &iterator);
		virtual ~Iterator();
		Iterator &operator ++();
		Iterator operator ++(int);
		Iterator &operator =(const Iterator &iterator);
		bool operator ==(Iterator &iterator);
		bool operator !=(Iterator &iterator);
		uint64_t getIndexRank();
		uint64_t getRegionRank();
		uint64_t getComponentRank(unsigned iComponent);
		uint64_t getIndex(unsigned iDim);
		bool hasNext();
		void setComponentBound(unsigned iComponent, uint64_t newBound);
		uint64_t list(uint64_t amount, LocalArray &indexArray,
				uint64_t &amountLeft);
		uint64_t listRegion(uint64_t amount, LocalArray &indexArray,
				uint64_t &amountLeft, uint64_t &regionRank);
		uint64_t getRegionStart(uint64_t amount, LocalArray &indexArray,
				uint64_t &amountLeft);
		uint64_t nextRegion();
		void reset();
	protected:
		void increase();
		void decrease();
		void reset(bool release);
	private:
		IndexSet *_indexSet;
		unsigned _numberOfComponents;
		unsigned _numberOfMainComponents;
		uint64_t *_componentBound;
		uint64_t *_componentRank;
		uint64_t _indexRank;
		uint64_t _regionRank;
		uint64_t _index[7];
	};
	unsigned getNumberOfDimensions();
	uint64_t getNumberOfIndices();
	uint64_t getNumberOfRegions();
	Iterator &begin();
	Iterator &end();
        Iterator &curr();
protected:
	virtual void initConstantIterators() = 0;
	virtual void updateComponentBoundsOfIterator(Iterator *iterator) = 0;
	virtual void getIndex(Iterator &iterator) = 0;
	unsigned _numberOfDimensions;
	uint64_t _numberOfIndices;
	uint64_t _numberOfRegions;
	Iterator *_begin;
	Iterator *_end;
        Iterator *_curr;
};

} /* namespace BSP */
#endif /* BSPINDEXSET_HPP_ */
