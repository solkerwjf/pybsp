/*
 * GlobalRequestPointLinear.hpp
 *
 *  Created on: 2014-8-5
 *      Author: junfeng
 */

#ifndef GLOBALREQUESTPOINTSEQUENCE_HPP_
#define GLOBALREQUESTPOINTSEQUENCE_HPP_
#include "BSPGlobalRequest.hpp"
#include "BSPIndexSetPointSequence.hpp"

namespace BSP {

class GlobalRequestPointSequence: public GlobalRequest {
public:
	GlobalRequestPointSequence(ArrayPartition &partition,
			IndexSetPointSequence &indexSet, const std::string requestID);
	virtual ~GlobalRequestPointSequence();
	virtual void getData(const uint64_t numberOfBytesPerElement,
			const uint64_t nData, char *data);
	virtual void setData(const uint64_t numberOfBytesPerElement,
			const uint64_t nData, const char *data);
private:
	uint64_t _nRequests;
	uint64_t *_ownerIProc;
};

} /* namespace BSP */
#endif /* GLOBALREQUESTPOINTSEQUENCE_HPP_ */
