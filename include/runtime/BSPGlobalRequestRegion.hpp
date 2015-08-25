/*
 * BSPGlobalRequestRegion.hpp
 *
 *  Created on: 2014-8-12
 *      Author: junfeng
 */

#ifndef BSPGLOBALREQUESTREGION_HPP_
#define BSPGLOBALREQUESTREGION_HPP_

#include "BSPGlobalRequest.hpp"

namespace BSP {

class GlobalRequestRegion: public BSP::GlobalRequest {
public:
	GlobalRequestRegion(ArrayPartition &partition);
	virtual ~GlobalRequestRegion();
	void allocateComponents();
	uint64_t getRegionWidth(const unsigned iDim, const uint64_t iComponent);
protected:
	uint64_t _nRegions;
	uint64_t _nComponentsAlongDim[7];
	uint64_t *_lowerOwnerPositionAlongDim[7];
	uint64_t *_upperOwnerPositionAlongDim[7];
	uint64_t *_lowerOffsetInOwnerAlongDim[7];
	uint64_t *_upperOffsetInOwnerAlongDim[7];
};

} /* namespace BSP */
#endif /* BSPGLOBALREQUESTREGION_HPP_ */
