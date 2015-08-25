/*
 * BSPLocalRequestPointSequence.cpp
 *
 *  Created on: 2014-8-21
 *      Author: junfeng
 */

#include "BSPLocalRequestPointSequence.hpp"

using namespace BSP;

LocalRequestPointSequence::LocalRequestPointSequence(ArrayShape &shape, IndexSetPointSequence &indexSet) :
LocalRequest(shape) {
    if (_numberOfDimensions != indexSet._numberOfDimensions)
        throw EInvalidArgument();
    _dataCount = indexSet.getNumberOfIndices();
    uint64_t indexLength = 2 + indexSet._numberOfIndices;
    allocate(indexLength);

    // index structure:
    // data byte count
    // n
    // point 0
    // point 1
    // ...
    // point n - 1

    _indexList[0] = indexSet._numberOfIndices * _numberOfBytesPerElement;
    _indexList[1] = indexSet._numberOfIndices;
    for (IndexSet::Iterator iter = indexSet.begin(); iter != indexSet.end(); ++iter) {
        uint64_t current = iter.getIndex(0);
        for (unsigned iDim = 1; iDim < _numberOfDimensions; ++iDim) {
            current *= _numberOfElementsAlongDimension[iDim];
            current += iter.getIndex(iDim);
        }
        _indexList[iter.getIndexRank() + 2] = current;
    }
}

LocalRequestPointSequence::~LocalRequestPointSequence() {
}

