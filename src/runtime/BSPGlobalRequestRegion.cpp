/*
 * BSPGlobalRequestRegion.cpp
 *
 *  Created on: 2014-8-12
 *      Author: junfeng
 */

#include "BSPGlobalRequestRegion.hpp"

using namespace BSP;

GlobalRequestRegion::GlobalRequestRegion(ArrayPartition &partition) :
GlobalRequest(partition) {
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        _nComponentsAlongDim[iDim] = 0;
        _lowerOwnerPositionAlongDim[iDim] = NULL;
        _upperOwnerPositionAlongDim[iDim] = NULL;
        _lowerOffsetInOwnerAlongDim[iDim] = NULL;
        _upperOffsetInOwnerAlongDim[iDim] = NULL;
    }
}

GlobalRequestRegion::~GlobalRequestRegion() {
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        if (_nComponentsAlongDim[iDim] == 0)
            continue;
        delete[] _lowerOwnerPositionAlongDim[iDim];
        delete[] _upperOwnerPositionAlongDim[iDim];
        delete[] _lowerOffsetInOwnerAlongDim[iDim];
        delete[] _upperOffsetInOwnerAlongDim[iDim];
    }
}

void GlobalRequestRegion::allocateComponents() {
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        if (_nComponentsAlongDim[iDim] == 0)
            throw EInvalidArgument();

        _lowerOwnerPositionAlongDim[iDim] =
                new uint64_t[_nComponentsAlongDim[iDim]];
        _lowerOffsetInOwnerAlongDim[iDim] =
                new uint64_t[_nComponentsAlongDim[iDim]];
        _upperOwnerPositionAlongDim[iDim] =
                new uint64_t[_nComponentsAlongDim[iDim]];
        _upperOffsetInOwnerAlongDim[iDim] =
                new uint64_t[_nComponentsAlongDim[iDim]];

        if (_lowerOwnerPositionAlongDim[iDim] == NULL
                || _lowerOffsetInOwnerAlongDim[iDim] == NULL
                || _upperOwnerPositionAlongDim[iDim] == NULL
                || _upperOffsetInOwnerAlongDim[iDim] == NULL)
            throw ENotEnoughMemory();
    }
}

uint64_t GlobalRequestRegion::getRegionWidth(const unsigned iDim,
        const uint64_t iComponent) {
    if (iDim >= _numberOfDimensions)
        throw EInvalidArgument();
    if (iComponent >= _nComponentsAlongDim[iDim])
        throw EInvalidArgument();
    uint64_t lower = getNode(iDim,
            _lowerOwnerPositionAlongDim[iDim][iComponent])
            + _lowerOffsetInOwnerAlongDim[iDim][iComponent];
    uint64_t upper = getNode(iDim,
            _upperOwnerPositionAlongDim[iDim][iComponent])
            + _upperOffsetInOwnerAlongDim[iDim][iComponent];
    return upper - lower + 1;
}

