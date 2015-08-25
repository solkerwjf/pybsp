/*
 * BSPIndexSetRegionTensor.hpp
 *
 *  Created on: 2014-8-20
 *      Author: junfeng
 */

#ifndef BSPINDEXSETREGIONTENSOR_HPP_
#define BSPINDEXSETREGIONTENSOR_HPP_

#include "BSPIndexSet.hpp"
#include "BSPLocalArray.hpp"

namespace BSP {

class IndexSetRegionTensor: public BSP::IndexSet {
	friend class GlobalRequestRegionTensor;
	friend class LocalRequestRegionTensor;
public:
	IndexSetRegionTensor(const unsigned numberOfDimensions,
			LocalArray **lowerComponentAlongDim,
			LocalArray **upperComponentAlongDim);
	virtual ~IndexSetRegionTensor();
protected:
	uint64_t computeNumberOfIndices(const unsigned numberOfDimensions,
			LocalArray **lowerComponentAlongDim,
			LocalArray **upperComponentAlongDim);
	virtual void initConstantIterators();
	virtual void updateComponentBoundsOfIterator(Iterator *iterator);
	virtual void getIndex(Iterator &iterator);
private:
	uint64_t _numberOfComponentsAlongDim[7];
	uint64_t *_lowerComponentAlongDim[7];
	uint64_t *_upperComponentAlongDim[7];
};

} /* namespace BSP */
#endif /* BSPINDEXSETREGIONTENSOR_HPP_ */
