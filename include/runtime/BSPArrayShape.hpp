/*
 * BSPLocalArrayRef.hpp
 *
 *  Created on: 2014-8-5
 *      Author: junfeng
 */

#ifndef BSPLOCALARRAYREF_HPP_
#define BSPLOCALARRAYREF_HPP_
#include <stdint.h>
#include "BSPException.hpp"

namespace BSP {
class ArrayShape {
public:
	const static unsigned ALL_DIMS = 7;

	enum ElementType {
		INT8 = 0,
		INT16 = 1,
		INT32 = 2,
		INT64 = 3,
		UINT8 = 4,
		UINT16 = 5,
		UINT32 = 6,
		UINT64 = 7,
		FLOAT = 8,
		DOUBLE = 9,
                CINT8 = 10,
                CINT16 = 11,
                CINT32 = 12,
                CINT64 = 13,
                CUINT8 = 14,
                CUINT16 = 15,
                CUINT32 = 16,
                CUINT64 = 17,
                CFLOAT = 18,
                CDOUBLE = 19,
		BINARY = 20
	};
	const static unsigned SerializationSize = 9;

	ArrayShape(ElementType elementType, uint64_t numberOfBytesPerElement,
			unsigned numberOfDimensions,
			const uint64_t *numberOfElementsAlongDimension);
	ArrayShape(uint64_t *serialization);
        ArrayShape();

	virtual ~ArrayShape() {
	}
        
        /// @brief size of basic element type
        static uint64_t elementSize(ElementType elementType);

	/// @brief get element type
        ElementType getElementType();
        
        /// @brief get element count
	uint64_t getElementCount(unsigned iDim);

	/// @brief get number of dimensions
	unsigned getNumberOfDimensions() {
		return _numberOfDimensions;
	}

	uint64_t getNumberOfBytesPerElement() {
		return _numberOfBytesPerElement;
	}

	void serialize(uint64_t *serialization);

protected:
	void init(ElementType elementType, uint64_t numberOfBytesPerElement,
			unsigned numberOfDimensions,
			const uint64_t *numberOfElementsAlongDimension);
	ElementType _elementType;
	uint64_t _numberOfBytesPerElement;
	unsigned _numberOfDimensions;
	uint64_t _numberOfElementsAlongDimension[7];
	uint64_t _numberOfElementsInTotal;
};
}

#endif /* BSPLOCALARRAYREF_HPP_ */
