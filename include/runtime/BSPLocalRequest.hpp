/*
 * BSPLocalRequest.hpp
 *
 *  Created on: 2014-8-20
 *      Author: junfeng
 */

#ifndef BSPLOCALREQUEST_HPP_
#define BSPLOCALREQUEST_HPP_

#include "BSPArrayShape.hpp"

namespace BSP {

class LocalRequest: public BSP::ArrayShape {
public:
	LocalRequest(ArrayShape &shape);
	virtual ~LocalRequest();
	uint64_t *getIndexList();
	uint64_t getIndexLength();
	uint64_t getDataCount();
        void setRequestID(std::string _requestID);
        std::string getRequestID() const;
protected:
	void allocate(uint64_t indexLength);
	uint64_t _indexLength;
	uint64_t *_indexList;
	uint64_t _dataCount;
        std::string _requestID;
};

} /* namespace BSP */
#endif /* BSPLOCALREQUEST_HPP_ */
