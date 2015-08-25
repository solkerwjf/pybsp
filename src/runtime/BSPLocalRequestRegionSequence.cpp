/*
 * BSPLocalRequestRegionSequence.cpp
 *
 *  Created on: 2014-8-21
 *      Author: junfeng
 */

#include "BSPLocalRequestRegionSequence.hpp"

using namespace BSP;

LocalRequestRegionSequence::LocalRequestRegionSequence(ArrayShape &shape,
        IndexSetRegionSequence &indexSet) :
LocalRequestRegion(shape) {
    if (_numberOfDimensions != indexSet._numberOfDimensions)
        throw EInvalidArgument();

    _dataCount = indexSet.getNumberOfIndices();

    _nRegions = indexSet._numberOfRegions;
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        _nComponentsAlongDim[iDim] = indexSet._numberOfRegions;
    }
    allocateComponents();
    uint64_t indexLength = 2 + (1 + _numberOfDimensions) * _nRegions;
    allocate(indexLength);

    // index structure for k-dimensional array:
    // data byte count
    // n
    // start, width 0, ..., width k-1 of region 0,
    // start, width 0, ..., width k-1 of region 1,
    // ...
    // start, width 0, ..., width k-1 of region n-1,

    uint64_t srcOffset = 0;
    uint64_t dstOffset = 2;
    _indexList[0] = _numberOfBytesPerElement * _dataCount;
    _indexList[1] = _nRegions;
    for (uint64_t iRegion = 0; iRegion < _nRegions; iRegion++) {
        uint64_t currentLower = 0;
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            _lowerIndexAlongDim[iDim][iRegion] =
                    indexSet._lowerIndexList[srcOffset];
            _upperIndexAlongDim[iDim][iRegion] =
                    indexSet._upperIndexList[srcOffset];
            srcOffset++;

            if (iDim > 0) {
                currentLower *= _numberOfElementsAlongDimension[iDim];
            }
            currentLower += _lowerIndexAlongDim[iDim][iRegion];
            _indexList[dstOffset + 1 + iDim] =
                    _upperIndexAlongDim[iDim][iRegion]
                    - _lowerIndexAlongDim[iDim][iRegion] + 1;
        }
        _indexList[dstOffset] = currentLower;
        dstOffset += 1 + _numberOfDimensions;
    }
}

LocalRequestRegionSequence::~LocalRequestRegionSequence() {
}

