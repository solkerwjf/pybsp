/*
 * BSPLocalArrayRef.cpp
 *
 *  Created on: 2014-8-5
 *      Author: junfeng
 */
#include "BSPArrayShape.hpp"
#include "BSPRuntime.hpp"
namespace BSP {

    ArrayShape::ArrayShape(ElementType elementType,
            uint64_t numberOfBytesPerElement, unsigned numberOfDimensions,
            const uint64_t *numberOfElementsAlongDimension) {
        init(elementType, numberOfBytesPerElement, numberOfDimensions,
                numberOfElementsAlongDimension);
    }

    ArrayShape::ArrayShape(uint64_t *serialization) {
        ElementType elementType = (ElementType) (serialization[0] >> 16);
        unsigned numberOfDimensions = (unsigned) (serialization[0] & 0xffff);
        uint64_t numberOfBytesPerElement = serialization[1];

        init(elementType, numberOfBytesPerElement, numberOfDimensions,
                serialization + 2);
    }

    const uint64_t nullConstructorSize_[1] = {1};

    ArrayShape::ArrayShape() {
        init(ArrayShape::BINARY, 1, 1, nullConstructorSize_);
    }

    void ArrayShape::init(ElementType elementType, uint64_t numberOfBytesPerElement,
            unsigned numberOfDimensions,
            const uint64_t *numberOfElementsAlongDimension) {
        _elementType = elementType;
        if (_elementType != BINARY) {
            if (numberOfBytesPerElement != elementSize(_elementType))
                throw EInvalidArgument();
        }
        if (numberOfBytesPerElement == 0 || numberOfDimensions == 0)
            throw EInvalidArgument();
        _numberOfBytesPerElement = numberOfBytesPerElement;
        _numberOfDimensions = numberOfDimensions;
        _numberOfElementsInTotal = 1;
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            if (numberOfElementsAlongDimension[iDim] == 0)
                throw EInvalidArgument();
            _numberOfElementsAlongDimension[iDim] =
                    numberOfElementsAlongDimension[iDim];
            _numberOfElementsInTotal *= _numberOfElementsAlongDimension[iDim];
        }
    }

    uint64_t ArrayShape::getElementCount(unsigned iDim) {
        if (iDim == ALL_DIMS)
            return _numberOfElementsInTotal;
        else if (iDim < _numberOfDimensions)
            return _numberOfElementsAlongDimension[iDim];
        else
            return 1;
    }

    void ArrayShape::serialize(uint64_t *serialization) {
        serialization[0] = _elementType;
        serialization[0] <<= 16;
        serialization[0] |= _numberOfDimensions;
        serialization[1] = _numberOfBytesPerElement;
        for (unsigned iDim = 0; iDim < _numberOfDimensions; iDim++) {
            serialization[2 + iDim] = _numberOfElementsAlongDimension[iDim];
        }
        for (unsigned iDim = _numberOfDimensions; iDim < 7; iDim++) {
            serialization[2 + iDim] = 1;
        }
    }

    ArrayShape::ElementType ArrayShape::getElementType() {
        return _elementType;
    }

    uint64_t ArrayShape::elementSize(ElementType elementType) {
        switch (elementType) {
            case INT8:
                return sizeof (int8_t);
                break;
            case INT16:
                return sizeof (int16_t);
                break;
            case INT32:
                return sizeof (int32_t);
                break;
            case INT64:
                return sizeof (int64_t);
                break;
            case UINT8:
                return sizeof (uint8_t);
                break;
            case UINT16:
                return sizeof (uint16_t);
                break;
            case UINT32:
                return sizeof (uint32_t);
                break;
            case UINT64:
                return sizeof (uint64_t);
                break;
            case FLOAT:
                return sizeof (float);
                break;
            case DOUBLE:
                return sizeof (double);
                break;
            case CINT8:
                return 2 * sizeof (int8_t);
                break;
            case CINT16:
                return 2 * sizeof (int16_t);
                break;
            case CINT32:
                return 2 * sizeof (int32_t);
                break;
            case CINT64:
                return 2 * sizeof (int64_t);
                break;
            case CUINT8:
                return 2 * sizeof (uint8_t);
                break;
            case CUINT16:
                return 2 * sizeof (uint16_t);
                break;
            case CUINT32:
                return 2 * sizeof (uint32_t);
                break;
            case CUINT64:
                return 2 * sizeof (uint64_t);
                break;
            case CFLOAT:
                return 2 * sizeof (float);
                break;
            case CDOUBLE:
                return 2 * sizeof (double);
                break;
            default:
                break;
        }
	return 0;
    }

}

