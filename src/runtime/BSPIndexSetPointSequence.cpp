/*
 * BSPIndetSetPointSequence.cpp
 *
 *  Created on: 2014-8-18
 *      Author: junfeng
 */

#include "BSPIndexSetPointSequence.hpp"

using namespace BSP;

IndexSetPointSequence::IndexSetPointSequence(LocalArray &points) :
IndexSet(points.getElementCount(1), points.getElementCount(0)) {
    _numberOfRegions = 0;
    uint64_t indexLength = _numberOfIndices * _numberOfDimensions;
    _indexList = new uint64_t[indexLength];
    if (_indexList == NULL)
        throw ENotEnoughMemory();
    memcpy(_indexList, points.getData(), indexLength * sizeof (uint64_t));
    this->initConstantIterators();
    if (_begin == NULL || _end == NULL || _curr == NULL)
        throw ENotEnoughMemory();
}

IndexSetPointSequence::~IndexSetPointSequence() {
    delete[] _indexList;
}

void IndexSetPointSequence::initConstantIterators() {
    uint64_t componentBound[1];
    componentBound[0] = _numberOfIndices;
    _begin = new Iterator(this, 1, 1, componentBound, true);
    _end = new Iterator(this, 1, 1, componentBound, false);
    _curr = new Iterator(*_begin);
}

void IndexSetPointSequence::updateComponentBoundsOfIterator(
        Iterator *) {
}

void IndexSetPointSequence::getIndex(Iterator &iterator) {
    uint64_t *current = _indexList
            + _numberOfDimensions * iterator.getIndexRank();
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        iterator._index[iDim] = current[iDim];
    }
}


