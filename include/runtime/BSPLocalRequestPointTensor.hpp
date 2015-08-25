/*
 * BSPLocalRequestPointTensor.hpp
 *
 *  Created on: 2014-8-21
 *      Author: junfeng
 */

#ifndef BSPLOCALREQUESTPOINTTENSOR_HPP_
#define BSPLOCALREQUESTPOINTTENSOR_HPP_
#include "BSPLocalRequest.hpp"
#include "BSPIndexSetPointTensor.hpp"
namespace BSP {

class LocalRequestPointTensor: public LocalRequest {
public:
	LocalRequestPointTensor(ArrayShape &shape, IndexSetPointTensor &indexSet);
	virtual ~LocalRequestPointTensor();
};

} /* namespace BSP */
#endif /* BSPLOCALREQUESTPOINTTENSOR_HPP_ */
