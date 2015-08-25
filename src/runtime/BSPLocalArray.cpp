/*
 * BSPLocalArray.cpp
 *
 *  Created on: 2014-7-17
 *      Author: junfeng
 */

#include "BSPLocalArray.hpp"
#include "BSPRuntime.hpp"
#include <cassert>
#include <cstdio>
#include "BSPLocalRequestPointSequence.hpp"
#include "BSPLocalRequestPointTensor.hpp"
#include "BSPLocalRequestRegionSequence.hpp"
#include "BSPLocalRequestRegionTensor.hpp"
#include "BSPArrayRegistration.hpp"

using namespace BSP;

uint64_t LocalArray::getByteCount() {
    return _numberOfElementsInTotal * _numberOfBytesPerElement;
}

/// @brief constructor with shape definition

LocalArray::LocalArray(std::string path, ElementType elementType,
        uint64_t numberOfBytesPerElement, unsigned numberOfDimensions,
        uint64_t *numberOfElementsAlongDimension) :
ArrayShape(elementType, numberOfBytesPerElement, numberOfDimensions,
numberOfElementsAlongDimension), _path(path) {
    mapData();
    _registration = NULL;
    _path = path;
}

/// @brief constructor with shape serialization

LocalArray::LocalArray(std::string path, uint64_t *serialization) :
ArrayShape(serialization) {
    mapData();
    _registration = NULL;
    _path = path;
}

/// @brief null constructor for processes out of grid

LocalArray::LocalArray() : ArrayShape() {
    _registration = NULL;
}

/// @brief destructor

LocalArray::~LocalArray() {
    if (_data != NULL)
        unmapData();
    if (_registration != NULL)
        delete _registration;
}

void LocalArray::mapData() {
    _data = new char[getByteCount()];
    if (_data == NULL) {
        throw ENotEnoughMemory();
    }
}

void LocalArray::unmapData() {
    delete[] _data;
}

template<class TFrom, class TTo>
void localArrayCopy(uint64_t n, char *cfrom, char *cto) {
    TFrom *from = (TFrom *) cfrom;
    TTo *to = (TTo *) cto;
    for (uint64_t i = 0; i < n; ++i)
        to[i] = (TTo) from[i];
}

template<class TFrom, class TTo>
void localArrayCopyR2C(uint64_t n, char *cfrom, char *cto) {
    TFrom *from = (TFrom *) cfrom;
    TTo *to = (TTo *) cto;
    for (uint64_t i = 0; i < n; ++i) {
        to[i<<1] = (TTo) from[i];
        to[1 + (i << 1)] = (TTo)0;
    }
}
template<class TFrom, class TTo>
void localArrayCopyC2C(uint64_t n, char *cfrom, char *cto) {
    TFrom *from = (TFrom *) cfrom;
    TTo *to = (TTo *) cto;
    for (uint64_t i = 0; i < n; ++i) {
        to[i<<1] = (TTo) from[i<<1];
        to[1 + (i << 1)] = (TTo)from[1 + (i << 1)];
    }
}

void localArrayCopy(uint64_t n, uint64_t m, char *from, char *to) {
    uint64_t mn = m * n;
    for (uint64_t i = 0; i < mn; i++)
        to[i] = from[i];
}

/// @brief copy array
/// @param array the array to copy

void LocalArray::copy(LocalArray &array) {
    bool equalSize = true;
    if (_numberOfDimensions != array._numberOfDimensions)
        equalSize = false;
    else if (_numberOfElementsInTotal != array._numberOfElementsInTotal)
        equalSize = false;
    if (!equalSize)
        throw EInvalidArgument();
    switch (_elementType) {
        case INT8:
            switch (array._elementType) {
                case INT8:
                    localArrayCopy<int8_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT16:
                    localArrayCopy<int16_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT32:
                    localArrayCopy<int32_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT64:
                    localArrayCopy<int64_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT8:
                    localArrayCopy<uint8_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT16:
                    localArrayCopy<uint16_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT32:
                    localArrayCopy<uint32_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT64:
                    localArrayCopy<uint64_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case FLOAT:
                    localArrayCopy<float, int8_t>(_numberOfElementsInTotal, array._data,
                            _data);
                    break;
                case DOUBLE:
                    localArrayCopy<double, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT8:
                    localArrayCopyR2C<int8_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT16:
                    localArrayCopyR2C<int16_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT32:
                    localArrayCopyR2C<int32_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT64:
                    localArrayCopyR2C<int64_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT8:
                    localArrayCopyR2C<uint8_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT16:
                    localArrayCopyR2C<uint16_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT32:
                    localArrayCopyR2C<uint32_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT64:
                    localArrayCopyR2C<uint64_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CFLOAT:
                    localArrayCopyR2C<float, int8_t>(_numberOfElementsInTotal, array._data,
                            _data);
                    break;
                case CDOUBLE:
                    localArrayCopyR2C<double, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case BINARY:
                    if (_numberOfBytesPerElement != sizeof (int8_t))
                        throw EInvalidArgument();
                    localArrayCopy<int8_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        case INT16:
            switch (array._elementType) {
                case INT8:
                    localArrayCopy<int8_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT16:
                    localArrayCopy<int16_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT32:
                    localArrayCopy<int32_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT64:
                    localArrayCopy<int64_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT8:
                    localArrayCopy<uint8_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT16:
                    localArrayCopy<uint16_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT32:
                    localArrayCopy<uint32_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT64:
                    localArrayCopy<uint64_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case FLOAT:
                    localArrayCopy<float, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case DOUBLE:
                    localArrayCopy<double, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT8:
                    localArrayCopyR2C<int8_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT16:
                    localArrayCopyR2C<int16_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT32:
                    localArrayCopyR2C<int32_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT64:
                    localArrayCopyR2C<int64_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT8:
                    localArrayCopyR2C<uint8_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT16:
                    localArrayCopyR2C<uint16_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT32:
                    localArrayCopyR2C<uint32_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT64:
                    localArrayCopyR2C<uint64_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CFLOAT:
                    localArrayCopyR2C<float, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CDOUBLE:
                    localArrayCopyR2C<double, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case BINARY:
                    if (_numberOfBytesPerElement != sizeof (int16_t))
                        throw EInvalidArgument();
                    localArrayCopy<int16_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        case INT32:
            switch (array._elementType) {
                case INT8:
                    localArrayCopy<int8_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT16:
                    localArrayCopy<int16_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT32:
                    localArrayCopy<int32_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT64:
                    localArrayCopy<int64_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT8:
                    localArrayCopy<uint8_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT16:
                    localArrayCopy<uint16_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT32:
                    localArrayCopy<uint32_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT64:
                    localArrayCopy<uint64_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case FLOAT:
                    localArrayCopy<float, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case DOUBLE:
                    localArrayCopy<double, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT8:
                    localArrayCopyR2C<int8_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT16:
                    localArrayCopyR2C<int16_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT32:
                    localArrayCopyR2C<int32_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT64:
                    localArrayCopyR2C<int64_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT8:
                    localArrayCopyR2C<uint8_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT16:
                    localArrayCopyR2C<uint16_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT32:
                    localArrayCopyR2C<uint32_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT64:
                    localArrayCopyR2C<uint64_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CFLOAT:
                    localArrayCopyR2C<float, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CDOUBLE:
                    localArrayCopyR2C<double, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case BINARY:
                    if (_numberOfBytesPerElement != sizeof (int32_t))
                        throw EInvalidArgument();
                    localArrayCopy<int32_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        case INT64:
            switch (array._elementType) {
                case INT8:
                    localArrayCopy<int8_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT16:
                    localArrayCopy<int16_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT32:
                    localArrayCopy<int32_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT64:
                    localArrayCopy<int64_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT8:
                    localArrayCopy<uint8_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT16:
                    localArrayCopy<uint16_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT32:
                    localArrayCopy<uint32_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT64:
                    localArrayCopy<uint64_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case FLOAT:
                    localArrayCopy<float, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case DOUBLE:
                    localArrayCopy<double, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT8:
                    localArrayCopyR2C<int8_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT16:
                    localArrayCopyR2C<int16_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT32:
                    localArrayCopyR2C<int32_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT64:
                    localArrayCopyR2C<int64_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT8:
                    localArrayCopyR2C<uint8_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT16:
                    localArrayCopyR2C<uint16_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT32:
                    localArrayCopyR2C<uint32_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT64:
                    localArrayCopyR2C<uint64_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CFLOAT:
                    localArrayCopyR2C<float, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CDOUBLE:
                    localArrayCopyR2C<double, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case BINARY:
                    if (_numberOfBytesPerElement != sizeof (int64_t))
                        throw EInvalidArgument();
                    localArrayCopy<int64_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        case UINT8:
            switch (array._elementType) {
                case INT8:
                    localArrayCopy<int8_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT16:
                    localArrayCopy<int16_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT32:
                    localArrayCopy<int32_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT64:
                    localArrayCopy<int64_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT8:
                    localArrayCopy<uint8_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT16:
                    localArrayCopy<uint16_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT32:
                    localArrayCopy<uint32_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT64:
                    localArrayCopy<uint64_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case FLOAT:
                    localArrayCopy<float, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case DOUBLE:
                    localArrayCopy<double, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT8:
                    localArrayCopyR2C<int8_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT16:
                    localArrayCopyR2C<int16_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT32:
                    localArrayCopyR2C<int32_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT64:
                    localArrayCopyR2C<int64_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT8:
                    localArrayCopyR2C<uint8_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT16:
                    localArrayCopyR2C<uint16_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT32:
                    localArrayCopyR2C<uint32_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT64:
                    localArrayCopyR2C<uint64_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CFLOAT:
                    localArrayCopyR2C<float, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CDOUBLE:
                    localArrayCopyR2C<double, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case BINARY:
                    if (_numberOfBytesPerElement != sizeof (uint8_t))
                        throw EInvalidArgument();
                    localArrayCopy<uint8_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        case UINT16:
            switch (array._elementType) {
                case INT8:
                    localArrayCopy<int8_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT16:
                    localArrayCopy<int16_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT32:
                    localArrayCopy<int32_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT64:
                    localArrayCopy<int64_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT8:
                    localArrayCopy<uint8_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT16:
                    localArrayCopy<uint16_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT32:
                    localArrayCopy<uint32_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT64:
                    localArrayCopy<uint64_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case FLOAT:
                    localArrayCopy<float, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case DOUBLE:
                    localArrayCopy<double, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT8:
                    localArrayCopyR2C<int8_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT16:
                    localArrayCopyR2C<int16_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT32:
                    localArrayCopyR2C<int32_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT64:
                    localArrayCopyR2C<int64_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT8:
                    localArrayCopyR2C<uint8_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT16:
                    localArrayCopyR2C<uint16_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT32:
                    localArrayCopyR2C<uint32_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT64:
                    localArrayCopyR2C<uint64_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CFLOAT:
                    localArrayCopyR2C<float, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CDOUBLE:
                    localArrayCopyR2C<double, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case BINARY:
                    if (_numberOfBytesPerElement != sizeof (uint16_t))
                        throw EInvalidArgument();
                    localArrayCopy<uint16_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        case UINT32:
            switch (array._elementType) {
                case INT8:
                    localArrayCopy<int8_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT16:
                    localArrayCopy<int16_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT32:
                    localArrayCopy<int32_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT64:
                    localArrayCopy<int64_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT8:
                    localArrayCopy<uint8_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT16:
                    localArrayCopy<uint16_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT32:
                    localArrayCopy<uint32_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT64:
                    localArrayCopy<uint64_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case FLOAT:
                    localArrayCopy<float, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case DOUBLE:
                    localArrayCopy<double, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT8:
                    localArrayCopyR2C<int8_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT16:
                    localArrayCopyR2C<int16_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT32:
                    localArrayCopyR2C<int32_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT64:
                    localArrayCopyR2C<int64_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT8:
                    localArrayCopyR2C<uint8_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT16:
                    localArrayCopyR2C<uint16_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT32:
                    localArrayCopyR2C<uint32_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT64:
                    localArrayCopyR2C<uint64_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CFLOAT:
                    localArrayCopyR2C<float, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CDOUBLE:
                    localArrayCopyR2C<double, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case BINARY:
                    if (_numberOfBytesPerElement != sizeof (uint32_t))
                        throw EInvalidArgument();
                    localArrayCopy<uint32_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        case UINT64:
            switch (array._elementType) {
                case INT8:
                    localArrayCopy<int8_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT16:
                    localArrayCopy<int16_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT32:
                    localArrayCopy<int32_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT64:
                    localArrayCopy<int64_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT8:
                    localArrayCopy<uint8_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT16:
                    localArrayCopy<uint16_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT32:
                    localArrayCopy<uint32_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT64:
                    localArrayCopy<uint64_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case FLOAT:
                    localArrayCopy<float, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case DOUBLE:
                    localArrayCopy<double, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT8:
                    localArrayCopyR2C<int8_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT16:
                    localArrayCopyR2C<int16_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT32:
                    localArrayCopyR2C<int32_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT64:
                    localArrayCopyR2C<int64_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT8:
                    localArrayCopyR2C<uint8_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT16:
                    localArrayCopyR2C<uint16_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT32:
                    localArrayCopyR2C<uint32_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT64:
                    localArrayCopyR2C<uint64_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CFLOAT:
                    localArrayCopyR2C<float, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CDOUBLE:
                    localArrayCopyR2C<double, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case BINARY:
                    if (_numberOfBytesPerElement != sizeof (uint64_t))
                        throw EInvalidArgument();
                    localArrayCopy<uint64_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        case FLOAT:
            switch (array._elementType) {
                case INT8:
                    localArrayCopy<int8_t, float>(_numberOfElementsInTotal, array._data,
                            _data);
                    break;
                case INT16:
                    localArrayCopy<int16_t, float>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT32:
                    localArrayCopy<int32_t, float>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT64:
                    localArrayCopy<int64_t, float>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT8:
                    localArrayCopy<uint8_t, float>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT16:
                    localArrayCopy<uint16_t, float>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT32:
                    localArrayCopy<uint32_t, float>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT64:
                    localArrayCopy<uint64_t, float>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case FLOAT:
                    localArrayCopy<float, float>(_numberOfElementsInTotal, array._data,
                            _data);
                    break;
                case DOUBLE:
                    localArrayCopy<double, float>(_numberOfElementsInTotal, array._data,
                            _data);
                    break;
                case CINT8:
                    localArrayCopyR2C<int8_t, float>(_numberOfElementsInTotal, array._data,
                            _data);
                    break;
                case CINT16:
                    localArrayCopyR2C<int16_t, float>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT32:
                    localArrayCopyR2C<int32_t, float>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT64:
                    localArrayCopyR2C<int64_t, float>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT8:
                    localArrayCopyR2C<uint8_t, float>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT16:
                    localArrayCopyR2C<uint16_t, float>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT32:
                    localArrayCopyR2C<uint32_t, float>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT64:
                    localArrayCopyR2C<uint64_t, float>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CFLOAT:
                    localArrayCopyR2C<float, float>(_numberOfElementsInTotal, array._data,
                            _data);
                    break;
                case CDOUBLE:
                    localArrayCopyR2C<double, float>(_numberOfElementsInTotal, array._data,
                            _data);
                    break;
                case BINARY:
                    if (_numberOfBytesPerElement != sizeof (float))
                        throw EInvalidArgument();
                    localArrayCopy<float, float>(_numberOfElementsInTotal, array._data,
                            _data);
                    break;
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        case DOUBLE:
            switch (array._elementType) {
                case INT8:
                    localArrayCopy<int8_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT16:
                    localArrayCopy<int16_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT32:
                    localArrayCopy<int32_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case INT64:
                    localArrayCopy<int64_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT8:
                    localArrayCopy<uint8_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT16:
                    localArrayCopy<uint16_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT32:
                    localArrayCopy<uint32_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case UINT64:
                    localArrayCopy<uint64_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case FLOAT:
                    localArrayCopy<float, double>(_numberOfElementsInTotal, array._data,
                            _data);
                    break;
                case DOUBLE:
                    localArrayCopy<double, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT8:
                    localArrayCopyR2C<int8_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT16:
                    localArrayCopyR2C<int16_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT32:
                    localArrayCopyR2C<int32_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT64:
                    localArrayCopyR2C<int64_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT8:
                    localArrayCopyR2C<uint8_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT16:
                    localArrayCopyR2C<uint16_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT32:
                    localArrayCopyR2C<uint32_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT64:
                    localArrayCopyR2C<uint64_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CFLOAT:
                    localArrayCopyR2C<float, double>(_numberOfElementsInTotal, array._data,
                            _data);
                    break;
                case CDOUBLE:
                    localArrayCopyR2C<double, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case BINARY:
                    if (_numberOfBytesPerElement != sizeof (double))
                        throw EInvalidArgument();
                    localArrayCopy<double, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        case CINT8:
            switch (array._elementType) {
                case CINT8:
                    localArrayCopyC2C<int8_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT16:
                    localArrayCopyC2C<int16_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT32:
                    localArrayCopyC2C<int32_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT64:
                    localArrayCopyC2C<int64_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT8:
                    localArrayCopyC2C<uint8_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT16:
                    localArrayCopyC2C<uint16_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT32:
                    localArrayCopyC2C<uint32_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT64:
                    localArrayCopyC2C<uint64_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CFLOAT:
                    localArrayCopyC2C<float, int8_t>(_numberOfElementsInTotal, array._data,
                            _data);
                    break;
                case CDOUBLE:
                    localArrayCopyC2C<double, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case BINARY:
                    if (_numberOfBytesPerElement != sizeof (int8_t))
                        throw EInvalidArgument();
                    localArrayCopy<int8_t, int8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        case CINT16:
            switch (array._elementType) {
                case CINT8:
                    localArrayCopyC2C<int8_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT16:
                    localArrayCopyC2C<int16_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT32:
                    localArrayCopyC2C<int32_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT64:
                    localArrayCopyC2C<int64_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT8:
                    localArrayCopyC2C<uint8_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT16:
                    localArrayCopyC2C<uint16_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT32:
                    localArrayCopyC2C<uint32_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT64:
                    localArrayCopyC2C<uint64_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CFLOAT:
                    localArrayCopyC2C<float, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CDOUBLE:
                    localArrayCopyC2C<double, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case BINARY:
                    if (_numberOfBytesPerElement != sizeof (int16_t))
                        throw EInvalidArgument();
                    localArrayCopy<int16_t, int16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        case CINT32:
            switch (array._elementType) {
                case CINT8:
                    localArrayCopyC2C<int8_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT16:
                    localArrayCopyC2C<int16_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT32:
                    localArrayCopyC2C<int32_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT64:
                    localArrayCopyC2C<int64_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT8:
                    localArrayCopyC2C<uint8_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT16:
                    localArrayCopyC2C<uint16_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT32:
                    localArrayCopyC2C<uint32_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT64:
                    localArrayCopyC2C<uint64_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CFLOAT:
                    localArrayCopyC2C<float, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CDOUBLE:
                    localArrayCopyC2C<double, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case BINARY:
                    if (_numberOfBytesPerElement != sizeof (int32_t))
                        throw EInvalidArgument();
                    localArrayCopy<int32_t, int32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        case CINT64:
            switch (array._elementType) {
                case CINT8:
                    localArrayCopyC2C<int8_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT16:
                    localArrayCopyC2C<int16_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT32:
                    localArrayCopyC2C<int32_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT64:
                    localArrayCopyC2C<int64_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT8:
                    localArrayCopyC2C<uint8_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT16:
                    localArrayCopyC2C<uint16_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT32:
                    localArrayCopyC2C<uint32_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT64:
                    localArrayCopyC2C<uint64_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CFLOAT:
                    localArrayCopyC2C<float, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CDOUBLE:
                    localArrayCopyC2C<double, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case BINARY:
                    if (_numberOfBytesPerElement != sizeof (int64_t))
                        throw EInvalidArgument();
                    localArrayCopy<int64_t, int64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        case CUINT8:
            switch (array._elementType) {
                case CINT8:
                    localArrayCopyC2C<int8_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT16:
                    localArrayCopyC2C<int16_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT32:
                    localArrayCopyC2C<int32_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT64:
                    localArrayCopyC2C<int64_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT8:
                    localArrayCopyC2C<uint8_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT16:
                    localArrayCopyC2C<uint16_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT32:
                    localArrayCopyC2C<uint32_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT64:
                    localArrayCopyC2C<uint64_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CFLOAT:
                    localArrayCopyC2C<float, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CDOUBLE:
                    localArrayCopyC2C<double, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case BINARY:
                    if (_numberOfBytesPerElement != sizeof (uint8_t))
                        throw EInvalidArgument();
                    localArrayCopy<uint8_t, uint8_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        case CUINT16:
            switch (array._elementType) {
                case CINT8:
                    localArrayCopyC2C<int8_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT16:
                    localArrayCopyC2C<int16_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT32:
                    localArrayCopyC2C<int32_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT64:
                    localArrayCopyC2C<int64_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT8:
                    localArrayCopyC2C<uint8_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT16:
                    localArrayCopyC2C<uint16_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT32:
                    localArrayCopyC2C<uint32_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT64:
                    localArrayCopyC2C<uint64_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CFLOAT:
                    localArrayCopyC2C<float, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CDOUBLE:
                    localArrayCopyC2C<double, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case BINARY:
                    if (_numberOfBytesPerElement != sizeof (uint16_t))
                        throw EInvalidArgument();
                    localArrayCopy<uint16_t, uint16_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        case CUINT32:
            switch (array._elementType) {
                case CINT8:
                    localArrayCopyC2C<int8_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT16:
                    localArrayCopyC2C<int16_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT32:
                    localArrayCopyC2C<int32_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT64:
                    localArrayCopyC2C<int64_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT8:
                    localArrayCopyC2C<uint8_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT16:
                    localArrayCopyC2C<uint16_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT32:
                    localArrayCopyC2C<uint32_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT64:
                    localArrayCopyC2C<uint64_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CFLOAT:
                    localArrayCopyC2C<float, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CDOUBLE:
                    localArrayCopyC2C<double, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case BINARY:
                    if (_numberOfBytesPerElement != sizeof (uint32_t))
                        throw EInvalidArgument();
                    localArrayCopy<uint32_t, uint32_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        case CUINT64:
            switch (array._elementType) {
                case CINT8:
                    localArrayCopyC2C<int8_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT16:
                    localArrayCopyC2C<int16_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT32:
                    localArrayCopyC2C<int32_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT64:
                    localArrayCopyC2C<int64_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT8:
                    localArrayCopyC2C<uint8_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT16:
                    localArrayCopyC2C<uint16_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT32:
                    localArrayCopyC2C<uint32_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT64:
                    localArrayCopyC2C<uint64_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CFLOAT:
                    localArrayCopyC2C<float, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CDOUBLE:
                    localArrayCopyC2C<double, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case BINARY:
                    if (_numberOfBytesPerElement != sizeof (uint64_t))
                        throw EInvalidArgument();
                    localArrayCopy<uint64_t, uint64_t>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        case CFLOAT:
            switch (array._elementType) {
                case CINT8:
                    localArrayCopyC2C<int8_t, float>(_numberOfElementsInTotal, array._data,
                            _data);
                    break;
                case CINT16:
                    localArrayCopyC2C<int16_t, float>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT32:
                    localArrayCopyC2C<int32_t, float>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT64:
                    localArrayCopyC2C<int64_t, float>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT8:
                    localArrayCopyC2C<uint8_t, float>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT16:
                    localArrayCopyC2C<uint16_t, float>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT32:
                    localArrayCopyC2C<uint32_t, float>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT64:
                    localArrayCopyC2C<uint64_t, float>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CFLOAT:
                    localArrayCopyC2C<float, float>(_numberOfElementsInTotal, array._data,
                            _data);
                    break;
                case CDOUBLE:
                    localArrayCopyC2C<double, float>(_numberOfElementsInTotal, array._data,
                            _data);
                    break;
                case BINARY:
                    if (_numberOfBytesPerElement != sizeof (float))
                        throw EInvalidArgument();
                    localArrayCopy<float, float>(_numberOfElementsInTotal, array._data,
                            _data);
                    break;
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        case CDOUBLE:
            switch (array._elementType) {
                case CINT8:
                    localArrayCopyC2C<int8_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT16:
                    localArrayCopyC2C<int16_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT32:
                    localArrayCopyC2C<int32_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CINT64:
                    localArrayCopyC2C<int64_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT8:
                    localArrayCopyC2C<uint8_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT16:
                    localArrayCopyC2C<uint16_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT32:
                    localArrayCopyC2C<uint32_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CUINT64:
                    localArrayCopyC2C<uint64_t, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case CFLOAT:
                    localArrayCopyC2C<float, double>(_numberOfElementsInTotal, array._data,
                            _data);
                    break;
                case CDOUBLE:
                    localArrayCopyC2C<double, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                case BINARY:
                    if (_numberOfBytesPerElement != sizeof (double))
                        throw EInvalidArgument();
                    localArrayCopy<double, double>(_numberOfElementsInTotal,
                            array._data, _data);
                    break;
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        case BINARY:
            if (_numberOfBytesPerElement != array._numberOfBytesPerElement)
                throw EInvalidArgument();
            localArrayCopy(_numberOfElementsInTotal, _numberOfBytesPerElement,
                    array._data, _data);
            break;
        default:
            throw EInvalidArgument();
    }
}

/// @brief fill

void LocalArray::fill(char *value) throw (EInvalidArgument) {
    if (value == NULL)
        throw EInvalidArgument();
    uint64_t k = 0;
    for (uint64_t iElement = 0; iElement < _numberOfElementsInTotal;
            iElement++) {
        for (unsigned iByte = 0; iByte < _numberOfBytesPerElement; iByte++)
            _data[k++] = value[iByte];
    }
}

// index structure:
// data byte count
// n
// point 0
// point 1
// ...
// point n - 1

void LocalArray::getElementsPointSequence(const uint64_t *index, char *output) {
    uint64_t iValid = 0;
    uint64_t dataByteCount = index[0];
    uint64_t n = index[1];
    const uint64_t *temp = index + 2;
    for (uint64_t i = 0; i < n; i++) {
        uint64_t offset = temp[i];
        if (offset >= _numberOfElementsInTotal)
            throw EInvalidArgument();
        char *validOutput = output + iValid * _numberOfBytesPerElement;
        char *validData = _data + offset * _numberOfBytesPerElement;
        iValid++;
        for (unsigned iByte = 0; iByte < _numberOfBytesPerElement; iByte++)
            validOutput[iByte] = validData[iByte];
    }
    if (iValid * _numberOfBytesPerElement != dataByteCount)
        throw EInvalidArgument();
}

// index structure for k-dimensional array
// data byte count
// n[0]
// n[1]
// ...
// n[k-1]
// component[0][0]
// ...
// component[0][n[0]-1]
// component[1][0]
// ...
// component[1][n[1]-1]
// ...
// component[k-1][0]
// ...
// component[k-1][n[k-1]-1]

void LocalArray::getElementsPointTensor(const uint64_t *index, char *output) {
    uint64_t iValid = 0;
    uint64_t n[7];
    const uint64_t * component[7];
    uint64_t position[7];
    uint64_t prefix = _numberOfDimensions + 1;
    uint64_t dataByteCount = index[0];
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        n[iDim] = index[iDim + 1];
        component[iDim] = index + prefix;
        prefix += n[iDim];
        position[iDim] = 0;
        for (unsigned iComponent = 0; iComponent < n[iDim]; iComponent++) {
            if (component[iDim][iComponent]
                    >= _numberOfElementsAlongDimension[iDim])
                throw EInvalidArgument();
        }
    }

    while (position[0] < n[0]) {
        // compute the offset of the element
        uint64_t offset = component[0][position[0]];
        for (unsigned iDim = 1; iDim < _numberOfDimensions; iDim++) {
            offset *= _numberOfElementsAlongDimension[iDim];
            offset += component[iDim][position[iDim]];
        }

        // copy the element if valid
        char *validOutput = output + iValid * _numberOfBytesPerElement;
        char *validData = _data + offset * _numberOfBytesPerElement;
        iValid++;
        for (unsigned iByte = 0; iByte < _numberOfBytesPerElement; iByte++)
            validOutput[iByte] = validData[iByte];

        // next position
        for (int iDim = _numberOfDimensions - 1; iDim >= 0; iDim--) {
            position[iDim]++;
            if (iDim == 0 || position[iDim] < n[iDim])
                break;
            position[iDim] = 0;
        }
    }
    if (iValid * _numberOfBytesPerElement != dataByteCount)
        throw EInvalidArgument();
}

// index structure for k-dimensional array:
// data byte count
// n
// start, width 0, ..., width k-1 of region 0,
// start, width 0, ..., width k-1 of region 1,
// ...
// start, width 0, ..., width k-1 of region n-1,

void LocalArray::getElementsRegionSequence(const uint64_t *index,
        char *output) {
    uint64_t iValid = 0;
    uint64_t dataByteCount = index[0];
    uint64_t n = index[1];
    const uint64_t *temp = index + 2;
    for (uint64_t i = 0; i < n; i++) {
        uint64_t offset;
        uint64_t width[7], position[7];
        offset = temp[0];
        temp++;
        if (offset >= _numberOfElementsInTotal)
            throw EInvalidArgument();
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            width[iDim] = temp[iDim];
            if (width[iDim] > _numberOfElementsAlongDimension[iDim])
                throw EInvalidArgument();
            position[iDim] = 0;
        }
        temp += _numberOfDimensions;

        while (position[0] < width[0]) {
            // compute the element offset increment
            uint64_t deltaOffset = position[0];
            for (unsigned iDim = 1; iDim < _numberOfDimensions; iDim++) {
                deltaOffset *= _numberOfElementsAlongDimension[iDim];
                deltaOffset += position[iDim];
            }
            if (offset + deltaOffset >= _numberOfElementsInTotal)
                throw EInvalidArgument();

            // copy the data
            char *validOutput = output + iValid * _numberOfBytesPerElement;
            char *validData = _data
                    + (offset + deltaOffset) * _numberOfBytesPerElement;
            iValid++;
            for (unsigned iByte = 0; iByte < _numberOfBytesPerElement; iByte++)
                validOutput[iByte] = validData[iByte];

            // next position
            for (int iDim = _numberOfDimensions - 1; iDim >= 0; iDim--) {
                position[iDim]++;
                if (iDim == 0 || position[iDim] < width[iDim])
                    break;
                position[iDim] = 0;
            }
        }
    }
    if (iValid * _numberOfBytesPerElement != dataByteCount)
        throw EInvalidArgument();
}

// index structure for k-dimensional array
// data byte count
// n[0]
// n[1]
// ...
// n[k-1]
// component[0][0], width[0][0]
// ...
// component[0][n[0]-1], width[0][n[0]-1]
// component[1][0], width[1][0]
// ...
// component[1][n[1]-1], width[1][n[1]-1]
// ...
// component[k-1][0], width[k-1][0]
// ...
// component[k-1][n[k-1]-1], width[k-1][n[k-1]-1]

void LocalArray::getElementsRegionTensor(const uint64_t *index, char *output) {
    uint64_t iValid = 0;
    uint64_t n[7];
    const uint64_t * component[7];
    uint64_t outerPosition[7], innerPosition[7];
    uint64_t prefix = _numberOfDimensions + 1;
    uint64_t dataByteCount = index[0];
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        n[iDim] = index[iDim + 1];
        component[iDim] = index + prefix;
        prefix += n[iDim] * 2;
        outerPosition[iDim] = 0;
        for (uint64_t iComponent = 0; iComponent < n[iDim]; iComponent++) {
            uint64_t end = component[iDim][iComponent << 1]
                    + component[iDim][(iComponent << 1) + 1];
            if (end > _numberOfElementsAlongDimension[iDim])
                throw EInvalidArgument();
        }
    }

    // iterate through the outer positions
    while (outerPosition[0] < n[0]) {
        // compute the initial offset of this outer position
        uint64_t width[7];
        uint64_t offset = component[0][outerPosition[0] << 1];
        width[0] = component[0][1 + (outerPosition[0] << 1)];
        for (unsigned iDim = 1; iDim < _numberOfDimensions; iDim++) {
            offset *= _numberOfElementsAlongDimension[iDim];
            offset += component[iDim][outerPosition[iDim] << 1];
            width[iDim] = component[iDim][1 + (outerPosition[iDim] << 1)];
        }

        // iterate through the inner positions
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++)
            innerPosition[iDim] = 0;
        while (innerPosition[0] < width[0]) {
            // compute the element offset increment
            uint64_t deltaOffset = innerPosition[0];
            for (unsigned iDim = 1; iDim < _numberOfDimensions; iDim++) {
                deltaOffset *= _numberOfElementsAlongDimension[iDim];
                deltaOffset += innerPosition[iDim];
            }

            // copy the data
            char *validOutput = output + iValid * _numberOfBytesPerElement;
            char *validData = _data
                    + (offset + deltaOffset) * _numberOfBytesPerElement;
            iValid++;
            for (unsigned iByte = 0; iByte < _numberOfBytesPerElement; iByte++)
                validOutput[iByte] = validData[iByte];

            // next inner position
            for (int iDim = _numberOfDimensions - 1; iDim >= 0; iDim--) {
                innerPosition[iDim]++;
                if (iDim == 0 || innerPosition[iDim] < width[iDim])
                    break;
                innerPosition[iDim] = 0;
            }
        }

        // next outer position
        for (int iDim = _numberOfDimensions - 1; iDim >= 0; iDim--) {
            outerPosition[iDim]++;
            if (iDim == 0 || outerPosition[iDim] < n[iDim])
                break;
            outerPosition[iDim] = 0;
        }
    }

    if (iValid * _numberOfBytesPerElement != dataByteCount)
        throw EInvalidArgument();
}

// index structure for k-dimensional array:
// data byte count
// nExpectedFailures, nVariables
// coefficient matrix (_numberOfDimensions x (nVariables + 1))
// start, end of variable 0
// ...
// start, end of variable nVariables - 1

void LocalArray::getElementsLinearMapping(const uint64_t *index, char *output) {
    uint64_t iValid = 0;

    // extract the linear mapping from the index
    int64_t *si = (int64_t *) index;
    uint64_t dataByteCount = index[0];
    uint64_t nExpectedFailures = index[1];
    uint64_t nVariables = index[2];
    if (nVariables == 0 || nVariables > 7)
        throw EInvalidArgument();
    int64_t coef[7][8];
    uint64_t start[7], end[7], position[7];
    unsigned j = 3;
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        for (unsigned iVar = 0; iVar <= nVariables; iVar++)
            coef[iDim][iVar] = si[j++];
    }
    for (unsigned iVar = 0; iVar < nVariables; iVar++) {
        start[iVar] = index[j++];
        end[iVar] = index[j++];
        if (end[iVar] < start[iVar])
            throw EInvalidArgument();
        position[iVar] = start[iVar];
    }

    // iterate throw all positions within this range
    uint64_t nFailures = 0;
    while (position[0] <= end[0]) {
        // compute the point from the mapping
        int64_t x[7];
        bool pointValid = true;
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            x[iDim] = coef[iDim][nVariables];
            for (unsigned iVar = 0; iVar < nVariables; iVar++)
                x[iDim] += coef[iDim][iVar] * position[iVar];
            // validate the point
            if (x[iDim] < 0
                    || x[iDim]
                    >= (int64_t) _numberOfElementsAlongDimension[iDim])
                pointValid = false;
        }

        // copy this point or increase the failure count
        if (pointValid) {
            uint64_t offset = x[0];
            for (unsigned iDim = 1; iDim < _numberOfDimensions; iDim++) {
                offset *= _numberOfElementsAlongDimension[iDim];
                offset += x[iDim];
            }
            char *validOutput = output + iValid * _numberOfBytesPerElement;
            char *validData = _data + offset * _numberOfBytesPerElement;
            iValid++;
            for (unsigned iByte = 0; iByte < _numberOfBytesPerElement; iByte++)
                validOutput[iByte] = validData[iByte];
        } else
            nFailures++;

        // get next position
        for (int iVar = nVariables - 1; iVar >= 0; --iVar) {
            position[iVar]++;
            if (iVar == 0 || position[iVar] <= end[iVar])
                break;
            position[iVar] = start[iVar];
        }
    }

    if (nFailures != nExpectedFailures)
        throw EInvalidArgument();

    if (iValid * _numberOfBytesPerElement != dataByteCount)
        throw EInvalidArgument();
}

template<class TDST, class TSRC>
void cmul(TDST *dst, const TSRC *src) {
    TDST re = dst[0] * src[0] - dst[1] * src[1];
    TDST im = dst[0] * src[1] + dst[1] * src[0];
    dst[0] = re;
    dst[1] = im;
}

void LocalArray::updateElement(const char *src, char *dst,
        uint16_t updateType) {
    switch (updateType) {
        case OPID_ASSIGN:
        {
            switch (_elementType) {
                case INT8:
                {
                    *(int8_t *) dst = *(int8_t *) src;
                    break;
                }
                case INT16:
                {
                    *(int16_t *) dst = *(int16_t *) src;
                    break;
                }
                case INT32:
                {
                    *(int32_t *) dst = *(int32_t *) src;
                    break;
                }
                case INT64:
                {
                    *(int64_t *) dst = *(int64_t *) src;
                    break;
                }
                case UINT8:
                {
                    *(uint8_t *) dst = *(uint8_t *) src;
                    break;
                }
                case UINT16:
                {
                    *(uint16_t *) dst = *(uint16_t *) src;
                    break;
                }
                case UINT32:
                {
                    *(uint32_t *) dst = *(uint32_t *) src;
                    break;
                }
                case UINT64:
                {
                    *(uint64_t *) dst = *(uint64_t *) src;
                    break;
                }
                case FLOAT:
                {
                    *(float *) dst = *(float *) src;
                    break;
                }
                case DOUBLE:
                {
                    *(double *) dst = *(double *) src;
                    break;
                }
                case CINT8:
                {
                    *(int8_t *) dst = *(int8_t *) src;
                    *((int8_t *) dst + 1) = *((int8_t *) src + 1);
                    break;
                }
                case CINT16:
                {
                    *(int16_t *) dst = *(int16_t *) src;
                    *((int16_t *) dst + 1) = *((int16_t *) src + 1);
                    break;
                }
                case CINT32:
                {
                    *(int32_t *) dst = *(int32_t *) src;
                    *((int32_t *) dst + 1) = *((int32_t *) src + 1);
                    break;
                }
                case CINT64:
                {
                    *(int64_t *) dst = *(int64_t *) src;
                    *((int64_t *) dst + 1) = *((int64_t *) src + 1);
                    break;
                }
                case CUINT8:
                {
                    *(uint8_t *) dst = *(uint8_t *) src;
                    *((uint8_t *) dst + 1) = *((uint8_t *) src + 1);
                    break;
                }
                case CUINT16:
                {
                    *(uint16_t *) dst = *(uint16_t *) src;
                    *((uint16_t *) dst + 1) = *((uint16_t *) src + 1);
                    break;
                }
                case CUINT32:
                {
                    *(uint32_t *) dst = *(uint32_t *) src;
                    *((uint32_t *) dst + 1) = *((uint32_t *) src + 1);
                    break;
                }
                case CUINT64:
                {
                    *(uint64_t *) dst = *(uint64_t *) src;
                    *((uint64_t *) dst + 1) = *((uint64_t *) src + 1);
                    break;
                }
                case CFLOAT:
                {
                    *(float *) dst = *(float *) src;
                    *((float *) dst + 1) = *((float *) src + 1);
                    break;
                }
                case CDOUBLE:
                {
                    *(double *) dst = *(double *) src;
                    *((double *) dst + 1) = *((double *) src + 1);
                    break;
                }
                case BINARY:
                {
                    for (unsigned iByte = 0; iByte < _numberOfBytesPerElement; iByte++)
                        dst[iByte] = src[iByte];
                    break;
                }
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        }
        case OPID_ADD:
        {
            switch (_elementType) {
                case INT8:
                {
                    *(int8_t *) dst += *(int8_t *) src;
                    break;
                }
                case INT16:
                {
                    *(int16_t *) dst += *(int16_t *) src;
                    break;
                }
                case INT32:
                {
                    *(int32_t *) dst += *(int32_t *) src;
                    break;
                }
                case INT64:
                {
                    *(int64_t *) dst += *(int64_t *) src;
                    break;
                }
                case UINT8:
                {
                    *(uint8_t *) dst += *(uint8_t *) src;
                    break;
                }
                case UINT16:
                {
                    *(uint16_t *) dst += *(uint16_t *) src;
                    break;
                }
                case UINT32:
                {
                    *(uint32_t *) dst += *(uint32_t *) src;
                    break;
                }
                case UINT64:
                {
                    *(uint64_t *) dst += *(uint64_t *) src;
                    break;
                }
                case FLOAT:
                {
                    *(float *) dst += *(float *) src;
                    break;
                }
                case DOUBLE:
                {
                    *(double *) dst += *(double *) src;
                    break;
                }
                case CINT8:
                {
                    *(int8_t *) dst += *(int8_t *) src;
                    *((int8_t *) dst + 1) += *((int8_t *) src + 1);
                    break;
                }
                case CINT16:
                {
                    *(int16_t *) dst += *(int16_t *) src;
                    *((int16_t *) dst + 1) += *((int16_t *) src + 1);
                    break;
                }
                case CINT32:
                {
                    *(int32_t *) dst += *(int32_t *) src;
                    *((int32_t *) dst + 1) += *((int32_t *) src + 1);
                    break;
                }
                case CINT64:
                {
                    *(int64_t *) dst += *(int64_t *) src;
                    *((int64_t *) dst + 1) += *((int64_t *) src + 1);
                    break;
                }
                case CUINT8:
                {
                    *(uint8_t *) dst += *(uint8_t *) src;
                    *((uint8_t *) dst + 1) += *((uint8_t *) src + 1);
                    break;
                }
                case CUINT16:
                {
                    *(uint16_t *) dst += *(uint16_t *) src;
                    *((uint16_t *) dst + 1) += *((uint16_t *) src + 1);
                    break;
                }
                case CUINT32:
                {
                    *(uint32_t *) dst += *(uint32_t *) src;
                    *((uint32_t *) dst + 1) += *((uint32_t *) src + 1);
                    break;
                }
                case CUINT64:
                {
                    *(uint64_t *) dst += *(uint64_t *) src;
                    *((uint64_t *) dst + 1) += *((uint64_t *) src + 1);
                    break;
                }
                case CFLOAT:
                {
                    *(float *) dst += *(float *) src;
                    *((float *) dst + 1) += *((float *) src + 1);
                    break;
                }
                case CDOUBLE:
                {
                    *(double *) dst += *(double *) src;
                    *((double *) dst + 1) += *((double *) src + 1);
                    break;
                }
                case BINARY:
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        }
        case OPID_MUL:
        {
            switch (_elementType) {
                case INT8:
                {
                    *(int8_t *) dst *= *(int8_t *) src;
                    break;
                }
                case INT16:
                {
                    *(int16_t *) dst *= *(int16_t *) src;
                    break;
                }
                case INT32:
                {
                    *(int32_t *) dst *= *(int32_t *) src;
                    break;
                }
                case INT64:
                {
                    *(int64_t *) dst *= *(int64_t *) src;
                    break;
                }
                case UINT8:
                {
                    *(uint8_t *) dst *= *(uint8_t *) src;
                    break;
                }
                case UINT16:
                {
                    *(uint16_t *) dst *= *(uint16_t *) src;
                    break;
                }
                case UINT32:
                {
                    *(uint32_t *) dst *= *(uint32_t *) src;
                    break;
                }
                case UINT64:
                {
                    *(uint64_t *) dst *= *(uint64_t *) src;
                    break;
                }
                case FLOAT:
                {
                    *(float *) dst *= *(float *) src;
                    break;
                }
                case DOUBLE:
                {
                    *(double *) dst *= *(double *) src;
                    break;
                }
                case CINT8:
                {
                    cmul((int8_t *) dst, (const int8_t *) src);
                    break;
                }
                case CINT16:
                {
                    cmul((int16_t *) dst, (const int16_t *) src);
                    break;
                }
                case CINT32:
                {
                    cmul((int32_t *) dst, (const int32_t *) src);
                    break;
                }
                case CINT64:
                {
                    cmul((int64_t *) dst, (const int64_t *) src);
                    break;
                }
                case CUINT8:
                {
                    cmul((uint8_t *) dst, (const uint8_t *) src);
                    break;
                }
                case CUINT16:
                {
                    cmul((uint16_t *) dst, (const uint16_t *) src);
                    break;
                }
                case CUINT32:
                {
                    cmul((uint32_t *) dst, (const uint32_t *) src);
                    break;
                }
                case CUINT64:
                {
                    cmul((uint64_t *) dst, (const uint64_t *) src);
                    break;
                }
                case CFLOAT:
                {
                    cmul((float *) dst, (const float *) src);
                    break;
                }
                case CDOUBLE:
                {
                    cmul((double *) dst, (const double *) src);
                    break;
                }
                case BINARY:
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        }
        case OPID_AND:
        {
            switch (_elementType) {
                case INT8:
                {
                    *(int8_t *) dst &= *(int8_t *) src;
                    break;
                }
                case INT16:
                {
                    *(int16_t *) dst &= *(int16_t *) src;
                    break;
                }
                case INT32:
                {
                    *(int32_t *) dst &= *(int32_t *) src;
                    break;
                }
                case INT64:
                {
                    *(int64_t *) dst &= *(int64_t *) src;
                    break;
                }
                case UINT8:
                {
                    *(uint8_t *) dst &= *(uint8_t *) src;
                    break;
                }
                case UINT16:
                {
                    *(uint16_t *) dst &= *(uint16_t *) src;
                    break;
                }
                case UINT32:
                {
                    *(uint32_t *) dst &= *(uint32_t *) src;
                    break;
                }
                case UINT64:
                {
                    *(uint64_t *) dst &= *(uint64_t *) src;
                    break;
                }
                case CINT8:
                {
                    *(int8_t *) dst &= *(int8_t *) src;
                    *((int8_t *) dst + 1) &= *((int8_t *) src + 1);
                    break;
                }
                case CINT16:
                {
                    *(int16_t *) dst &= *(int16_t *) src;
                    *((int16_t *) dst + 1) &= *((int16_t *) src + 1);
                    break;
                }
                case CINT32:
                {
                    *(int32_t *) dst &= *(int32_t *) src;
                    *((int32_t *) dst + 1) &= *((int32_t *) src + 1);
                    break;
                }
                case CINT64:
                {
                    *(int64_t *) dst &= *(int64_t *) src;
                    *((int64_t *) dst + 1) &= *((int64_t *) src + 1);
                    break;
                }
                case CUINT8:
                {
                    *(uint8_t *) dst &= *(uint8_t *) src;
                    *((uint8_t *) dst + 1) &= *((uint8_t *) src + 1);
                    break;
                }
                case CUINT16:
                {
                    *(uint16_t *) dst &= *(uint16_t *) src;
                    *((uint16_t *) dst + 1) &= *((uint16_t *) src + 1);
                    break;
                }
                case CUINT32:
                {
                    *(uint32_t *) dst &= *(uint32_t *) src;
                    *((uint32_t *) dst + 1) &= *((uint32_t *) src + 1);
                    break;
                }
                case CUINT64:
                {
                    *(uint64_t *) dst &= *(uint64_t *) src;
                    *((uint64_t *) dst + 1) &= *((uint64_t *) src + 1);
                    break;
                }
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        }
        case OPID_XOR:
        {
            switch (_elementType) {
                case INT8:
                {
                    *(int8_t *) dst ^= *(int8_t *) src;
                    break;
                }
                case INT16:
                {
                    *(int16_t *) dst ^= *(int16_t *) src;
                    break;
                }
                case INT32:
                {
                    *(int32_t *) dst ^= *(int32_t *) src;
                    break;
                }
                case INT64:
                {
                    *(int64_t *) dst ^= *(int64_t *) src;
                    break;
                }
                case UINT8:
                {
                    *(uint8_t *) dst ^= *(uint8_t *) src;
                    break;
                }
                case UINT16:
                {
                    *(uint16_t *) dst ^= *(uint16_t *) src;
                    break;
                }
                case UINT32:
                {
                    *(uint32_t *) dst ^= *(uint32_t *) src;
                    break;
                }
                case UINT64:
                {
                    *(uint64_t *) dst ^= *(uint64_t *) src;
                    break;
                }
                case CINT8:
                {
                    *(int8_t *) dst ^= *(int8_t *) src;
                    *((int8_t *) dst + 1) ^= *((int8_t *) src + 1);
                    break;
                }
                case CINT16:
                {
                    *(int16_t *) dst ^= *(int16_t *) src;
                    *((int16_t *) dst + 1) ^= *((int16_t *) src + 1);
                    break;
                }
                case CINT32:
                {
                    *(int32_t *) dst ^= *(int32_t *) src;
                    *((int32_t *) dst + 1) ^= *((int32_t *) src + 1);
                    break;
                }
                case CINT64:
                {
                    *(int64_t *) dst ^= *(int64_t *) src;
                    *((int64_t *) dst + 1) ^= *((int64_t *) src + 1);
                    break;
                }
                case CUINT8:
                {
                    *(uint8_t *) dst ^= *(uint8_t *) src;
                    *((uint8_t *) dst + 1) ^= *((uint8_t *) src + 1);
                    break;
                }
                case CUINT16:
                {
                    *(uint16_t *) dst ^= *(uint16_t *) src;
                    *((uint16_t *) dst + 1) ^= *((uint16_t *) src + 1);
                    break;
                }
                case CUINT32:
                {
                    *(uint32_t *) dst ^= *(uint32_t *) src;
                    *((uint32_t *) dst + 1) ^= *((uint32_t *) src + 1);
                    break;
                }
                case CUINT64:
                {
                    *(uint64_t *) dst ^= *(uint64_t *) src;
                    *((uint64_t *) dst + 1) ^= *((uint64_t *) src + 1);
                    break;
                }
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        }
        case OPID_OR:
        {
            switch (_elementType) {
                case INT8:
                {
                    *(int8_t *) dst |= *(int8_t *) src;
                    break;
                }
                case INT16:
                {
                    *(int16_t *) dst |= *(int16_t *) src;
                    break;
                }
                case INT32:
                {
                    *(int32_t *) dst |= *(int32_t *) src;
                    break;
                }
                case INT64:
                {
                    *(int64_t *) dst |= *(int64_t *) src;
                    break;
                }
                case UINT8:
                {
                    *(uint8_t *) dst |= *(uint8_t *) src;
                    break;
                }
                case UINT16:
                {
                    *(uint16_t *) dst |= *(uint16_t *) src;
                    break;
                }
                case UINT32:
                {
                    *(uint32_t *) dst |= *(uint32_t *) src;
                    break;
                }
                case UINT64:
                {
                    *(uint64_t *) dst |= *(uint64_t *) src;
                    break;
                }
                case CINT8:
                {
                    *(int8_t *) dst |= *(int8_t *) src;
                    *((int8_t *) dst + 1) |= *((int8_t *) src + 1);
                    break;
                }
                case CINT16:
                {
                    *(int16_t *) dst |= *(int16_t *) src;
                    *((int16_t *) dst + 1) |= *((int16_t *) src + 1);
                    break;
                }
                case CINT32:
                {
                    *(int32_t *) dst |= *(int32_t *) src;
                    *((int32_t *) dst + 1) |= *((int32_t *) src + 1);
                    break;
                }
                case CINT64:
                {
                    *(int64_t *) dst |= *(int64_t *) src;
                    *((int64_t *) dst + 1) |= *((int64_t *) src + 1);
                    break;
                }
                case CUINT8:
                {
                    *(uint8_t *) dst |= *(uint8_t *) src;
                    *((uint8_t *) dst + 1) |= *((uint8_t *) src + 1);
                    break;
                }
                case CUINT16:
                {
                    *(uint16_t *) dst |= *(uint16_t *) src;
                    *((uint16_t *) dst + 1) |= *((uint16_t *) src + 1);
                    break;
                }
                case CUINT32:
                {
                    *(uint32_t *) dst |= *(uint32_t *) src;
                    *((uint32_t *) dst + 1) |= *((uint32_t *) src + 1);
                    break;
                }
                case CUINT64:
                {
                    *(uint64_t *) dst |= *(uint64_t *) src;
                    *((uint64_t *) dst + 1) |= *((uint64_t *) src + 1);
                    break;
                }
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        }
        case OPID_MAX:
        {
            switch (_elementType) {
                case INT8:
                {
                    if (*(int8_t *) dst < *(int8_t *) src)
                        *(int8_t *) dst = *(int8_t *) src;
                    break;
                }
                case INT16:
                {
                    if (*(int16_t *) dst < *(int16_t *) src)
                        *(int16_t *) dst = *(int16_t *) src;
                    break;
                }
                case INT32:
                {
                    if (*(int32_t *) dst < *(int32_t *) src)
                        *(int32_t *) dst = *(int32_t *) src;
                    break;
                }
                case INT64:
                {
                    if (*(int64_t *) dst < *(int64_t *) src)
                        *(int64_t *) dst = *(int64_t *) src;
                    break;
                }
                case UINT8:
                {
                    if (*(uint8_t *) dst < *(uint8_t *) src)
                        *(uint8_t *) dst = *(uint8_t *) src;
                    break;
                }
                case UINT16:
                {
                    if (*(uint16_t *) dst < *(uint16_t *) src)
                        *(uint16_t *) dst = *(uint16_t *) src;
                    break;
                }
                case UINT32:
                {
                    if (*(uint32_t *) dst < *(uint32_t *) src)
                        *(uint32_t *) dst = *(uint32_t *) src;
                    break;
                }
                case UINT64:
                {
                    if (*(uint64_t *) dst < *(uint64_t *) src)
                        *(uint64_t *) dst = *(uint64_t *) src;
                    break;
                }
                case FLOAT:
                {
                    if (*(float *) dst < *(float *) src)
                        *(float *) dst = *(float *) src;
                    break;
                }
                case DOUBLE:
                {
                    if (*(double *) dst < *(double *) src)
                        *(double *) dst = *(double *) src;
                    break;
                }
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        }
        case OPID_MIN:
        {
            switch (_elementType) {
                case INT8:
                {
                    if (*(int8_t *) dst > *(int8_t *) src)
                        *(int8_t *) dst = *(int8_t *) src;
                    break;
                }
                case INT16:
                {
                    if (*(int16_t *) dst > *(int16_t *) src)
                        *(int16_t *) dst = *(int16_t *) src;
                    break;
                }
                case INT32:
                {
                    if (*(int32_t *) dst > *(int32_t *) src)
                        *(int32_t *) dst = *(int32_t *) src;
                    break;
                }
                case INT64:
                {
                    if (*(int64_t *) dst > *(int64_t *) src)
                        *(int64_t *) dst = *(int64_t *) src;
                    break;
                }
                case UINT8:
                {
                    if (*(uint8_t *) dst > *(uint8_t *) src)
                        *(uint8_t *) dst = *(uint8_t *) src;
                    break;
                }
                case UINT16:
                {
                    if (*(uint16_t *) dst > *(uint16_t *) src)
                        *(uint16_t *) dst = *(uint16_t *) src;
                    break;
                }
                case UINT32:
                {
                    if (*(uint32_t *) dst > *(uint32_t *) src)
                        *(uint32_t *) dst = *(uint32_t *) src;
                    break;
                }
                case UINT64:
                {
                    if (*(uint64_t *) dst > *(uint64_t *) src)
                        *(uint64_t *) dst = *(uint64_t *) src;
                    break;
                }
                case FLOAT:
                {
                    if (*(float *) dst > *(float *) src)
                        *(float *) dst = *(float *) src;
                    break;
                }
                case DOUBLE:
                {
                    if (*(double *) dst > *(double *) src)
                        *(double *) dst = *(double *) src;
                    break;
                }
                default:
                    throw EInvalidArgument();
                    break;
            }
            break;
        }
        default:
            throw EInvalidArgument();
            break;
    }
}

// index structure:
// data byte count
// n
// point 0
// point 1
// ...
// point n - 1

void LocalArray::setElementsPointSequence(const uint64_t *index,
        const char *input, uint16_t updateType) {
    uint64_t iValid = 0;
    uint64_t dataByteCount = index[0];
    uint64_t n = index[1];
    const uint64_t *temp = index + 2;
    for (uint64_t i = 0; i < n; i++) {
        uint64_t offset = temp[i];
        if (offset >= _numberOfElementsInTotal)
            throw EInvalidArgument();
        const char *validInput = input + iValid * _numberOfBytesPerElement;
        char *validData = _data + offset * _numberOfBytesPerElement;
        iValid++;
        updateElement(validInput, validData, updateType);
    }

    if (iValid * _numberOfBytesPerElement != dataByteCount)
        throw EInvalidArgument();
}

// index structure for k-dimensional array
// data byte count
// n[0]
// n[1]
// ...
// n[k-1]
// component[0][0]
// ...
// component[0][n[0]-1]
// component[1][0]
// ...
// component[1][n[1]-1]
// ...
// component[k-1][0]
// ...
// component[k-1][n[k-1]-1]

void LocalArray::setElementsPointTensor(const uint64_t *index,
        const char *input, uint16_t updateType) {
    uint64_t iValid = 0;
    uint64_t n[7];
    const uint64_t * component[7];
    uint64_t position[7];
    uint64_t prefix = _numberOfDimensions + 1;
    uint64_t dataByteCount = index[0];
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        n[iDim] = index[iDim + 1];
        component[iDim] = index + prefix;
        prefix += n[iDim];
        position[iDim] = 0;
        for (unsigned iComponent = 0; iComponent < n[iDim]; iComponent++) {
            if (component[iDim][iComponent]
                    >= _numberOfElementsAlongDimension[iDim])
                throw EInvalidArgument();
        }
    }

    while (position[0] < n[0]) {
        // compute the offset of the element
        uint64_t offset = component[0][position[0]];
        for (unsigned iDim = 1; iDim < _numberOfDimensions; iDim++) {
            offset *= _numberOfElementsAlongDimension[iDim];
            offset += component[iDim][position[iDim]];
        }

        // copy the element if valid
        const char *validInput = input + iValid * _numberOfBytesPerElement;
        char *validData = _data + offset * _numberOfBytesPerElement;
        iValid++;
        updateElement(validInput, validData, updateType);

        // next position
        for (int iDim = _numberOfDimensions - 1; iDim >= 0; iDim--) {
            position[iDim]++;
            if (iDim == 0 || position[iDim] < n[iDim])
                break;
            position[iDim] = 0;
        }
    }

    if (iValid * _numberOfBytesPerElement != dataByteCount)
        throw EInvalidArgument();
}

// index structure for k-dimensional array:
// data byte count
// n
// start, width 0, ..., width k-1 of region 0,
// start, width 0, ..., width k-1 of region 1,
// ...
// start, width 0, ..., width k-1 of region n-1,

void LocalArray::setElementsRegionSequence(const uint64_t *index,
        const char *input, uint16_t updateType) {
    uint64_t iValid = 0;
    uint64_t dataByteCount = index[0];
    uint64_t n = index[1];

    const uint64_t *temp = index + 2;
    for (uint64_t i = 0; i < n; i++) {
        uint64_t offset;
        uint64_t width[7], position[7];
        offset = temp[0];
        temp++;
        if (offset >= _numberOfElementsInTotal)
            throw EInvalidArgument();
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            width[iDim] = temp[iDim];
            if (width[iDim] > _numberOfElementsAlongDimension[iDim])
                throw EInvalidArgument();
            position[iDim] = 0;
        }
        temp += _numberOfDimensions;

        while (position[0] < width[0]) {
            // compute the element offset increment
            uint64_t deltaOffset = position[0];
            for (unsigned iDim = 1; iDim < _numberOfDimensions; iDim++) {
                deltaOffset *= _numberOfElementsAlongDimension[iDim];
                deltaOffset += position[iDim];
            }
            if (offset + deltaOffset >= _numberOfElementsInTotal)
                throw EInvalidArgument();

            // copy the data
            const char *validInput = input + iValid * _numberOfBytesPerElement;
            char *validData = _data
                    + (offset + deltaOffset) * _numberOfBytesPerElement;
            iValid++;
            updateElement(validInput, validData, updateType);

            // next position
            for (int iDim = _numberOfDimensions - 1; iDim >= 0; iDim--) {
                position[iDim]++;
                if (iDim == 0 || position[iDim] < width[iDim])
                    break;
                position[iDim] = 0;
            }
        }
    }

    if (iValid * _numberOfBytesPerElement != dataByteCount)
        throw EInvalidArgument();
}

// index structure for k-dimensional array
// data byte count
// n[0]
// n[1]
// ...
// n[k-1]
// component[0][0], width[0][0]
// ...
// component[0][n[0]-1], width[0][n[0]-1]
// component[1][0], width[1][0]
// ...
// component[1][n[1]-1], width[1][n[1]-1]
// ...
// component[k-1][0], width[k-1][0]
// ...
// component[k-1][n[k-1]-1], width[k-1][n[k-1]-1]

void LocalArray::setElementsRegionTensor(const uint64_t *index,
        const char *input, uint16_t updateType) {
    uint64_t iValid = 0;
    uint64_t n[7];
    const uint64_t * component[7];
    uint64_t outerPosition[7], innerPosition[7];
    uint64_t prefix = _numberOfDimensions + 1;
    uint64_t dataByteCount = index[0];
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        n[iDim] = index[iDim + 1];
        component[iDim] = index + prefix;
        prefix += n[iDim] * 2;
        outerPosition[iDim] = 0;
        for (uint64_t iComponent = 0; iComponent < n[iDim]; iComponent++) {
            uint64_t end = component[iDim][iComponent << 1]
                    + component[iDim][(iComponent << 1) + 1];
            if (end > _numberOfElementsAlongDimension[iDim])
                throw EInvalidArgument();
        }
    }

    // iterate through the outer positions
    while (outerPosition[0] < n[0]) {
        // compute the initial offset of this outer position
        uint64_t width[7];
        uint64_t offset = component[0][outerPosition[0] << 1];
        width[0] = component[0][1 + (outerPosition[0] << 1)];
        for (unsigned iDim = 1; iDim < _numberOfDimensions; iDim++) {
            offset *= _numberOfElementsAlongDimension[iDim];
            offset += component[iDim][outerPosition[iDim] << 1];
            width[iDim] = component[iDim][1 + (outerPosition[iDim] << 1)];
        }

        // iterate through the inner positions
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++)
            innerPosition[iDim] = 0;
        while (innerPosition[0] < width[0]) {
            // compute the element offset increment
            uint64_t deltaOffset = innerPosition[0];
            for (unsigned iDim = 1; iDim < _numberOfDimensions; iDim++) {
                deltaOffset *= _numberOfElementsAlongDimension[iDim];
                deltaOffset += innerPosition[iDim];
            }

            // copy the data
            const char *validInput = input + iValid * _numberOfBytesPerElement;
            char *validData = _data
                    + (offset + deltaOffset) * _numberOfBytesPerElement;
            iValid++;
            updateElement(validInput, validData, updateType);

            // next inner position
            for (int iDim = _numberOfDimensions - 1; iDim >= 0; iDim--) {
                innerPosition[iDim]++;
                if (iDim == 0 || innerPosition[iDim] < width[iDim])
                    break;
                innerPosition[iDim] = 0;
            }
        }

        // next outer position
        for (int iDim = _numberOfDimensions - 1; iDim >= 0; iDim--) {
            outerPosition[iDim]++;
            if (iDim == 0 || outerPosition[iDim] < n[iDim])
                break;
            outerPosition[iDim] = 0;
        }
    }

    if (iValid * _numberOfBytesPerElement != dataByteCount)
        throw EInvalidArgument();
}

// index structure for k-dimensional array:
// data byte count
// nExpectedFailures, nVariables
// coefficient matrix (_numberOfDimensions x (nVariables + 1))
// start, end of variable 0
// ...
// start, end of variable nVariables - 1

void LocalArray::setElementsLinearMapping(const uint64_t *index,
        const char *input, uint16_t updateType) {
    uint64_t iValid = 0;

    // extract the linear mapping from the index
    int64_t *si = (int64_t *) index;
    uint64_t dataByteCount = index[0];
    uint64_t nExpectedFailures = index[1];
    uint64_t nVariables = index[2];
    if (nVariables == 0 || nVariables > 7)
        throw EInvalidArgument();
    int64_t coef[7][8];
    uint64_t start[7], end[7], position[7];
    unsigned j = 3;
    for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
        for (unsigned iVar = 0; iVar <= nVariables; iVar++)
            coef[iDim][iVar] = si[j++];
    }
    for (unsigned iVar = 0; iVar < nVariables; iVar++) {
        start[iVar] = index[j++];
        end[iVar] = index[j++];
        if (end[iVar] < start[iVar])
            throw EInvalidArgument();
        position[iVar] = start[iVar];
    }

    // iterate throw all positions within this range
    uint64_t nFailures = 0;
    while (position[0] <= end[0]) {
        // compute the point from the mapping
        int64_t x[7];
        bool pointValid = true;
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            x[iDim] = coef[iDim][nVariables];
            for (unsigned iVar = 0; iVar < nVariables; iVar++)
                x[iDim] += coef[iDim][iVar] * position[iVar];
            // validate the point
            if (x[iDim] < 0
                    || x[iDim]
                    >= (int64_t) _numberOfElementsAlongDimension[iDim])
                pointValid = false;
        }

        // copy this point or increase the failure count
        if (pointValid) {
            uint64_t offset = x[0];
            for (unsigned iDim = 1; iDim < _numberOfDimensions; iDim++) {
                offset *= _numberOfElementsAlongDimension[iDim];
                offset += x[iDim];
            }
            const char *validInput = input + iValid * _numberOfBytesPerElement;
            char *validData = _data + offset * _numberOfBytesPerElement;
            iValid++;
            updateElement(validInput, validData, updateType);
        } else
            nFailures++;

        // get next position
        for (int iVar = nVariables - 1; iVar >= 0; --iVar) {
            position[iVar]++;
            if (iVar == 0 || position[iVar] <= end[iVar])
                break;
            position[iVar] = start[iVar];
        }
    }

    if (nFailures != nExpectedFailures)
        throw EInvalidArgument();

    if (iValid * _numberOfBytesPerElement != dataByteCount)
        throw EInvalidArgument();
}

void LocalArray::getElements(IndexType indexType, const uint64_t *index,
        char *output) {
    switch (indexType) {
        case POINT_SEQUENCE:
        {
            getElementsPointSequence(index, output);
            break;
        }
        case POINT_TENSOR:
        {
            getElementsPointTensor(index, output);
            break;
        }
        case REGION_SEQUENCE:
        {
            getElementsRegionSequence(index, output);
            break;
        }
        case REGION_TENSOR:
        {
            getElementsRegionTensor(index, output);
            break;
        }
        case LINEAR_MAPPING:
        {
            getElementsLinearMapping(index, output);
            break;
        }
        default:
            throw EInvalidArgument();
            break;
    }
}

void LocalArray::getElements(LocalRequest *request, char *output) {
    if (NULL != dynamic_cast<LocalRequestPointSequence *> (request)) {
        getElements(POINT_SEQUENCE, request->getIndexList(), output);
    } else if (NULL != dynamic_cast<LocalRequestPointTensor *> (request)) {
        getElements(POINT_TENSOR, request->getIndexList(), output);
    } else if (NULL != dynamic_cast<LocalRequestRegionSequence *> (request)) {
        getElements(REGION_SEQUENCE, request->getIndexList(), output);
    } else if (NULL != dynamic_cast<LocalRequestRegionTensor *> (request)) {
        getElements(REGION_TENSOR, request->getIndexList(), output);
    } else {
        throw EInvalidArgument();
    }
}

void LocalArray::setElements(IndexType indexType, const uint64_t *index,
        const char *input, uint16_t updateType) {
    switch (indexType) {
        case POINT_SEQUENCE:
        {
            setElementsPointSequence(index, input, updateType);
            break;
        }
        case POINT_TENSOR:
        {
            setElementsPointTensor(index, input, updateType);
            break;
        }
        case REGION_SEQUENCE:
        {
            setElementsRegionSequence(index, input, updateType);
            break;
        }
        case REGION_TENSOR:
        {
            setElementsRegionTensor(index, input, updateType);
            break;
        }
        case LINEAR_MAPPING:
        {
            setElementsLinearMapping(index, input, updateType);
            break;
        }
        default:
            throw EInvalidArgument();
            break;
    }
}

void LocalArray::setElements(LocalRequest *request, const char *input,
        uint16_t updateType) {
    if (NULL != dynamic_cast<LocalRequestPointSequence *> (request)) {
        setElements(POINT_SEQUENCE, request->getIndexList(), input, updateType);
    } else if (NULL != dynamic_cast<LocalRequestPointTensor *> (request)) {
        setElements(POINT_TENSOR, request->getIndexList(), input, updateType);
    } else if (NULL != dynamic_cast<LocalRequestRegionSequence *> (request)) {
        setElements(REGION_SEQUENCE, request->getIndexList(), input,
                updateType);
    } else if (NULL != dynamic_cast<LocalRequestRegionTensor *> (request)) {
        setElements(REGION_TENSOR, request->getIndexList(), input, updateType);
    } else {
        throw EInvalidArgument();
    }
}


