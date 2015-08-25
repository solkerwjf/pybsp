/*
 * GlobalRequestPointTensor.cpp
 *
 *  Created on: 2014-8-5
 *      Author: junfeng
 */

#include "BSPGlobalRequestPointTensor.hpp"

using namespace BSP;

GlobalRequestPointTensor::GlobalRequestPointTensor(ArrayPartition &partition,
        IndexSetPointTensor &indexSet, const std::string requestID) :
GlobalRequest(partition) {
    setRequestID(requestID);
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        _nComponentsAlongDim[iDim] = indexSet._numberOfComponentsAlongDim[iDim];
        if (_nComponentsAlongDim[iDim] == 0)
            throw EInvalidArgument();
        _ownerPositionAlongDim[iDim] = new uint64_t[_nComponentsAlongDim[iDim]];
        _offsetInOwnerAlongDim[iDim] = new uint64_t[_nComponentsAlongDim[iDim]];
        if (_ownerPositionAlongDim[iDim] == NULL
                || _offsetInOwnerAlongDim[iDim] == NULL)
            throw ENotEnoughMemory();
        for (uint64_t iComponent = 0; iComponent < _nComponentsAlongDim[iDim];
                iComponent++) {
            _ownerPositionAlongDim[iDim][iComponent] = getPosition(iDim,
                    indexSet._componentAlongDim[iDim][iComponent],
                    getRequestID(), 0, NULL);
        }
    }

    // allocate auxiliary arrays
    uint64_t * nComponentsAtPositionAlongDim[7];
    uint64_t **localOffsetAtPositionAlongDim[7];
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        uint64_t nProcsAlongThisDim = getProcCount(iDim);
        nComponentsAtPositionAlongDim[iDim] = new uint64_t[nProcsAlongThisDim];
        localOffsetAtPositionAlongDim[iDim] =
                new uint64_t *[nProcsAlongThisDim];
        if (nComponentsAtPositionAlongDim[iDim] == NULL
                || localOffsetAtPositionAlongDim[iDim] == NULL)
            throw ENotEnoughMemory();
        for (uint64_t iProc = 0; iProc < nProcsAlongThisDim; iProc++) {
            nComponentsAtPositionAlongDim[iDim][iProc] = 0;
            localOffsetAtPositionAlongDim[iDim][iProc] = NULL;
        }
    }

    // iterate through the components to convert them into owner positions and offsets in owners
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        for (uint64_t iComponent = 0; iComponent < _nComponentsAlongDim[iDim];
                iComponent++) {
            _ownerPositionAlongDim[iDim][iComponent] = getPosition(iDim,
                    indexSet._componentAlongDim[iDim][iComponent],
                    getRequestID(), 0, NULL);
            _offsetInOwnerAlongDim[iDim][iComponent] =
                    indexSet._componentAlongDim[iDim][iComponent]
                    - getNode(iDim,
                    _ownerPositionAlongDim[iDim][iComponent]);
            nComponentsAtPositionAlongDim[iDim][_ownerPositionAlongDim[iDim][iComponent]]++;
        }
    }

    // allocate more of auxiliary arrays
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        uint64_t nProcsAlongThisDim = getProcCount(iDim);
        for (uint64_t iProc = 0; iProc < nProcsAlongThisDim; iProc++) {
            if (nComponentsAtPositionAlongDim[iDim][iProc] == 0)
                continue;
            localOffsetAtPositionAlongDim[iDim][iProc] =
                    new uint64_t[nComponentsAtPositionAlongDim[iDim][iProc]];
            if (localOffsetAtPositionAlongDim[iDim][iProc] == NULL)
                throw ENotEnoughMemory();
        }
    }

    // iterate through the components to fill in localOffsetAtPositionAlongDim
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        uint64_t nProcsAlongThisDim = getProcCount(iDim);
        for (uint64_t iProc = 0; iProc < nProcsAlongThisDim; iProc++) {
            nComponentsAtPositionAlongDim[iDim][iProc] = 0;
        }

        for (uint64_t iComponent = 0; iComponent < _nComponentsAlongDim[iDim];
                iComponent++) {
            uint64_t iProc = _ownerPositionAlongDim[iDim][iComponent];
            uint64_t iLocalComponent =
                    nComponentsAtPositionAlongDim[iDim][iProc]++;
            localOffsetAtPositionAlongDim[iDim][iProc][iLocalComponent] =
                    _offsetInOwnerAlongDim[iDim][iComponent];
        }
    }

    // iterate through the procs in grid to generate their request list and data list
    for (uint64_t iProc = 0; iProc < _nProcsInGrid; iProc++) {
        // decompose the iProc to position
        uint64_t position[7];
        getPositionFromIProc(iProc, position);

        // check whether at this position holds no requests
        bool empty = false;
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            if (nComponentsAtPositionAlongDim[iDim][position[iDim]] == 0) {
                empty = true;
                break;
            }
        }

        // skip empty positions
        if (empty)
            continue;

        // compute index length and data count
        _dataCount[iProc] = 1;
        _indexLength[iProc] = 1;
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            _indexLength[iProc] += 1
                    + nComponentsAtPositionAlongDim[iDim][position[iDim]];
            _dataCount[iProc] *=
                    nComponentsAtPositionAlongDim[iDim][position[iDim]];
        }

        // allocate the index-list and the data-list
        allocateForProc(iProc);

        // fill in the index-list
        uint64_t *currentIndex = _indexList[iProc] + _numberOfDimensions + 1;
        _indexList[iProc][0] = _dataCount[iProc] * _numberOfBytesPerElement;
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            _indexList[iProc][iDim + 1] =
                    nComponentsAtPositionAlongDim[iDim][position[iDim]];
            for (uint64_t iComponent = 0;
                    iComponent
                    < nComponentsAtPositionAlongDim[iDim][position[iDim]];
                    iComponent++) {
                currentIndex[iComponent] =
                        localOffsetAtPositionAlongDim[iDim][position[iDim]][iComponent];
            }
            currentIndex += nComponentsAtPositionAlongDim[iDim][position[iDim]];
        }
    }

    // delete temporarily auxiliary arrays
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        uint64_t nProcsAlongThisDim = getProcCount(iDim);
        for (uint64_t iProc = 0; iProc < nProcsAlongThisDim; iProc++) {
            if (nComponentsAtPositionAlongDim[iDim][iProc] == 0)
                continue;
            delete[] localOffsetAtPositionAlongDim[iDim][iProc];
        }
        delete[] nComponentsAtPositionAlongDim[iDim];
        delete[] localOffsetAtPositionAlongDim[iDim];
    }
    
    if (_nData != indexSet.getNumberOfIndices())
        throw ECorruptedRuntime();
}

GlobalRequestPointTensor::~GlobalRequestPointTensor() {
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        delete[] _ownerPositionAlongDim[iDim];
        delete[] _offsetInOwnerAlongDim[iDim];
    }
}

void GlobalRequestPointTensor::getData(const uint64_t numberOfBytesPerElement,
        const uint64_t nData, char *data) {
    if (nData < _nData)
        throw EClientArrayTooSmall(getRequestID(), nData, _nData);
    if (numberOfBytesPerElement
            != _numberOfBytesPerElement || nData < _nData || data == NULL)
        throw EInvalidArgument();
    uint64_t *nDataInProc = new uint64_t[_nProcsInGrid];
    if (nDataInProc == NULL)
        throw ENotEnoughMemory();
    for (uint64_t iProc = 0; iProc < _nProcsInGrid; iProc++)
        nDataInProc[iProc] = 0;

    // iterate through the combinations
    uint64_t iData = 0;
    uint64_t combination[7], position[7];
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        combination[iDim] = 0;
    }
    while (combination[0] < _nComponentsAlongDim[0]) {
        // set position
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            position[iDim] = _ownerPositionAlongDim[iDim][combination[iDim]];
        }

        // get iProc from position
        uint64_t iProc = getIProcFromPosition(position);

        // copy data
        const char *src = _dataList[iProc]
                + _numberOfBytesPerElement * nDataInProc[iProc]++;
        char *dst = data + _numberOfBytesPerElement * iData++;
        for (uint64_t iByte = 0; iByte < _numberOfBytesPerElement; iByte++) {
            dst[iByte] = src[iByte];
        }

        // get next combination
        for (int iDim = _numberOfDimensions - 1; iDim >= 0; iDim--) {
            combination[iDim]++;
            if (iDim == 0 || combination[iDim] < _nComponentsAlongDim[iDim])
                break;
            combination[iDim] = 0;
        }
    }
    delete[] nDataInProc;
}

void GlobalRequestPointTensor::setData(const uint64_t numberOfBytesPerElement,
        const uint64_t nData, const char *data) {
    if (nData < _nData)
        throw EClientArrayTooSmall(getRequestID(), nData, _nData);
    if (numberOfBytesPerElement
            != _numberOfBytesPerElement || nData < _nData || data == NULL)
        throw EInvalidArgument();
    uint64_t *nDataInProc = new uint64_t[_nProcsInGrid];
    if (nDataInProc == NULL)
        throw ENotEnoughMemory();
    for (uint64_t iProc = 0; iProc < _nProcsInGrid; iProc++)
        nDataInProc[iProc] = 0;

    // iterate through the combinations
    uint64_t iData = 0;
    uint64_t combination[7], position[7];
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        combination[iDim] = 0;
    }
    while (combination[0] < _nComponentsAlongDim[0]) {
        // set position
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            position[iDim] = _ownerPositionAlongDim[iDim][combination[iDim]];
        }

        // get iProc from position
        uint64_t iProc = getIProcFromPosition(position);

        // copy data
        char *dst = _dataList[iProc]
                + _numberOfBytesPerElement * nDataInProc[iProc]++;
        const char *src = data + _numberOfBytesPerElement * iData++;
        for (uint64_t iByte = 0; iByte < _numberOfBytesPerElement; iByte++) {
            dst[iByte] = src[iByte];
        }

        // get next combination
        for (int iDim = _numberOfDimensions - 1; iDim >= 0; iDim--) {
            combination[iDim]++;
            if (iDim == 0 || combination[iDim] < _nComponentsAlongDim[iDim])
                break;
            combination[iDim] = 0;
        }
    }
    delete[] nDataInProc;
}


