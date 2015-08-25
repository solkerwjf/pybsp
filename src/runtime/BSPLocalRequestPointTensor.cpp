/*
 * BSPLocalRequestPointTensor.cpp
 *
 *  Created on: 2014-8-21
 *      Author: junfeng
 */

#include "BSPLocalRequestPointTensor.hpp"

using namespace BSP;

LocalRequestPointTensor::LocalRequestPointTensor(ArrayShape &shape,
        IndexSetPointTensor &indexSet) :
LocalRequest(shape) {
    if (_numberOfDimensions != indexSet._numberOfDimensions)
        throw EInvalidArgument();
    _dataCount = indexSet.getNumberOfIndices();
    uint64_t indexLength = 1 + _numberOfDimensions;
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        indexLength += indexSet._numberOfComponentsAlongDim[iDim];
    }
    allocate(indexLength);

    // index structure for k-dimensional array
    // data byte count
    // n[0]
    // n[1]
    // ...
    // n[k-1]
    // component[0][0]
    // ...
    // component[0][n[0]-1]
    // component[1][0]
    // ...
    // component[1][n[1]-1]
    // ...
    // component[k-1][0]
    // ...
    // component[k-1][n[k-1]-1]
    _indexList[0] = _dataCount * _numberOfBytesPerElement;
    uint64_t offset = _numberOfDimensions + 1;
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        _indexList[iDim + 1] = indexSet._numberOfComponentsAlongDim[iDim];
        memcpy(_indexList + offset, indexSet._componentAlongDim[iDim],
                sizeof (uint64_t) * indexSet._numberOfComponentsAlongDim[iDim]);
        offset += indexSet._numberOfComponentsAlongDim[iDim];
    }
}

LocalRequestPointTensor::~LocalRequestPointTensor() {
}

