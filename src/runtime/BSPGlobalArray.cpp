/*
 * BSPGlobalArray.cpp
 *
 *  Created on: 2014-8-21
 *      Author: junfeng
 */

#include "BSPGlobalArray.hpp"

namespace BSP {

GlobalArray::GlobalArray(ArrayRegistration &registration):
	ArrayPartition(registration) {
	_registration = &registration;
}

GlobalArray::~GlobalArray() {
	// TODO Auto-generated destructor stub
}

} /* namespace BSP */
