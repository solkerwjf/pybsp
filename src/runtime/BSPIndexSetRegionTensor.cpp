/*
 * BSPIndexSetRegionTensor.cpp
 *
 *  Created on: 2014-8-20
 *      Author: junfeng
 */

#include "BSPIndexSetRegionTensor.hpp"

using namespace BSP;

IndexSetRegionTensor::IndexSetRegionTensor(const unsigned numberOfDimensions,
        LocalArray **lowerComponentAlongDim, LocalArray **upperComponentAlongDim) :
IndexSet(numberOfDimensions,
computeNumberOfIndices(numberOfDimensions,
lowerComponentAlongDim, upperComponentAlongDim)) {
    this->initConstantIterators();
    if (_begin == NULL || _end == NULL || _curr == NULL)
        throw ENotEnoughMemory();
}

IndexSetRegionTensor::~IndexSetRegionTensor() {
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        delete[] _lowerComponentAlongDim[iDim];
        delete[] _upperComponentAlongDim[iDim];
    }
}

uint64_t IndexSetRegionTensor::computeNumberOfIndices(
        const unsigned numberOfDimensions, LocalArray **lowerComponentAlongDim,
        LocalArray **upperComponentAlongDim) {
    _numberOfRegions = 1;
    uint64_t combination[7];
    for (unsigned iDim = 0; iDim < numberOfDimensions; iDim++) {
        if (lowerComponentAlongDim[iDim]->getNumberOfBytesPerElement()
                != sizeof (uint64_t)
                || upperComponentAlongDim[iDim]->getNumberOfBytesPerElement()
                != sizeof (uint64_t))
            throw EInvalidArgument();
        _numberOfComponentsAlongDim[iDim] =
                lowerComponentAlongDim[iDim]->getElementCount(
                LocalArray::ALL_DIMS);
        if (0 == _numberOfComponentsAlongDim[iDim]
                || upperComponentAlongDim[iDim]->getElementCount(
                LocalArray::ALL_DIMS)
                != _numberOfComponentsAlongDim[iDim])
            throw EInvalidArgument();
        _lowerComponentAlongDim[iDim] =
                new uint64_t[_numberOfComponentsAlongDim[iDim]];
        _upperComponentAlongDim[iDim] =
                new uint64_t[_numberOfComponentsAlongDim[iDim]];
        if (_lowerComponentAlongDim[iDim] == NULL
                || _upperComponentAlongDim[iDim] == NULL)
            throw ENotEnoughMemory();
        memcpy(_lowerComponentAlongDim[iDim],
                lowerComponentAlongDim[iDim]->getData(),
                _numberOfComponentsAlongDim[iDim] * sizeof (uint64_t));
        memcpy(_upperComponentAlongDim[iDim],
                upperComponentAlongDim[iDim]->getData(),
                _numberOfComponentsAlongDim[iDim] * sizeof (uint64_t));
        combination[iDim] = 0;
        for (uint64_t iComponent = 0;
                iComponent < _numberOfComponentsAlongDim[iDim]; iComponent++) {
            if (_upperComponentAlongDim[iDim][iComponent]
                    < _lowerComponentAlongDim[iDim][iComponent])
                throw EInvalidRegionDescriptor(iDim, iComponent,
                        _lowerComponentAlongDim[iDim][iComponent],
                        _upperComponentAlongDim[iDim][iComponent]);
        }
        _numberOfRegions *= _numberOfComponentsAlongDim[iDim];
    }

    uint64_t result = 0;
    while (combination[0] < _numberOfComponentsAlongDim[0]) {
        uint64_t currentRegionSize = 1;
        for (unsigned iDim = 0; iDim < numberOfDimensions; iDim++) {
            currentRegionSize *=
                    _upperComponentAlongDim[iDim][combination[iDim]]
                    - _lowerComponentAlongDim[iDim][combination[iDim]]
                    + 1;
        }
        result += currentRegionSize;
        for (int iDim = numberOfDimensions - 1; iDim >= 0; iDim--) {
            combination[iDim]++;
            if (iDim == 0
                    || combination[iDim] < _numberOfComponentsAlongDim[iDim])
                break;
            combination[iDim] = 0;
        }
    }

    return result;
}

void IndexSetRegionTensor::initConstantIterators() {
    uint64_t initialComponentBound[14];
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        initialComponentBound[iDim] = _numberOfComponentsAlongDim[iDim];
        initialComponentBound[_numberOfDimensions + iDim] =
                _upperComponentAlongDim[iDim][0]
                - _lowerComponentAlongDim[iDim][0] + 1;
    }
    _begin = new Iterator(this, 2 * _numberOfDimensions, _numberOfDimensions,
            initialComponentBound, true);
    _end = new Iterator(this, 2 * _numberOfDimensions, _numberOfDimensions,
            initialComponentBound, false);
    _curr = new Iterator(*_begin);
}

void IndexSetRegionTensor::updateComponentBoundsOfIterator(Iterator *iterator) {
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        uint64_t newBound =
                _upperComponentAlongDim[iDim][iterator->getComponentRank(iDim)]
                - _lowerComponentAlongDim[iDim][iterator->getComponentRank(
                iDim)] + 1;
        iterator->setComponentBound(_numberOfDimensions + iDim, newBound);
    }
}

void IndexSetRegionTensor::getIndex(Iterator &iterator) {
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        iterator._index[iDim] =
                _lowerComponentAlongDim[iDim][iterator.getComponentRank(iDim)]
                + iterator.getComponentRank(_numberOfDimensions + iDim);
    }
}


