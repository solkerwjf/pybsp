/*
 * BSPLocalRequestRegion.hpp
 *
 *  Created on: 2014-8-20
 *      Author: junfeng
 */

#ifndef BSPLOCALREQUESTREGION_HPP_
#define BSPLOCALREQUESTREGION_HPP_

#include "BSPLocalRequest.hpp"

namespace BSP {

class LocalRequestRegion: public BSP::LocalRequest {
public:
	LocalRequestRegion(ArrayShape &shape);
	virtual ~LocalRequestRegion();
	void allocateComponents();
	uint64_t getRegionWidth(const unsigned iDim, const uint64_t iComponent);
protected:
	uint64_t _nRegions;

	uint64_t _nComponentsAlongDim[7];
	uint64_t *_lowerIndexAlongDim[7];
	uint64_t *_upperIndexAlongDim[7];
};

} /* namespace BSP */
#endif /* BSPLOCALREQUESTREGION_HPP_ */
