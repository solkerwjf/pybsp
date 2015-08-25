/*
 * BSPLocalRequestRegionTensor.hpp
 *
 *  Created on: 2014-8-21
 *      Author: junfeng
 */

#ifndef BSPLOCALREQUESTREGIONTENSOR_HPP_
#define BSPLOCALREQUESTREGIONTENSOR_HPP_

#include "BSPLocalRequestRegion.hpp"
#include "BSPIndexSetRegionTensor.hpp"

namespace BSP {

class LocalRequestRegionTensor: public BSP::LocalRequestRegion {
public:
	LocalRequestRegionTensor(ArrayShape &shape, IndexSetRegionTensor &indexSet);
	virtual ~LocalRequestRegionTensor();
};

} /* namespace BSP */
#endif /* BSPLOCALREQUESTREGIONTENSOR_HPP_ */
