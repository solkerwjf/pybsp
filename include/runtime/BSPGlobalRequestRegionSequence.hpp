/*
 * GlobalRequestRegionLinear.hpp
 *
 *  Created on: 2014-8-5
 *      Author: junfeng
 */

#ifndef GLOBALREQUESTREGIONSEQUENCE_HPP_
#define GLOBALREQUESTREGIONSEQUENCE_HPP_
#include "BSPGlobalRequestRegion.hpp"
#include "BSPIndexSetRegionSequence.hpp"

namespace BSP {

class GlobalRequestRegionSequence: public GlobalRequestRegion {
public:
	GlobalRequestRegionSequence(ArrayPartition &partition,
			IndexSetRegionSequence &indexSet, const std::string requestID);
	virtual ~GlobalRequestRegionSequence();
	virtual void getData(const uint64_t numberOfBytesPerElement,
			const uint64_t nData, char *data);
	virtual void setData(const uint64_t numberOfBytesPerElement,
			const uint64_t nData, const char *data);
};

} /* namespace BSP */
#endif /* GLOBALREQUESTREGIONSEQUENCE_HPP_ */
