/*
 * BSPIndexSetRegionSequence.hpp
 *
 *  Created on: 2014-8-20
 *      Author: junfeng
 */

#ifndef BSPINDEXSETREGIONSEQUENCE_HPP_
#define BSPINDEXSETREGIONSEQUENCE_HPP_

#include "BSPIndexSet.hpp"
#include "BSPLocalArray.hpp"

namespace BSP {

class IndexSetRegionSequence: public BSP::IndexSet {
	friend class GlobalRequestRegionSequence;
	friend class LocalRequestRegionSequence;
public:
	IndexSetRegionSequence(LocalArray &lower, LocalArray &upper);
	virtual ~IndexSetRegionSequence();
protected:
	uint64_t computeNumberOfIndices(LocalArray &lower, LocalArray &upper);
	virtual void initConstantIterators();
	virtual void updateComponentBoundsOfIterator(Iterator *iterator);
	virtual void getIndex(Iterator &iterator);
private:
	uint64_t *_lowerIndexList;
	uint64_t *_upperIndexList;
};

} /* namespace BSP */
#endif /* BSPINDEXSETREGIONSEQUENCE_HPP_ */
