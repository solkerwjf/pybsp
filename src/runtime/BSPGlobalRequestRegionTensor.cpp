/*
 * GlobalRequestRegionTensor.cpp
 *
 *  Created on: 2014-8-5
 *      Author: junfeng
 */

#include "BSPGlobalRequestRegionTensor.hpp"

using namespace BSP;

GlobalRequestRegionTensor::GlobalRequestRegionTensor(ArrayPartition &partition,
        IndexSetRegionTensor &indexSet, const std::string requestID) :
GlobalRequestRegion(partition) {
    setRequestID(requestID);
    _nRegions = 1;
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        _nComponentsAlongDim[iDim] = indexSet._numberOfComponentsAlongDim[iDim];
        _nRegions *= _nComponentsAlongDim[iDim];
    }
    allocateComponents();

    // allocate some auxiliary arrays
    uint64_t * nComponentsAtPositionAlongDim[7];
    uint64_t **lowerLocalOffsetAtPositionAlongDim[7];
    uint64_t **upperLocalOffsetAtPositionAlongDim[7];
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        uint64_t nProcsAlongThisDim = partition.getProcCount(iDim);
        nComponentsAtPositionAlongDim[iDim] = new uint64_t[nProcsAlongThisDim];
        lowerLocalOffsetAtPositionAlongDim[iDim] =
                new uint64_t *[nProcsAlongThisDim];
        upperLocalOffsetAtPositionAlongDim[iDim] =
                new uint64_t *[nProcsAlongThisDim];
        if (nComponentsAtPositionAlongDim[iDim] == NULL
                || lowerLocalOffsetAtPositionAlongDim[iDim] == NULL
                || upperLocalOffsetAtPositionAlongDim[iDim] == NULL)
            throw ENotEnoughMemory();
        for (uint64_t iProc = 0; iProc < nProcsAlongThisDim; iProc++) {
            nComponentsAtPositionAlongDim[iDim][iProc] = 0;
            lowerLocalOffsetAtPositionAlongDim[iDim][iProc] = NULL;
            upperLocalOffsetAtPositionAlongDim[iDim][iProc] = NULL;
        }
    }

    // iterate through the components to convert them into owner positions 
    // and offsets in owners
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        for (uint64_t iComponent = 0; iComponent < _nComponentsAlongDim[iDim];
                iComponent++) {
            if (indexSet._lowerComponentAlongDim[iDim][iComponent]
                    > indexSet._upperComponentAlongDim[iDim][iComponent])
                throw EInvalidArgument();
            _lowerOwnerPositionAlongDim[iDim][iComponent] =
                    partition.getPosition(iDim,
                    indexSet._lowerComponentAlongDim[iDim][iComponent],
                    getRequestID(), 0, NULL);
            _lowerOffsetInOwnerAlongDim[iDim][iComponent] =
                    indexSet._lowerComponentAlongDim[iDim][iComponent]
                    - getNode(iDim,
                    _lowerOwnerPositionAlongDim[iDim][iComponent]);
            _upperOwnerPositionAlongDim[iDim][iComponent] =
                    partition.getPosition(iDim,
                    indexSet._upperComponentAlongDim[iDim][iComponent],
                    getRequestID(), 0, NULL);
            _upperOffsetInOwnerAlongDim[iDim][iComponent] =
                    indexSet._upperComponentAlongDim[iDim][iComponent]
                    - getNode(iDim,
                    _upperOwnerPositionAlongDim[iDim][iComponent]);
            for (uint64_t iProc = _lowerOwnerPositionAlongDim[iDim][iComponent];
                    iProc <= _upperOwnerPositionAlongDim[iDim][iComponent];
                    iProc++)
                nComponentsAtPositionAlongDim[iDim][iProc]++;
        }
    }

    // allocate more of auxiliary arrays
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        uint64_t nProcsAlongThisDim = partition.getProcCount(iDim);
        for (uint64_t iProc = 0; iProc < nProcsAlongThisDim; iProc++) {
            if (nComponentsAtPositionAlongDim[iDim][iProc] == 0)
                continue;
            lowerLocalOffsetAtPositionAlongDim[iDim][iProc] =
                    new uint64_t[nComponentsAtPositionAlongDim[iDim][iProc]];
            upperLocalOffsetAtPositionAlongDim[iDim][iProc] =
                    new uint64_t[nComponentsAtPositionAlongDim[iDim][iProc]];
            if (lowerLocalOffsetAtPositionAlongDim[iDim][iProc] == NULL
                    || upperLocalOffsetAtPositionAlongDim[iDim][iProc] == NULL)
                throw ENotEnoughMemory();
        }
    }

    // iterate through the components to fill in localOffsetAtPositionAlongDim
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        uint64_t nProcsAlongThisDim = partition.getProcCount(iDim);
        for (uint64_t iProc = 0; iProc < nProcsAlongThisDim; iProc++) {
            nComponentsAtPositionAlongDim[iDim][iProc] = 0;
        }

        for (uint64_t iComponent = 0; iComponent < _nComponentsAlongDim[iDim];
                iComponent++) {
            for (uint64_t iProc = _lowerOwnerPositionAlongDim[iDim][iComponent];
                    iProc <= _upperOwnerPositionAlongDim[iDim][iComponent];
                    iProc++) {
                uint64_t iLocalComponent =
                        nComponentsAtPositionAlongDim[iDim][iProc]++;
                if (iProc == _lowerOwnerPositionAlongDim[iDim][iComponent]) {
                    lowerLocalOffsetAtPositionAlongDim[iDim][iProc][iLocalComponent] =
                            _lowerOffsetInOwnerAlongDim[iDim][iComponent];
                } else {
                    lowerLocalOffsetAtPositionAlongDim[iDim][iProc][iLocalComponent] =
                            0;
                }
                if (iProc == _upperOwnerPositionAlongDim[iDim][iComponent]) {
                    upperLocalOffsetAtPositionAlongDim[iDim][iProc][iLocalComponent] =
                            _upperOffsetInOwnerAlongDim[iDim][iComponent];
                } else {
                    upperLocalOffsetAtPositionAlongDim[iDim][iProc][iLocalComponent] =
                            getNode(iDim, iProc + 1) - getNode(iDim, iProc) - 1;
                }
            }
        }
    }

    // iterate through the procs in grid to generate their index list and data list
    for (uint64_t iProc = 0; iProc < _nProcsInGrid; iProc++) {
        // decompose the iProc to position
        uint64_t position[7];
        partition.getPositionFromIProc(iProc, position);

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
        uint64_t localElementCount = 1;
        _indexLength[iProc] = 1;
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            uint64_t nLocalComponents =
                    nComponentsAtPositionAlongDim[iDim][position[iDim]];
            _indexLength[iProc] += 1 + 2 * nLocalComponents;
            uint64_t localElementCountAlongDim = 0;
            for (uint64_t iLocalComponent = 0;
                    iLocalComponent < nLocalComponents;
                    ++iLocalComponent) {
                localElementCountAlongDim +=
                        upperLocalOffsetAtPositionAlongDim[iDim]
                        [position[iDim]][iLocalComponent]
                        - lowerLocalOffsetAtPositionAlongDim[iDim]
                        [position[iDim]][iLocalComponent]
                        + 1;
            }
            localElementCount *= localElementCountAlongDim;
        }
        _dataCount[iProc] = localElementCount;

        // allocate the index-list and the data-list
        allocateForProc(iProc);

        // fill in the index-list
        _indexList[iProc][0] = _dataCount[iProc] * _numberOfBytesPerElement;
        uint64_t *currentIndex = _indexList[iProc] + _numberOfDimensions + 1;
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            _indexList[iProc][iDim + 1] =
                    nComponentsAtPositionAlongDim[iDim][position[iDim]];
            for (uint64_t iComponent = 0;
                    iComponent
                    < nComponentsAtPositionAlongDim[iDim][position[iDim]];
                    iComponent++) {
                currentIndex[(iComponent << 1)] =
                        lowerLocalOffsetAtPositionAlongDim[iDim][position[iDim]][iComponent];
                currentIndex[(iComponent << 1) + 1] =
                        upperLocalOffsetAtPositionAlongDim[iDim][position[iDim]][iComponent]
                        - lowerLocalOffsetAtPositionAlongDim[iDim][position[iDim]][iComponent]
                        + 1;
            }
            currentIndex += nComponentsAtPositionAlongDim[iDim][position[iDim]]
                    << 1;
        }
    }

    // delete temporarily auxiliary arrays
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        uint64_t nProcsAlongThisDim = partition.getProcCount(iDim);
        for (uint64_t iProc = 0; iProc < nProcsAlongThisDim; iProc++) {
            if (nComponentsAtPositionAlongDim[iDim][iProc] == 0)
                continue;
            delete[] lowerLocalOffsetAtPositionAlongDim[iDim][iProc];
            delete[] upperLocalOffsetAtPositionAlongDim[iDim][iProc];
        }
        delete[] nComponentsAtPositionAlongDim[iDim];
        delete[] lowerLocalOffsetAtPositionAlongDim[iDim];
        delete[] upperLocalOffsetAtPositionAlongDim[iDim];
    }

    if (_nData != indexSet.getNumberOfIndices())
        throw ECorruptedRuntime();
}

GlobalRequestRegionTensor::~GlobalRequestRegionTensor() {
}

void GlobalRequestRegionTensor::getData(const uint64_t numberOfBytesPerElement,
        const uint64_t nData, char *data) {
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
    uint64_t regionCombination[7];
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        regionCombination[iDim] = 0;
    }
    while (regionCombination[0] < _nComponentsAlongDim[0]) {
        uint64_t regionWidth[7];
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            regionWidth[iDim] = getRegionWidth(iDim, regionCombination[iDim]);
        }
        // iterate through the points within this region
        uint64_t combination[7], position[7], localOffset[7], localWidth[7];
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            combination[iDim] = 0;
            position[iDim] =
                    _lowerOwnerPositionAlongDim[iDim][regionCombination[iDim]];
            localOffset[iDim] =
                    _lowerOffsetInOwnerAlongDim[iDim][regionCombination[iDim]];
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
                position[iDim] =
                        _lowerOwnerPositionAlongDim[iDim][regionCombination[iDim]];
                localOffset[iDim] =
                        _lowerOffsetInOwnerAlongDim[iDim][regionCombination[iDim]];
                localWidth[iDim] = getNode(iDim, position[iDim] + 1)
                        - getNode(iDim, position[iDim]);
            }
        }
        // get next region combination
        for (int iDim = _numberOfDimensions - 1; iDim >= 0; iDim--) {
            regionCombination[iDim]++;
            if (iDim == 0
                    || regionCombination[iDim] < _nComponentsAlongDim[iDim])
                break;
            regionCombination[iDim] = 0;
        }
    }

    delete[] dataIndexAtProc;
}

void GlobalRequestRegionTensor::setData(const uint64_t numberOfBytesPerElement,
        const uint64_t nData, const char *data) {
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
    uint64_t regionCombination[7];
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        regionCombination[iDim] = 0;
    }
    while (regionCombination[0] < _nComponentsAlongDim[0]) {
        uint64_t regionWidth[7];
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            regionWidth[iDim] = getRegionWidth(iDim, regionCombination[iDim]);
        }
        // iterate through the points within this region
        uint64_t combination[7], position[7], localOffset[7], localWidth[7];
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            combination[iDim] = 0;
            position[iDim] =
                    _lowerOwnerPositionAlongDim[iDim][regionCombination[iDim]];
            localOffset[iDim] =
                    _lowerOffsetInOwnerAlongDim[iDim][regionCombination[iDim]];
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
                position[iDim] =
                        _lowerOwnerPositionAlongDim[iDim][regionCombination[iDim]];
                localOffset[iDim] =
                        _lowerOffsetInOwnerAlongDim[iDim][regionCombination[iDim]];
                localWidth[iDim] = getNode(iDim, position[iDim] + 1)
                        - getNode(iDim, position[iDim]);
            }
        }
        // get next region combination
        for (int iDim = _numberOfDimensions - 1; iDim >= 0; iDim--) {
            regionCombination[iDim]++;
            if (iDim == 0
                    || regionCombination[iDim] < _nComponentsAlongDim[iDim])
                break;
            regionCombination[iDim] = 0;
        }
    }

    delete[] dataIndexAtProc;
}


