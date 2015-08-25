/*
 * BSPLocalArray.hpp
 *
 *  Created on: 2014-7-17
 *      Author: junfeng
 */

#ifndef BSPLOCALARRAY_HPP_
#define BSPLOCALARRAY_HPP_
#include <string>
#include "BSPArrayShape.hpp"

namespace BSP {
class LocalRequest;
class ArrayRegistration;

/// @brief blockwise shared memory shared array of C$
class LocalArray: public ArrayShape {
	friend class ArrayRegistration;
public:
	/// @brief constructor with shape definition
	/// @param elementType the type of element
	/// @param numberOfBytesPerElement the number of bytes per element
	/// @param numberOfDimensions the number of the dimensions
	/// @param numberOfElementsAlongDimension the number of elements along each dimension
	/// @param path the path of the array
	LocalArray(std::string path, ElementType elementType,
			uint64_t numberOfBytesPerElement, unsigned numberOfDimensions,
			uint64_t *numberOfElementsAlongDimension);

	/// @brief constructor with shape serialization
	/// @param serialization the shape serialization
	/// @param path the path of the array
	LocalArray(std::string path, uint64_t *serialization);
        
        /// @brief null constructor for processes out of grid
        LocalArray();

	/// @brief destructor
	virtual ~LocalArray();

	/// @brief get byte count
	uint64_t getByteCount();

	/// @brief copy array
	/// @param array the array to copy
	void copy(LocalArray &array);

	char *getData() {
		return _data;
	}

        std::string getPath() const {
            return _path;
        }

	/// @brief fill
	void fill(char *value) throw (EInvalidArgument);

	enum IndexType {
		UNKNOWN_INDEX_TYPE,
		POINT_SEQUENCE,
		POINT_TENSOR,
		REGION_SEQUENCE,
		REGION_TENSOR,
		LINEAR_MAPPING
	};

	/// opID for requests
	const static uint16_t OPID_ASSIGN = 0;
	const static uint16_t OPID_ADD = 1;
	const static uint16_t OPID_MUL = 2;
	const static uint16_t OPID_AND = 3;
	const static uint16_t OPID_XOR = 4;
	const static uint16_t OPID_OR = 5;
	const static uint16_t OPID_MAX = 6;
	const static uint16_t OPID_MIN = 7;

	void getElements(IndexType indexType, const uint64_t *index, char *output);
	void getElements(LocalRequest *request, char *output);
	void setElements(IndexType indexType, const uint64_t *index,
			const char *input, uint16_t updateType = OPID_ASSIGN);
	void setElements(LocalRequest *request, const char *input,
			uint16_t updateType = OPID_ASSIGN);

	ArrayRegistration *getRegistration() {
		return _registration;
	}

private:
        std::string _path;
	char *_data;
	ArrayRegistration *_registration;

protected:
	void mapData();
	void unmapData();
	void getElementsPointSequence(const uint64_t *index, char *output);
	void getElementsPointTensor(const uint64_t *index, char *output);
	void getElementsRegionSequence(const uint64_t *index, char *output);
	void getElementsRegionTensor(const uint64_t *index, char *output);
	void getElementsLinearMapping(const uint64_t *index, char *output);
	void setElementsPointSequence(const uint64_t *index, const char *input,
			uint16_t updateType);
	void setElementsPointTensor(const uint64_t *index, const char *input,
			uint16_t updateType);
	void setElementsRegionSequence(const uint64_t *index, const char *input,
			uint16_t updateType);
	void setElementsRegionTensor(const uint64_t *index, const char *input,
			uint16_t updateType);
	void setElementsLinearMapping(const uint64_t *index, const char *input,
			uint16_t updateType);
	void updateElement(const char *src, char *dst, uint16_t updateType);
};

}

#endif /* BSPLOCALARRAY_HPP_ */
