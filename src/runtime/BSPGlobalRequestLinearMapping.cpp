/*
 * BSPGlobalRequestLinearMapping.cpp
 *
 *  Created on: 2014-8-12
 *      Author: junfeng
 */

#include "BSPGlobalRequestLinearMapping.hpp"
#include <stdint.h>

using namespace BSP;

GlobalRequestLinearMapping::GlobalRequestLinearMapping(
        ArrayPartition &partition, const unsigned numberOfVariables,
        const int64_t *matrix, const uint64_t *variableStart,
        const uint64_t *variableEnd, const std::string requestID) :
GlobalRequest(partition) {
    // set the basic attributes
    setRequestID(requestID);
    _numberOfVariables = numberOfVariables;
    if (_numberOfVariables == 0)
        throw EInvalidArgument();

    // set the matrix and the variable ranges
    unsigned iEntry = 0;
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        for (unsigned iVar = 0; iVar <= _numberOfVariables; iVar++) {
            _coefMatrix[iDim][iVar] = matrix[iEntry++];
        }
    }
    for (unsigned iVar = 0; iVar < _numberOfVariables; iVar++) {
        _variableStart[iVar] = variableStart[iVar];
        _variableEnd[iVar] = variableEnd[iVar];
        if (_variableStart[iVar] > _variableEnd[iVar])
            throw EInvalidArgument();
    }

    // get the domain of the array
    uint64_t domain[7];
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        domain[iDim] = getElementCount(iDim);
    }

    // allocate some auxiliary arrays
    uint64_t * variableStartOfProc[7];
    uint64_t * variableEndOfProc[7];
    for (unsigned iVar = 0; iVar < _numberOfVariables; iVar++) {
        variableStartOfProc[iVar] = new uint64_t[_nProcsInGrid];
        variableEndOfProc[iVar] = new uint64_t[_nProcsInGrid];
        if (variableStartOfProc[iVar] == NULL || variableEndOfProc[iVar] == NULL)
            throw ENotEnoughMemory();
        // initialize the start and end of variable for each proc
        for (uint64_t iProc = 0; iProc < _nProcsInGrid; iProc++) {
            variableStartOfProc[iVar][iProc] = _variableEnd[iVar];
            variableEndOfProc[iVar][iProc] = _variableStart[iVar];
        }
    }

    // iterate through the variable combinations
    uint64_t elementIndex[7], combination[7], position[7];
    for (unsigned iVar = 0; iVar < _numberOfVariables; iVar++) {
        combination[iVar] = _variableStart[iVar];
    }
    while (combination[0] <= _variableEnd[0]) {
        // compute the element index using matrix vector multiplication
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            int64_t indexAlongThisDim = _coefMatrix[iDim][_numberOfVariables];
            for (unsigned iVar = 0; iVar < _numberOfVariables; iVar++) {
                indexAlongThisDim += _coefMatrix[iDim][iVar]
                        * combination[iVar];
            }
            if (indexAlongThisDim < 0
                    || indexAlongThisDim >= (int64_t) domain[iDim])
                throw EInvalidElementPosition(getRequestID(), iDim, 
                        _numberOfVariables, indexAlongThisDim, combination, domain[iDim]);
            elementIndex[iDim] = (uint64_t) indexAlongThisDim;
        }

        // obtain the owner position form the element index
        getPosition(elementIndex, position, getRequestID(), _numberOfVariables,
                combination);
        uint64_t iProc = getIProcFromPosition(position);

        // increase the counter and the variable start/end at this position
        _dataCount[iProc]++;
        for (unsigned iVar = 0; iVar < _numberOfVariables; iVar++) {
            if (variableStartOfProc[iVar][iProc] > combination[iVar])
                variableStartOfProc[iVar][iProc] = combination[iVar];
            if (variableEndOfProc[iVar][iProc] < combination[iVar])
                variableEndOfProc[iVar][iProc] = combination[iVar];
        }

        // get next combination
        for (int iVar = _numberOfVariables - 1; iVar >= 0; iVar--) {
            combination[iVar]++;
            if (iVar == 0 || combination[iVar] <= _variableEnd[iVar])
                break;
            combination[iVar] = _variableStart[iVar];
        }
    }

    // compute the size of each index list
    uint64_t lengthOfIndexList = 3
            + _numberOfDimensions * (_numberOfVariables + 1)
            + 2 * _numberOfVariables;

    // generate the index list and allocate the data list for each proc
    for (uint64_t iProc = 0; iProc < _nProcsInGrid; iProc++) {
        // skip the procs without requests
        if (_dataCount[iProc] == 0)
            continue;

        // get position
        getPositionFromProcID(iProc + _startProcID, position);

        // allocate index and data list
        _indexLength[iProc] = lengthOfIndexList;
        allocateForProc(iProc);

        // compute nExpectedFailures
        uint64_t nCombinations = 1;
        for (unsigned iVar = 0; iVar < _numberOfVariables; iVar++) {
            nCombinations *= variableEndOfProc[iVar][iProc]
                    - variableStartOfProc[iVar][iProc] + 1;
        }
        if (nCombinations == 0)
            throw EInvalidArgument();
        uint64_t nExpectedFailures = nCombinations - _dataCount[iProc];

        // fill in the index list
        uint64_t *currentList = _indexList[iProc];
        int64_t *si = (int64_t *) currentList + 3;
        currentList[0] = _dataCount[iProc] * _numberOfBytesPerElement;
        currentList[1] = nExpectedFailures;
        currentList[2] = _numberOfVariables;
        iEntry = 0;
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            for (unsigned iVar = 0; iVar < _numberOfVariables; iVar++) {
                si[iEntry++] = _coefMatrix[iDim][iVar];
            }
            si[iEntry++] = _coefMatrix[iDim][_numberOfVariables]
                    - getNode(iDim, position[iDim]);
        }

        // fill in the variable starts and ends
        currentList += 3 + _numberOfDimensions * (_numberOfVariables + 1);
        for (unsigned iVar = 0; iVar < _numberOfVariables; iVar++) {
            currentList[iVar << 1] = variableStartOfProc[iVar][iProc];
            currentList[(iVar << 1) + 1] = variableEndOfProc[iVar][iProc];
        }
    }

    // release auxiliary arrays
    for (unsigned iVar = 0; iVar < _numberOfVariables; iVar++) {
        delete[] variableStartOfProc[iVar];
        delete[] variableEndOfProc[iVar];
    }
}

GlobalRequestLinearMapping::~GlobalRequestLinearMapping() {
}

void GlobalRequestLinearMapping::getData(const uint64_t numberOfBytesPerElement,
        const uint64_t nData, char *data) {
    if (numberOfBytesPerElement
            != _numberOfBytesPerElement || nData < _nData || data == NULL)
        throw EInvalidArgument();

    // allocate auxiliary array
    uint64_t *dataIndexAtProc = new uint64_t[_nProcsInGrid];
    if (dataIndexAtProc == NULL)
        throw ENotEnoughMemory();
    for (uint64_t iProc = 0; iProc < _nProcsInGrid; iProc++) {
        dataIndexAtProc[iProc] = 0;
    }

    // iterate through the variable combinations
    uint64_t dataIndex = 0;
    uint64_t elementIndex[7], combination[7], position[7];
    for (unsigned iVar = 0; iVar < _numberOfVariables; iVar++) {
        combination[iVar] = _variableStart[iVar];
    }
    while (combination[0] <= _variableEnd[0]) {
        // compute the element index using matrix vector multiplication
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            int64_t indexAlongThisDim = _coefMatrix[iDim][_numberOfVariables];
            for (unsigned iVar = 0; iVar < _numberOfVariables; iVar++) {
                indexAlongThisDim += _coefMatrix[iDim][iVar]
                        * combination[iVar];
            }
            elementIndex[iDim] = (uint64_t) indexAlongThisDim;
        }

        // obtain the owner position form the element index
        getPosition(elementIndex, position, getRequestID(), _numberOfVariables,
                combination);
        uint64_t iProc = getIProcFromPosition(position);

        // get data from this position
        const char *src = _dataList[iProc]
                + _numberOfBytesPerElement * dataIndexAtProc[iProc]++;
        char *dst = data + _numberOfBytesPerElement * dataIndex++;
        for (uint64_t iByte = 0; iByte < _numberOfBytesPerElement; iByte++)
            dst[iByte] = src[iByte];

        // get next combination
        for (int iVar = _numberOfVariables - 1; iVar >= 0; iVar--) {
            combination[iVar]++;
            if (iVar == 0 || combination[iVar] <= _variableEnd[iVar])
                break;
            combination[iVar] = _variableStart[iVar];
        }
    }
    delete[] dataIndexAtProc;
}

void GlobalRequestLinearMapping::setData(const uint64_t numberOfBytesPerElement,
        const uint64_t nData, const char *data) {
    if (numberOfBytesPerElement
            != _numberOfBytesPerElement || nData < _nData || data == NULL)
        throw EInvalidArgument();

    // allocate auxiliary array
    uint64_t *dataIndexAtProc = new uint64_t[_nProcsInGrid];
    if (dataIndexAtProc == NULL)
        throw ENotEnoughMemory();
    for (uint64_t iProc = 0; iProc < _nProcsInGrid; iProc++) {
        dataIndexAtProc[iProc] = 0;
    }

    // iterate through the variable combinations
    uint64_t dataIndex = 0;
    uint64_t elementIndex[7], combination[7], position[7];
    for (unsigned iVar = 0; iVar < _numberOfVariables; iVar++) {
        combination[iVar] = _variableStart[iVar];
    }
    while (combination[0] <= _variableEnd[0]) {
        // compute the element index using matrix vector multiplication
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            int64_t indexAlongThisDim = _coefMatrix[iDim][_numberOfVariables];
            for (unsigned iVar = 0; iVar < _numberOfVariables; iVar++) {
                indexAlongThisDim += _coefMatrix[iDim][iVar]
                        * combination[iVar];
            }
            elementIndex[iDim] = (uint64_t) indexAlongThisDim;
        }

        // obtain the owner position form the element index
        getPosition(elementIndex, position, getRequestID(), _numberOfVariables,
                combination);
        uint64_t iProc = getIProcFromPosition(position);

        // get data from this position
        char *dst = _dataList[iProc]
                + _numberOfBytesPerElement * dataIndexAtProc[iProc]++;
        const char *src = data + _numberOfBytesPerElement * dataIndex++;
        for (uint64_t iByte = 0; iByte < _numberOfBytesPerElement; iByte++)
            dst[iByte] = src[iByte];

        // get next combination
        for (int iVar = _numberOfVariables - 1; iVar >= 0; iVar--) {
            combination[iVar]++;
            if (iVar == 0 || combination[iVar] <= _variableEnd[iVar])
                break;
            combination[iVar] = _variableStart[iVar];
        }
    }
    delete[] dataIndexAtProc;
}


