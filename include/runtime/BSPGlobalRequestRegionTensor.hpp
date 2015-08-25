/*
 * GlobalRequestRegionTensor.hpp
 *
 *  Created on: 2014-8-5
 *      Author: junfeng
 */

#ifndef GLOBALREQUESTREGIONTENSOR_HPP_
#define GLOBALREQUESTREGIONTENSOR_HPP_
#include "BSPGlobalRequestRegion.hpp"
#include "BSPIndexSetRegionTensor.hpp"

namespace BSP {

class GlobalRequestRegionTensor: public GlobalRequestRegion {
public:
	GlobalRequestRegionTensor(ArrayPartition &partition,
			IndexSetRegionTensor &indexSet, const std::string requestID);
	virtual ~GlobalRequestRegionTensor();
	virtual void getData(const uint64_t numberOfBytesPerElement,
			const uint64_t nData, char *data);
	virtual void setData(const uint64_t numberOfBytesPerElement,
			const uint64_t nData, const char *data);
};

} /* namespace BSP */
#endif /* GLOBALREQUESTREGIONTENSOR_HPP_ */
