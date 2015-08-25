/*
 * BSPPartition.cpp
 *
 *  Created on: 2014-8-5
 *      Author: junfeng
 */

#include "BSPArrayPartition.hpp"
#include "BSPRuntime.hpp"
#include <iostream>
using namespace BSP;

ArrayPartition::ArrayPartition(ArrayRegistration &registration) {
    _numberOfDimensions = registration.getNumberOfDimensions();
    if (registration._localArrayRefInGrid[0]->getNumberOfDimensions()
            > _numberOfDimensions)
        _numberOfDimensions = registration._localArrayRefInGrid[0]
            ->getNumberOfDimensions();
    _startProcID = registration.getStartProcID();
    _numberOfBytesPerElement = registration._localArrayRefInGrid[0]->getNumberOfBytesPerElement();
    _nProcsInGrid = 1;
    uint64_t gridID[7];
    for (unsigned iDim = 0; iDim < _numberOfDimensions; ++iDim) {
        _nProcsAlongDim[iDim] = registration.getSize(iDim);
        _nProcsInGrid *= _nProcsAlongDim[iDim];
        gridID[iDim] = 0;
        _nodeAlongDim[iDim] = new uint64_t[_nProcsAlongDim[iDim]];
        if (_nodeAlongDim[iDim] == NULL)
            throw ENotEnoughMemory();
    }
    for (unsigned iDim = _numberOfDimensions; iDim < 7; ++iDim) {
        _nProcsAlongDim[iDim] = 1;
        gridID[iDim] = 0;
        _nodeAlongDim[iDim] = NULL;
    }

    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        for (uint64_t i = 0; i < _nProcsAlongDim[iDim]; i++) {
            gridID[iDim] = i;
            uint64_t offset = registration.getProcID(gridID)
                    - registration.getStartProcID();
            uint64_t partWidth = registration._localArrayRefInGrid[offset]
                    ->getElementCount(iDim);
            if (i == 0)
                _nodeAlongDim[iDim][i] = partWidth;
            else
                _nodeAlongDim[iDim][i] = _nodeAlongDim[iDim][i - 1] + partWidth;
        }
        gridID[iDim] = 0;
    }

    if (!combineLocalArrays(registration, registration._localArrayRefInGrid))
        throw EInvalidArgument();
}

ArrayShape::ElementType ArrayPartition::getElementType() const {
    return _elementType;
}

ArrayPartition::ArrayPartition(const ArrayPartition& partition) {
    _numberOfBytesPerElement = partition._numberOfBytesPerElement;
    _numberOfDimensions = partition._numberOfDimensions;
    _startProcID = partition._startProcID;
    _nProcsInGrid = partition._nProcsInGrid;
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        _nProcsAlongDim[iDim] = partition._nProcsAlongDim[iDim];
        _nodeAlongDim[iDim] = new uint64_t[_nProcsAlongDim[iDim]];
        if (_nodeAlongDim[iDim] == NULL)
            throw ENotEnoughMemory();
        for (uint64_t i = 0; i < _nProcsAlongDim[iDim]; i++) {
            _nodeAlongDim[iDim][i] = partition._nodeAlongDim[iDim][i];
        }
    }
}

ArrayPartition::~ArrayPartition() {
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        delete[] _nodeAlongDim[iDim];
    }
}

bool ArrayPartition::combineLocalArrays(Grid &grid, ArrayShape **localArrayRef) {
    uint64_t numberOfBytesPerElement =
            localArrayRef[0]->getNumberOfBytesPerElement();
    uint64_t nProcsInGrid = grid.getSize(ALL_DIMS);
    uint64_t startProcID = grid.getStartProcID();
    uint64_t gridID[7] = {0, 0, 0, 0, 0, 0, 0};
    for (uint64_t procID = startProcID; procID < startProcID + nProcsInGrid;
            procID++) {
        if (localArrayRef[procID - startProcID]->getNumberOfBytesPerElement()
                != numberOfBytesPerElement)
            return false;
        if (localArrayRef[procID - startProcID]->getNumberOfDimensions()
                != _numberOfDimensions)
            return false;
        grid.getIndex(procID, gridID);
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            uint64_t partWidth =
                    localArrayRef[procID - startProcID]->getElementCount(iDim);
            if (gridID[iDim] == 0) {
                if (partWidth != _nodeAlongDim[iDim][gridID[iDim]])
                    return false;
            } else {
                if (partWidth
                        != _nodeAlongDim[iDim][gridID[iDim]]
                        - _nodeAlongDim[iDim][gridID[iDim] - 1])
                    return false;
            }
        }
    }
    return true;
}

uint64_t ArrayPartition::getProcCount(const unsigned iDim) {
    if (iDim == ALL_DIMS) {
        return _nProcsInGrid;
    } else if (iDim < _numberOfDimensions)
        return _nProcsAlongDim[iDim];
    else
        return 1;
}

uint64_t ArrayPartition::getElementCount(const unsigned iDim) {
    if (iDim == ALL_DIMS) {
        uint64_t result = 1;
        for (unsigned jDim = 0; jDim < _numberOfDimensions; jDim++)
            result *= _nodeAlongDim[jDim][_nProcsAlongDim[jDim] - 1];
        return result;
    } else if (iDim < _numberOfDimensions)
        return _nodeAlongDim[iDim][_nProcsAlongDim[iDim] - 1];
    else
        return 1;
}

uint64_t ArrayPartition::getIProc(const uint64_t elementIndex[]) {
    uint64_t position[7];
    getPosition(elementIndex, position, 0, 0, NULL);
    return getIProcFromPosition(position);
}

uint64_t ArrayPartition::getProcID(const uint64_t elementIndex[]) {
    return getIProc(elementIndex) + _startProcID;
}

uint64_t ArrayPartition::getIProcFromPosition(const uint64_t position[]) {
    uint64_t iProc = position[0];
    for (unsigned iDim = 1; iDim < _numberOfDimensions; iDim++) {
        iProc *= _nProcsAlongDim[iDim];
        iProc += position[iDim];
    }
    return iProc;
}

uint64_t ArrayPartition::getProcIDFromPosition(const uint64_t position[]) {
    return getIProcFromPosition(position) + _startProcID;
}

void ArrayPartition::getPositionFromIProc(const uint64_t iProc,
        uint64_t position[]) {
    if (iProc > _nProcsInGrid)
        throw EInvalidArgument();
    uint64_t tempIProc = iProc;
    for (int iDim = _numberOfDimensions - 1; iDim > 0; iDim--) {
        position[iDim] = tempIProc % _nProcsAlongDim[iDim];
        tempIProc /= _nProcsAlongDim[iDim];
    }
    position[0] = tempIProc;
}

void ArrayPartition::getPositionFromProcID(const uint64_t procID,
        uint64_t position[]) {
    //if (procID < 0)
    //    throw EInvalidArgument();
    getPositionFromIProc(procID - _startProcID, position);
}

uint64_t ArrayPartition::getPosition(const unsigned iDim,
        const uint64_t index,
        const std::string requestID, const unsigned nVars,
        const uint64_t valueOfVar[]) {
    if (iDim >= _numberOfDimensions)
        throw EInvalidArgument();
    if (index >= _nodeAlongDim[iDim][_nProcsAlongDim[iDim] - 1])
        throw EInvalidElementPosition(requestID, iDim, nVars, index, valueOfVar, _nodeAlongDim[iDim][_nProcsAlongDim[iDim] - 1]);
    uint64_t left = 0;
    uint64_t right = _nProcsAlongDim[iDim] - 1;
    while (right > left) {
        // divide [left,right] to [left, middle] and [middle + 1, right]
        uint64_t middle = (left + right) >> 1;
        if (_nodeAlongDim[iDim][middle] > index)
            right = middle;
        else
            left = middle + 1;
    }
    return left;
}

void ArrayPartition::getPosition(const uint64_t elementIndex[],
        uint64_t position[],
        const std::string requestID, const unsigned nVars,
        const uint64_t valueOfVar[]) {
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++)
        position[iDim] = getPosition(iDim, elementIndex[iDim],
                requestID, nVars, valueOfVar);
}

uint64_t ArrayPartition::getNode(const unsigned iDim, const uint64_t position) {
    if (iDim >= _numberOfDimensions)
        throw EInvalidArgument();
    if (position > _nProcsAlongDim[iDim])
        throw EInvalidArgument();
    if (position == 0)
        return 0;
    return _nodeAlongDim[iDim][position - 1];
}

