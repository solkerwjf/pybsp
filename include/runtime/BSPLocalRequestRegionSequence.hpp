/*
 * BSPLocalRequestRegionSequence.hpp
 *
 *  Created on: 2014-8-21
 *      Author: junfeng
 */

#ifndef BSPLOCALREQUESTREGIONSEQUENCE_HPP_
#define BSPLOCALREQUESTREGIONSEQUENCE_HPP_

#include "BSPLocalRequestRegion.hpp"
#include "BSPIndexSetRegionSequence.hpp"

namespace BSP {

class LocalRequestRegionSequence: public BSP::LocalRequestRegion {
public:
	LocalRequestRegionSequence(ArrayShape &shape,
			IndexSetRegionSequence &indexSet);
	virtual ~LocalRequestRegionSequence();
};

} /* namespace BSP */
#endif /* BSPLOCALREQUESTREGIONSEQUENCE_HPP_ */
