/*
 * BSPGlobalRequest.cpp
 *
 *  Created on: 2014-8-12
 *      Author: junfeng
 */

#include "BSPGlobalRequest.hpp"

using namespace BSP;

GlobalRequest::GlobalRequest(ArrayPartition &partition) :
ArrayPartition(partition) {
    // set the basic attributes
    if (_numberOfBytesPerElement == 0 || _numberOfDimensions == 0
            || _nProcsInGrid == 0)
        throw EInvalidArgument();

    // initialize the counters
    _nData = 0;
    _indexLength = new uint64_t[_nProcsInGrid];
    _indexList = new uint64_t *[_nProcsInGrid];
    _dataCount = new uint64_t[_nProcsInGrid];
    _dataList = new char *[_nProcsInGrid];
    if (_indexLength == NULL || _indexList == NULL || _dataCount == NULL
            || _dataList == NULL)
        throw ENotEnoughMemory();
    for (uint64_t iProc = 0; iProc < _nProcsInGrid; iProc++) {
        _indexLength[iProc] = 0;
        _dataCount[iProc] = 0;
        _indexList[iProc] = NULL;
        _dataList[iProc] = NULL;
    }
}

void GlobalRequest::setRequestID(std::string _requestID) {
    this->_requestID = _requestID;
}

std::string GlobalRequest::getRequestID() const {
    return _requestID;
}

GlobalRequest::~GlobalRequest() {
    for (uint64_t iProc = 0; iProc < _nProcsInGrid; iProc++) {
        if (_dataCount[iProc] > 0) {
            delete[] _indexList[iProc];
            delete[] _dataList[iProc];
        }
    }
    delete[] _indexLength;
    delete[] _indexList;
    delete[] _dataCount;
    delete[] _dataList;
}

uint64_t *GlobalRequest::getIndexList(const uint64_t procID) {
    if (procID < _startProcID || procID >= _startProcID + _nProcsInGrid)
        return NULL;
    else
        return _indexList[procID - _startProcID];
}

uint64_t GlobalRequest::getIndexLength(const uint64_t procID) {
    if (procID < _startProcID || procID >= _startProcID + _nProcsInGrid)
        return 0;
    else
        return _indexLength[procID - _startProcID];
}

char *GlobalRequest::getDataList(const uint64_t procID) {
    if (procID < _startProcID || procID >= _startProcID + _nProcsInGrid)
        return NULL;
    else
        return _dataList[procID - _startProcID];
}

uint64_t GlobalRequest::getDataCount(const uint64_t procID) {
    if (procID < _startProcID || procID >= _startProcID + _nProcsInGrid)
        return 0;
    else
        return _dataCount[procID - _startProcID];
}

void GlobalRequest::allocateForProc(uint64_t iProc) {
    if (iProc >= _nProcsInGrid)
        throw EInvalidArgument();
    if (_indexList[iProc] != NULL || _dataList[iProc] != NULL)
        throw ENotAvailable();
    if (_indexLength[iProc] == 0 || _dataCount[iProc] == 0)
        throw EInvalidArgument();
    _indexList[iProc] = new uint64_t[_indexLength[iProc]];
    _dataList[iProc] = new char[_numberOfBytesPerElement * _dataCount[iProc]];
    if (_indexList[iProc] == NULL || _dataList[iProc] == NULL)
        throw ENotEnoughMemory();
    _nData += _dataCount[iProc];
}


