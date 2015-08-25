/*
 * GlobalRequestPointLinear.cpp
 *
 *  Created on: 2014-8-5
 *      Author: junfeng
 */

#include "BSPGlobalRequestPointSequence.hpp"

using namespace BSP;

GlobalRequestPointSequence::GlobalRequestPointSequence(
        ArrayPartition &partition, IndexSetPointSequence &indexSet, 
        const std::string requestID) :
GlobalRequest(partition) {
    // record the basic attributes
    setRequestID(requestID);
    _nRequests = indexSet._numberOfIndices;
    if (_nRequests == 0)
        throw EInvalidArgument();

    // allocate some auxiliary arrays
    _ownerIProc = new uint64_t[_nRequests];
    uint64_t *offsetInOwner = new uint64_t[_nRequests];
    if (_ownerIProc == NULL || offsetInOwner == NULL)
        throw ENotEnoughMemory();

    // transform the element indices to owner proc ID and offset in owner
    uint64_t position[7];
    const uint64_t *current = indexSet._indexList;
    for (uint64_t iRequest = 0; iRequest < _nRequests; iRequest++, current +=
            _numberOfDimensions) {
        getPosition(current, position, getRequestID(), _numberOfDimensions,
                current);
        uint64_t offset = current[0] - partition.getNode(0, position[0]);
        for (unsigned iDim = 1; iDim < _numberOfDimensions; iDim++) {
            uint64_t localStart = partition.getNode(iDim, position[iDim]);
            uint64_t localEnd = partition.getNode(iDim, position[iDim] + 1);
            offset *= localEnd - localStart;
            offset += current[iDim] - localStart;
        }
        uint64_t procID = getProcIDFromPosition(position);
        _ownerIProc[iRequest] = procID - _startProcID;
        offsetInOwner[iRequest] = offset;
        _dataCount[procID - _startProcID]++;
    }

    // encode the index lists for the local arrays
    for (uint64_t iProc = 0; iProc < _nProcsInGrid; iProc++) {
        if (_dataCount[iProc] > 0) {
            _indexLength[iProc] = 2 + _dataCount[iProc];
            allocateForProc(iProc);
            _indexList[iProc][0] = _numberOfBytesPerElement * _dataCount[iProc];
            _indexList[iProc][1] = 0;
        }
    }
    for (uint64_t iRequest = 0; iRequest < _nRequests; iRequest++) {
        uint64_t iProc = _ownerIProc[iRequest];
        _indexList[iProc][++_indexList[iProc][1] + 1] = offsetInOwner[iRequest];
    }

    delete[] offsetInOwner;
    
    if (_nData != indexSet.getNumberOfIndices())
        throw ECorruptedRuntime();
}

GlobalRequestPointSequence::~GlobalRequestPointSequence() {
    delete[] _ownerIProc;
}

void GlobalRequestPointSequence::getData(const uint64_t numberOfBytesPerElement,
        const uint64_t nData, char *data) {
    if (nData < _nData)
        throw EClientArrayTooSmall(getRequestID(), nData, _nData);
    if (numberOfBytesPerElement
            != _numberOfBytesPerElement || nData < _nData || data == NULL)
        throw EInvalidArgument();
    uint64_t *nDataInProc = new uint64_t[_nProcsInGrid];
    if (nDataInProc == NULL)
        throw ENotEnoughMemory();
    for (uint64_t iProc = 0; iProc < _nProcsInGrid; iProc++)
        nDataInProc[iProc] = 0;
    char *current = data;
    for (uint64_t iRequest = 0; iRequest < _nRequests; iRequest++, current +=
            _numberOfBytesPerElement) {
        uint64_t iProc = _ownerIProc[iRequest];
        char *currentData = _dataList[iProc]
                + _numberOfBytesPerElement * nDataInProc[iProc];
        nDataInProc[iProc]++;
        for (uint64_t iByte = 0; iByte < _numberOfBytesPerElement; iByte++)
            current[iByte] = currentData[iByte];
    }
    delete[] nDataInProc;
}

void GlobalRequestPointSequence::setData(const uint64_t numberOfBytesPerElement,
        const uint64_t nData, const char *data) {
    if (nData < _nData)
        throw EClientArrayTooSmall(getRequestID(), nData, _nData);
    if (numberOfBytesPerElement
            != _numberOfBytesPerElement || nData < _nData || data == NULL)
        throw EInvalidArgument();
    uint64_t *nDataInProc = new uint64_t[_nProcsInGrid];
    if (nDataInProc == NULL)
        throw ENotEnoughMemory();
    for (uint64_t iProc = 0; iProc < _nProcsInGrid; iProc++)
        nDataInProc[iProc] = 0;
    const char *current = data;
    for (uint64_t iRequest = 0; iRequest < _nRequests; iRequest++, current +=
            _numberOfBytesPerElement) {
        uint64_t iProc = _ownerIProc[iRequest];
        char *currentData = _dataList[iProc]
                + _numberOfBytesPerElement * nDataInProc[iProc];
        nDataInProc[iProc]++;
        for (uint64_t iByte = 0; iByte < _numberOfBytesPerElement; iByte++)
            currentData[iByte] = current[iByte];
    }
    delete[] nDataInProc;
}

