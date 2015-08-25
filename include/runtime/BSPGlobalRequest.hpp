/*
 * BSPGlobalRequest.hpp
 *
 *  Created on: 2014-8-12
 *      Author: junfeng
 */

#ifndef BSPGLOBALREQUEST_HPP_
#define BSPGLOBALREQUEST_HPP_
#include "BSPArrayPartition.hpp"
#include <string>

namespace BSP {

class GlobalRequest: public ArrayPartition {
public:
	GlobalRequest(ArrayPartition &partition);
	virtual ~GlobalRequest();
	uint64_t *getIndexList(const uint64_t procID);
	uint64_t getIndexLength(const uint64_t procID);
	char *getDataList(const uint64_t procID);
	uint64_t getDataCount(const uint64_t procID);
	virtual void getData(const uint64_t numberOfBytesPerElement,
			const uint64_t nData, char *data) = 0;
	virtual void setData(const uint64_t numberOfBytesPerElement,
			const uint64_t nData, const char *data) = 0;
        void setRequestID(std::string _requestID);
        std::string getRequestID() const;
protected:
	void allocateForProc(uint64_t iProc);
        std::string _requestID;
	uint64_t _nData;
	uint64_t *_indexLength;
	uint64_t **_indexList;
	uint64_t *_dataCount;
	char **_dataList;
};

} /* namespace BSP */
#endif /* BSPGLOBALREQUEST_HPP_ */
