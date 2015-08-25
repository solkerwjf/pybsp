/*
 * BSPIndexSetPointTensor.cpp
 *
 *  Created on: 2014-8-20
 *      Author: junfeng
 */

#include "BSPIndexSetPointTensor.hpp"

using namespace BSP;

IndexSetPointTensor::IndexSetPointTensor(const unsigned numberOfDimensions,
        LocalArray **componentAlongDim) :
IndexSet(numberOfDimensions,
computeNumberOfIndices(numberOfDimensions, componentAlongDim)) {
    _numberOfRegions = 0;
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        _numberOfComponentsAlongDim[iDim] =
                componentAlongDim[iDim]->getElementCount(LocalArray::ALL_DIMS);
        if (_numberOfComponentsAlongDim[iDim] == 0
                || componentAlongDim[iDim]->getNumberOfBytesPerElement()
                != sizeof (uint64_t))
            throw EInvalidArgument();
        _componentAlongDim[iDim] =
                new uint64_t[_numberOfComponentsAlongDim[iDim]];
        if (_componentAlongDim[iDim] == NULL)
            throw ENotEnoughMemory();
        memcpy(_componentAlongDim[iDim], componentAlongDim[iDim]->getData(),
                _numberOfComponentsAlongDim[iDim] * sizeof (uint64_t));
    }
    this->initConstantIterators();
    if (_begin == NULL || _end == NULL || _curr == NULL)
        throw ENotEnoughMemory();
}

IndexSetPointTensor::~IndexSetPointTensor() {
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        delete[] _componentAlongDim[iDim];
    }
}

uint64_t IndexSetPointTensor::computeNumberOfIndices(
        const unsigned numberOfDimensions, LocalArray **componentAlongDim) {
    uint64_t result = 1;
    for (unsigned iDim = 0; iDim < numberOfDimensions; iDim++) {
        result *= componentAlongDim[iDim]->getElementCount(LocalArray::ALL_DIMS);
    }
    return result;
}

void IndexSetPointTensor::initConstantIterators() {
    _begin = new Iterator(this, _numberOfDimensions, _numberOfDimensions,
            _numberOfComponentsAlongDim, true);
    _end = new Iterator(this, _numberOfDimensions, _numberOfDimensions,
            _numberOfComponentsAlongDim, false);
    _curr = new Iterator(*_begin);
}

void IndexSetPointTensor::updateComponentBoundsOfIterator(Iterator *) {
}

void IndexSetPointTensor::getIndex(Iterator &iterator) {
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        iterator._index[iDim] =
                _componentAlongDim[iDim][iterator.getComponentRank(iDim)];
    }
}


