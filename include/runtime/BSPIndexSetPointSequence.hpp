/*
 * BSPIndetSetPointSequence.hpp
 *
 *  Created on: 2014-8-18
 *      Author: junfeng
 */

#ifndef BSPINDEXSETPOINTSEQUENCE_HPP_
#define BSPINDEXSETPOINTSEQUENCE_HPP_
#include "BSPIndexSet.hpp"
#include "BSPLocalArray.hpp"
namespace BSP {

class IndexSetPointSequence: public IndexSet {
	friend class GlobalRequestPointSequence;
	friend class LocalRequestPointSequence;
public:
	IndexSetPointSequence(LocalArray &points);
	virtual ~IndexSetPointSequence();
protected:
	virtual void initConstantIterators();
	virtual void updateComponentBoundsOfIterator(Iterator *);
	virtual void getIndex(Iterator &iterator);
private:
	uint64_t *_indexList;
};

} /* namespace BSP */
#endif /* BSPINDETSETPOINTSEQUENCE_HPP_ */
