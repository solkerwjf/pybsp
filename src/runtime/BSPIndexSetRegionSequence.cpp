/*
 * BSPIndexSetRegionSequence.cpp
 *
 *  Created on: 2014-8-20
 *      Author: junfeng
 */

#include "BSPIndexSetRegionSequence.hpp"

using namespace BSP;

IndexSetRegionSequence::IndexSetRegionSequence(LocalArray &lower,
        LocalArray &upper) :
IndexSet(lower.getElementCount(1), computeNumberOfIndices(lower, upper)) {
    this->initConstantIterators();
    if (_begin == NULL || _end == NULL || _curr == NULL)
        throw ENotEnoughMemory();
}

IndexSetRegionSequence::~IndexSetRegionSequence() {
    delete[] _lowerIndexList;
    delete[] _upperIndexList;
}

uint64_t IndexSetRegionSequence::computeNumberOfIndices(LocalArray &lower,
        LocalArray &upper) {
    unsigned numberOfDimensions = lower.getElementCount(1);
    _numberOfRegions = lower.getElementCount(0);
    if (numberOfDimensions == 0 || _numberOfRegions == 0)
        throw EInvalidArgument();
    if (upper.getElementCount(1) != numberOfDimensions
            || upper.getElementCount(0) != _numberOfRegions)
        throw EInvalidArgument();
    _lowerIndexList = new uint64_t[numberOfDimensions * _numberOfRegions];
    _upperIndexList = new uint64_t[numberOfDimensions * _numberOfRegions];
    if (_lowerIndexList == NULL || _upperIndexList == NULL)
        throw ENotEnoughMemory();
    memcpy(_lowerIndexList, lower.getData(),
            sizeof (uint64_t) * numberOfDimensions * _numberOfRegions);
    memcpy(_upperIndexList, upper.getData(),
            sizeof (uint64_t) * numberOfDimensions * _numberOfRegions);

    uint64_t result = 0;
    uint64_t k = 0;
    for (uint64_t iRegion = 0; iRegion < _numberOfRegions; iRegion++) {
        uint64_t currentRegionSize = 1;
        for (unsigned iDim = 0; iDim < numberOfDimensions; iDim++) {
            uint64_t currentLower = _lowerIndexList[k];
            uint64_t currentUpper = _upperIndexList[k];
            k++;
            if (currentUpper < currentLower)
                throw EInvalidRegionDescriptor(iDim, iRegion, 
                        currentLower, currentUpper);
            currentRegionSize *= currentUpper - currentLower + 1;
        }
        result += currentRegionSize;
    }
    return result;
}

void IndexSetRegionSequence::initConstantIterators() {
    uint64_t initialComponentBound[8];
    initialComponentBound[0] = _numberOfRegions;
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        initialComponentBound[iDim + 1] = _upperIndexList[iDim]
                - _lowerIndexList[iDim] + 1;
    }
    _begin = new Iterator(this, _numberOfDimensions + 1, 1,
            initialComponentBound, true);
    _end = new Iterator(this, _numberOfDimensions + 1, 1, initialComponentBound,
            false);
    _curr = new Iterator(*_begin);
}

void IndexSetRegionSequence::updateComponentBoundsOfIterator(
        Iterator *iterator) {
    uint64_t iRegion = iterator->getComponentRank(0);
    uint64_t regionIndexOffset = iRegion * _numberOfDimensions;
    uint64_t *currentLower = _lowerIndexList + regionIndexOffset;
    uint64_t *currentUpper = _upperIndexList + regionIndexOffset;
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        iterator->setComponentBound(iDim + 1,
                currentUpper[iDim] - currentLower[iDim] + 1);
    }
}

void IndexSetRegionSequence::getIndex(Iterator &iterator) {
    uint64_t iRegion = iterator.getComponentRank(0);
    uint64_t regionIndexOffset = iRegion * _numberOfDimensions;
    uint64_t *currentLower = _lowerIndexList + regionIndexOffset;
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        iterator._index[iDim] = currentLower[iDim]
                + iterator.getComponentRank(iDim + 1);
    }
}


