/*
 * BSPIndexSetPointTensor.hpp
 *
 *  Created on: 2014-8-20
 *      Author: junfeng
 */

#ifndef BSPINDEXSETPOINTTENSOR_HPP_
#define BSPINDEXSETPOINTTENSOR_HPP_

#include "BSPIndexSet.hpp"
#include "BSPLocalArray.hpp"

namespace BSP {

class IndexSetPointTensor: public BSP::IndexSet {
	friend class GlobalRequestPointTensor;
	friend class LocalRequestPointTensor;
public:
	IndexSetPointTensor(const unsigned numberOfDimensions,
			LocalArray **componentAlongDim);
	virtual ~IndexSetPointTensor();
protected:
	uint64_t computeNumberOfIndices(const unsigned numberOfDimensions,
			LocalArray **componentAlongDim);
	virtual void initConstantIterators();
	virtual void updateComponentBoundsOfIterator(Iterator *);
	virtual void getIndex(Iterator &iterator);
private:
	uint64_t _numberOfComponentsAlongDim[7];
	uint64_t *_componentAlongDim[7];
};

} /* namespace BSP */
#endif /* BSPINDEXSETPOINTTENSOR_HPP_ */
