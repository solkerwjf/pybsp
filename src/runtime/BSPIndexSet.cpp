/*
 * BSPIndexSet.cpp
 *
 *  Created on: 2014-8-14
 *      Author: junfeng
 */

#include "BSPIndexSet.hpp"
#include "BSPLocalArray.hpp"

using namespace BSP;

IndexSet::IndexSet(unsigned numberOfDimensions, uint64_t numberOfIndices) {
    if (numberOfDimensions == 0)
        throw EInvalidArgument();
    _numberOfDimensions = numberOfDimensions;
    _numberOfIndices = numberOfIndices;
    if (_numberOfDimensions == 0 || _numberOfIndices == 0
            || _numberOfDimensions > 7)
        throw EInvalidArgument();
    _begin = NULL;
    _end = NULL;
    _curr = NULL;
}

IndexSet::~IndexSet() {
    if (_begin != NULL)
        delete _begin;
    if (_end != NULL)
        delete _end;
    if (_curr != NULL)
        delete _curr;
}

unsigned IndexSet::getNumberOfDimensions() {
    return _numberOfDimensions;
}

uint64_t IndexSet::getNumberOfIndices() {
    return _numberOfIndices;
}

uint64_t IndexSet::getNumberOfRegions() {
    return _numberOfRegions;
}

IndexSet::Iterator &IndexSet::begin() {
    return *_begin;
}

IndexSet::Iterator &IndexSet::end() {
    return *_end;
}

IndexSet::Iterator &IndexSet::curr() {
    return *_curr;
}

IndexSet::Iterator::Iterator(IndexSet *indexSet, unsigned numberOfComponents,
        unsigned numberOfMainComponents, uint64_t initialComponentBound[],
        bool isBegin) {
    if (indexSet == NULL || initialComponentBound == NULL)
        throw EInvalidArgument();
    if (numberOfComponents == 0 || numberOfMainComponents == 0
            || numberOfMainComponents > numberOfComponents
            || indexSet->_numberOfIndices == 0)
        throw EInvalidArgument();
    _indexSet = indexSet;
    _numberOfComponents = numberOfComponents;
    _numberOfMainComponents = numberOfMainComponents;

    _componentBound = new uint64_t[_numberOfComponents];
    _componentRank = new uint64_t[_numberOfComponents];
    if (_componentBound == NULL || _componentRank == NULL)
        throw ENotEnoughMemory();

    for (unsigned iComponent = 0; iComponent < _numberOfComponents;
            iComponent++) {
        _componentBound[iComponent] = initialComponentBound[iComponent];
        if (_componentBound[iComponent] == 0)
            throw EInvalidArgument();
        if (isBegin || iComponent >= _numberOfMainComponents)
            _componentRank[iComponent] = 0;
        else
            _componentRank[iComponent] = _componentBound[iComponent] - 1;
    }

    if (isBegin) {
        _indexRank = 0;
        _regionRank = 0;
    } else {
        _indexSet->updateComponentBoundsOfIterator(this);
        for (unsigned iComponent = _numberOfMainComponents;
                iComponent < _numberOfComponents; iComponent++) {
            _componentRank[iComponent] = _componentBound[iComponent] - 1;
        }
        _indexRank = _indexSet->_numberOfIndices;
        _regionRank = _indexSet->_numberOfRegions;
    }
    _indexSet->getIndex(*this);
}

void IndexSet::Iterator::increase() {
    if (_indexRank >= _indexSet->_numberOfIndices)
        return;
    if (_indexRank == _indexSet->_numberOfIndices - 1) {
        _indexRank++;
        _regionRank++;
        return;
    }

    bool needBoundUpdate = false;
    for (int iComponent = _numberOfComponents - 1; iComponent >= 0;
            iComponent--) {
        _componentRank[iComponent]++;
        if (iComponent == 0
                || _componentRank[iComponent] < _componentBound[iComponent])
            break;
        _componentRank[iComponent] = 0;
        if (iComponent == (int) _numberOfMainComponents)
            needBoundUpdate = true;
    }
    if (needBoundUpdate) {
        _indexSet->updateComponentBoundsOfIterator(this);
        _regionRank++;
    }
    _indexRank++;
    _indexSet->getIndex(*this);
}

IndexSet::Iterator::Iterator(const Iterator &iterator) {
    reset(false);
    *this = iterator;
}

IndexSet::Iterator::~Iterator() {
    reset(true);
}

void IndexSet::Iterator::setComponentBound(unsigned iComponent,
        uint64_t newBound) {
    if (iComponent >= _numberOfComponents
            || iComponent < _numberOfMainComponents)
        throw EInvalidArgument();
    _componentBound[iComponent] = newBound;
}

IndexSet::Iterator &IndexSet::Iterator::operator++() {
    increase();
    return *this;
}

IndexSet::Iterator IndexSet::Iterator::operator++(int) {
    Iterator result(*this);
    increase();
    return result;
}

IndexSet::Iterator &IndexSet::Iterator::operator=(const Iterator &iterator) {
    reset(true);
    _indexSet = iterator._indexSet;
    _numberOfComponents = iterator._numberOfComponents;
    _numberOfMainComponents = iterator._numberOfMainComponents;
    _componentBound = new uint64_t[_numberOfComponents];
    _componentRank = new uint64_t[_numberOfComponents];
    if (_componentBound == NULL || _componentRank == NULL)
        throw ENotEnoughMemory();
    for (unsigned iComponent = 0; iComponent < _numberOfComponents;
            iComponent++) {
        _componentBound[iComponent] = iterator._componentBound[iComponent];
        _componentRank[iComponent] = iterator._componentRank[iComponent];
    }
    _indexRank = iterator._indexRank;
    _regionRank = iterator._regionRank;
    for (unsigned iDim = 0; iDim < _indexSet->_numberOfDimensions; iDim++) {
        _index[iDim] = iterator._index[iDim];
    }
    return *this;
}

bool IndexSet::Iterator::operator==(Iterator &iterator) {
    return (_indexSet == iterator._indexSet && _indexRank == iterator._indexRank);
}

bool IndexSet::Iterator::operator!=(Iterator &iterator) {
    return !(_indexSet == iterator._indexSet
            && _indexRank == iterator._indexRank);
}

uint64_t IndexSet::Iterator::getIndexRank() {
    return _indexRank;
}

uint64_t IndexSet::Iterator::getComponentRank(unsigned iComponent) {
    if (iComponent < _numberOfComponents)
        return _componentRank[iComponent];
    else
        throw EInvalidArgument();
}

bool IndexSet::Iterator::hasNext() {
    return _indexRank + 1 < _indexSet->_numberOfIndices;
}

uint64_t IndexSet::Iterator::nextRegion() {
    if (_indexSet->_numberOfRegions == 0) {
        uint64_t result = _indexSet->_numberOfIndices - _indexRank;
        *this = _indexSet->end();
        return result;
    }

    uint64_t iInRegion = _componentRank[_numberOfMainComponents];
    uint64_t nInRegion = _componentBound[_numberOfMainComponents];
    _componentRank[_numberOfMainComponents] =
            _componentBound[_numberOfMainComponents] - 1;
    for (unsigned iComponent = _numberOfMainComponents + 1;
            iComponent < _numberOfComponents; iComponent++) {
        iInRegion *= _componentBound[iComponent];
        iInRegion += _componentRank[iComponent];
        _componentRank[iComponent] = _componentBound[iComponent] - 1;
        nInRegion *= _componentBound[iComponent];
    }
    _indexRank += nInRegion - 1 - iInRegion;
    increase();
    return nInRegion - iInRegion;
}

void IndexSet::Iterator::reset(bool release) {
    if (release && _numberOfComponents > 0) {
        delete[] _componentBound;
        delete[] _componentRank;
    }
    _indexSet = NULL;
    _numberOfComponents = 0;
    _numberOfMainComponents = 0;
    _componentBound = NULL;
    _componentRank = NULL;
    _indexSet = 0;
}

uint64_t IndexSet::Iterator::getIndex(unsigned iDim) {
    if (iDim > _indexSet->_numberOfDimensions)
        throw EInvalidArgument();
    return _index[iDim];
}

uint64_t IndexSet::Iterator::list(uint64_t amount, LocalArray &indexArray,
        uint64_t &amountLeft) {
    if (indexArray.getNumberOfDimensions() != 2
            || indexArray.getNumberOfBytesPerElement() != sizeof (uint64_t))
        throw EInvalidArgument();
    if (indexArray.getElementCount(0) < amount
            || indexArray.getElementCount(1) != _indexSet->_numberOfDimensions)
        throw EInvalidArgument();

    uint64_t *result = (uint64_t *) indexArray.getData();
    uint64_t k = 0;
    for (uint64_t i = 0; i < amount && _indexRank < _indexSet->_numberOfIndices;
            i++) {
        for (unsigned iDim = 0; iDim < _indexSet->_numberOfDimensions; iDim++) {
            result[k++] = _index[iDim];
        }
        increase();
    }
    amountLeft = _indexSet->_numberOfIndices - _indexRank;
    return k / _indexSet->_numberOfDimensions;
}

uint64_t IndexSet::Iterator::listRegion(uint64_t amount, LocalArray &indexArray,
        uint64_t &amountLeft, uint64_t &regionRank) {
    if (indexArray.getNumberOfDimensions() != 2
            || indexArray.getNumberOfBytesPerElement() != sizeof (int64_t))
        throw EInvalidArgument();
    if (indexArray.getElementCount(0) < amount
            || indexArray.getElementCount(1) != _indexSet->_numberOfDimensions)
        throw EInvalidArgument();

    uint64_t *result = (uint64_t *) indexArray.getData();
    uint64_t k = 0;
    uint64_t currentRegionRank = _regionRank;
    for (uint64_t i = 0;
            i < amount && _indexRank < _indexSet->_numberOfIndices
            && _regionRank == currentRegionRank; i++) {
        for (unsigned iDim = 0; iDim < _indexSet->_numberOfDimensions; iDim++) {
            result[k++] = _index[iDim];
        }
        increase();
    }
    regionRank = currentRegionRank;
    amountLeft = _indexSet->_numberOfIndices - _indexRank;
    return k / _indexSet->_numberOfDimensions;
}

uint64_t IndexSet::Iterator::getRegionStart(uint64_t amount,
        LocalArray &indexArray, uint64_t &amountLeft) {
    if (indexArray.getNumberOfDimensions() != 1
            || indexArray.getNumberOfBytesPerElement() != sizeof (int64_t))
        throw EInvalidArgument();
    if (indexArray.getElementCount(0) < amount)
        throw EInvalidArgument();

    uint64_t *result = (uint64_t *) indexArray.getData();
    uint64_t actualAmount = 0;
    for (uint64_t i = 0; i < amount && _indexRank < _indexSet->_numberOfIndices;
            i++) {
        result[i] = _indexRank;
        nextRegion();
        actualAmount++;
    }
    amountLeft = _indexSet->_numberOfRegions - _regionRank;
    return actualAmount;
}

void IndexSet::Iterator::reset() {
    *this = *_indexSet->_begin;
}

