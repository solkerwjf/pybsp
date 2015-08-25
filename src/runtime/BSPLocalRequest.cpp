/*
 * BSPLocalRequest.cpp
 *
 *  Created on: 2014-8-20
 *      Author: junfeng
 */

#include "BSPLocalRequest.hpp"

using namespace BSP;

LocalRequest::LocalRequest(ArrayShape &shape) :
ArrayShape(shape) {
    _indexLength = 0;
    _indexList = NULL;
    _dataCount = 0;
}

LocalRequest::~LocalRequest() {
    if (_indexLength > 0) {
        delete[] _indexList;
    }
}

void LocalRequest::allocate(uint64_t indexLength) {
    if (_indexLength != 0)
        throw ENotAvailable();
    if (indexLength == 0)
        throw EInvalidArgument();
    _indexLength = indexLength;
    _indexList = new uint64_t[_indexLength];
    if (_indexList == NULL)
        throw ENotEnoughMemory();
}

uint64_t *LocalRequest::getIndexList() {
    return _indexList;
}

uint64_t LocalRequest::getIndexLength() {
    return _indexLength;
}

uint64_t LocalRequest::getDataCount() {
    return _dataCount;
}

void LocalRequest::setRequestID(std::string _requestID) {
    this->_requestID = _requestID;
}

std::string LocalRequest::getRequestID() const {
    return _requestID;
}


