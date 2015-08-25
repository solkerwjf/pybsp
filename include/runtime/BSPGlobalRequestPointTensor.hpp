/*
 * GlobalRequestPointTensor.hpp
 *
 *  Created on: 2014-8-5
 *      Author: junfeng
 */

#ifndef GLOBALREQUESTPOINTTENSOR_HPP_
#define GLOBALREQUESTPOINTTENSOR_HPP_
#include "BSPGlobalRequest.hpp"
#include "BSPIndexSetPointTensor.hpp"

namespace BSP {

class GlobalRequestPointTensor: public GlobalRequest {
public:
	GlobalRequestPointTensor(ArrayPartition &partition,
			IndexSetPointTensor &indexSet, const std::string requestID);
	virtual ~GlobalRequestPointTensor();
	virtual void getData(const uint64_t numberOfBytesPerElement,
			const uint64_t nData, char *data);
	virtual void setData(const uint64_t numberOfBytesPerElement,
			const uint64_t nData, const char *data);
private:
	uint64_t _nComponentsAlongDim[7];
	uint64_t *_ownerPositionAlongDim[7];
	uint64_t *_offsetInOwnerAlongDim[7];
};

} /* namespace BSP */
#endif /* GLOBALREQUESTPOINTTENSOR_HPP_ */
