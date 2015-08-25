/*
 * GlobalRequestRegionLinear.cpp
 *
 *  Created on: 2014-8-5
 *      Author: junfeng
 */

#include "BSPGlobalRequestRegionSequence.hpp"

using namespace BSP;

GlobalRequestRegionSequence::GlobalRequestRegionSequence(
        ArrayPartition &partition, IndexSetRegionSequence &indexSet, const std::string requestID) :
GlobalRequestRegion(partition) {
    setRequestID(requestID);
    
    // allocate the second part of auxiliary arrays
    _nRegions = indexSet.getNumberOfRegions();
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        _nComponentsAlongDim[iDim] = _nRegions;
    }
    allocateComponents();

    // iterate through the requests to count the indices and the data
    const uint64_t *currentLower = indexSet._lowerIndexList;
    const uint64_t *currentUpper = indexSet._upperIndexList;
    uint64_t combination[7], lowerLocalOffset[7], upperLocalOffset[7];
    for (uint64_t iRequest = 0; iRequest < _nRegions; iRequest++) {
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            if (currentLower[iDim] > currentUpper[iDim])
                throw EInvalidArgument();
            uint64_t lowerPosition = partition.getPosition(iDim,
                    currentLower[iDim], getRequestID(), _numberOfDimensions,
                    currentLower);
            uint64_t upperPosition = partition.getPosition(iDim,
                    currentUpper[iDim], getRequestID(), _numberOfDimensions,
                    currentUpper);
            _lowerOwnerPositionAlongDim[iDim][iRequest] = lowerPosition;
            _upperOwnerPositionAlongDim[iDim][iRequest] = upperPosition;
            _lowerOffsetInOwnerAlongDim[iDim][iRequest] = currentLower[iDim]
                    - getNode(iDim, lowerPosition);
            _upperOffsetInOwnerAlongDim[iDim][iRequest] = currentUpper[iDim]
                    - getNode(iDim, upperPosition);
            combination[iDim] = lowerPosition;
        }
        currentLower += _numberOfDimensions;
        currentUpper += _numberOfDimensions;

        while (combination[0] <= _upperOwnerPositionAlongDim[0][iRequest]) {
            // compute owner, local offset and data count
            for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
                if (combination[iDim]
                        == _lowerOwnerPositionAlongDim[iDim][iRequest])
                    lowerLocalOffset[iDim] =
                        _lowerOffsetInOwnerAlongDim[iDim][iRequest];
                else
                    lowerLocalOffset[iDim] = 0;
                if (combination[iDim]
                        == _upperOwnerPositionAlongDim[iDim][iRequest])
                    upperLocalOffset[iDim] =
                        _upperOffsetInOwnerAlongDim[iDim][iRequest];
                else
                    upperLocalOffset[iDim] = getNode(iDim,
                        combination[iDim] + 1)
                    - getNode(iDim, combination[iDim]) - 1;
            }
            uint64_t ownerIProc = partition.getProcIDFromPosition(combination)
                    - _startProcID;
            uint64_t dataCount = 1;
            for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
                dataCount *= upperLocalOffset[iDim] - lowerLocalOffset[iDim]
                        + 1;
            }

            // update indexLength and dataCount
            if (_indexLength[ownerIProc] == 0) {
                _indexLength[ownerIProc] = 2;
            }
            _indexLength[ownerIProc] += 1 + _numberOfDimensions;
            _dataCount[ownerIProc] += dataCount;

            // get next combination
            for (int iDim = _numberOfDimensions - 1; iDim >= 0; iDim--) {
                combination[iDim]++;
                if (iDim == 0
                        || combination[iDim]
                        <= _upperOwnerPositionAlongDim[iDim][iRequest])
                    break;
                combination[iDim] = _lowerOwnerPositionAlongDim[iDim][iRequest];
            }
        }
    }

    // allocate the index list
    for (uint64_t iProc = 0; iProc < _nProcsInGrid; iProc++) {
        if (_dataCount[iProc] > 0) {
            allocateForProc(iProc);
            _indexList[iProc][0] = _dataCount[iProc] * _numberOfBytesPerElement;
            _indexList[iProc][1] = 0;
        }
    }

    // iterate through the requests to store the indices
    for (uint64_t iRequest = 0; iRequest < _nRegions; iRequest++) {
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            combination[iDim] = _lowerOwnerPositionAlongDim[iDim][iRequest];
        }

        while (combination[0] <= _upperOwnerPositionAlongDim[0][iRequest]) {
            // compute owner, local offset and data count
            for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
                if (combination[iDim]
                        == _lowerOwnerPositionAlongDim[iDim][iRequest])
                    lowerLocalOffset[iDim] =
                        _lowerOffsetInOwnerAlongDim[iDim][iRequest];
                else
                    lowerLocalOffset[iDim] = 0;
                if (combination[iDim]
                        == _upperOwnerPositionAlongDim[iDim][iRequest])
                    upperLocalOffset[iDim] =
                        _upperOffsetInOwnerAlongDim[iDim][iRequest];
                else
                    upperLocalOffset[iDim] = getNode(iDim,
                        combination[iDim] + 1)
                    - getNode(iDim, combination[iDim]) - 1;
            }
            uint64_t ownerIProc = partition.getProcIDFromPosition(combination)
                    - _startProcID;
            uint64_t offsetInOwner = lowerLocalOffset[0];
            for (unsigned iDim = 1; iDim < _numberOfDimensions; iDim++) {
                offsetInOwner *= getNode(iDim, combination[iDim] + 1)
                        - getNode(iDim, combination[iDim]);
                offsetInOwner += lowerLocalOffset[iDim];
            }

            // store indices
            uint64_t *currentIndex = _indexList[ownerIProc] + 2
                    + _indexList[ownerIProc][1] * (1 + _numberOfDimensions);
            _indexList[ownerIProc][1]++;
            currentIndex[0] = offsetInOwner;
            currentIndex++;
            for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
                currentIndex[iDim] = upperLocalOffset[iDim]
                        - lowerLocalOffset[iDim] + 1;
            }

            // get next combination
            for (int iDim = _numberOfDimensions - 1; iDim >= 0; iDim--) {
                combination[iDim]++;
                if (iDim == 0
                        || combination[iDim]
                        <= _upperOwnerPositionAlongDim[iDim][iRequest])
                    break;
                combination[iDim] = _lowerOwnerPositionAlongDim[iDim][iRequest];
            }
        }
    }
    
    if (_nData != indexSet.getNumberOfIndices())
        throw ECorruptedRuntime();
}

GlobalRequestRegionSequence::~GlobalRequestRegionSequence() {
}

void GlobalRequestRegionSequence::getData(
        const uint64_t numberOfBytesPerElement, const uint64_t nData,
        char *data) {
    if (nData < _nData)
        throw EClientArrayTooSmall(getRequestID(), nData, _nData);
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

    // iterate through the regions
    uint64_t iData = 0;
    for (uint64_t iRegion = 0; iRegion < _nRegions; iRegion++) {
        uint64_t regionWidth[7];
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            regionWidth[iDim] = getRegionWidth(iDim, iRegion);
        }
        // iterate through the points within this region
        uint64_t combination[7], position[7], localOffset[7], localWidth[7];
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            combination[iDim] = 0;
            position[iDim] = _lowerOwnerPositionAlongDim[iDim][iRegion];
            localOffset[iDim] = _lowerOffsetInOwnerAlongDim[iDim][iRegion];
            localWidth[iDim] = getNode(iDim, position[iDim] + 1)
                    - getNode(iDim, position[iDim]);
        }
        while (combination[0] < regionWidth[0]) {
            // compute iProc
            uint64_t iProc = getIProcFromPosition(position);

            // copy data
            const char *src = _dataList[iProc]
                    + _numberOfBytesPerElement * dataIndexAtProc[iProc]++;
            char *dst = data + _numberOfBytesPerElement * iData++;
            for (uint64_t iByte = 0; iByte < _numberOfBytesPerElement; iByte++)
                dst[iByte] = src[iByte];

            // get next combination
            for (int iDim = _numberOfDimensions - 1; iDim >= 0; iDim--) {
                combination[iDim]++;
                if (combination[iDim] < regionWidth[iDim]) {
                    localOffset[iDim]++;
                    if (localOffset[iDim] >= localWidth[iDim]) {
                        position[iDim]++;
                        localOffset[iDim] = 0;
                        localWidth[iDim] = getNode(iDim, position[iDim] + 1)
                                - getNode(iDim, position[iDim]);
                    }
                }

                if (iDim == 0 || combination[iDim] < regionWidth[iDim])
                    break;

                combination[iDim] = 0;
                position[iDim] = _lowerOwnerPositionAlongDim[iDim][iRegion];
                localOffset[iDim] = _lowerOffsetInOwnerAlongDim[iDim][iRegion];
                localWidth[iDim] = getNode(iDim, position[iDim] + 1)
                        - getNode(iDim, position[iDim]);
            }
        }
    }

    delete[] dataIndexAtProc;
}

void GlobalRequestRegionSequence::setData(
        const uint64_t numberOfBytesPerElement, const uint64_t nData,
        const char *data) {
    if (nData < _nData)
        throw EClientArrayTooSmall(getRequestID(), nData, _nData);
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

    // iterate through the regions
    uint64_t iData = 0;
    for (uint64_t iRegion = 0; iRegion < _nRegions; iRegion++) {
        uint64_t regionWidth[7];
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            regionWidth[iDim] = getRegionWidth(iDim, iRegion);
        }
        // iterate through the points within this region
        uint64_t combination[7], position[7], localOffset[7], localWidth[7];
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            combination[iDim] = 0;
            position[iDim] = _lowerOwnerPositionAlongDim[iDim][iRegion];
            localOffset[iDim] = _lowerOffsetInOwnerAlongDim[iDim][iRegion];
            localWidth[iDim] = getNode(iDim, position[iDim] + 1)
                    - getNode(iDim, position[iDim]);
        }
        while (combination[0] < regionWidth[0]) {
            // compute iProc
            uint64_t iProc = getIProcFromPosition(position);

            // copy data
            char *dst = _dataList[iProc]
                    + _numberOfBytesPerElement * dataIndexAtProc[iProc]++;
            const char *src = data + _numberOfBytesPerElement * iData++;
            for (uint64_t iByte = 0; iByte < _numberOfBytesPerElement; iByte++)
                dst[iByte] = src[iByte];

            // get next combination
            for (int iDim = _numberOfDimensions - 1; iDim >= 0; iDim--) {
                combination[iDim]++;
                if (combination[iDim] < regionWidth[iDim]) {
                    localOffset[iDim]++;
                    if (localOffset[iDim] >= localWidth[iDim]) {
                        position[iDim]++;
                        localOffset[iDim] = 0;
                        localWidth[iDim] = getNode(iDim, position[iDim] + 1)
                                - getNode(iDim, position[iDim]);
                    }
                }

                if (iDim == 0 || combination[iDim] < regionWidth[iDim])
                    break;

                combination[iDim] = 0;
                position[iDim] = _lowerOwnerPositionAlongDim[iDim][iRegion];
                localOffset[iDim] = _lowerOffsetInOwnerAlongDim[iDim][iRegion];
                localWidth[iDim] = getNode(iDim, position[iDim] + 1)
                        - getNode(iDim, position[iDim]);
            }
        }
    }

    delete[] dataIndexAtProc;
}


