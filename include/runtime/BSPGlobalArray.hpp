/*
 * BSPGlobalArray.hpp
 *
 *  Created on: 2014-8-21
 *      Author: junfeng
 */

#ifndef BSPGLOBALARRAY_HPP_
#define BSPGLOBALARRAY_HPP_

#include "BSPArrayPartition.hpp"

namespace BSP {

class GlobalArray: public BSP::ArrayPartition {
private:
	ArrayRegistration *_registration;
public:
	GlobalArray(ArrayRegistration &registration);
	virtual ~GlobalArray();
	ArrayRegistration *getRegistration() {return _registration;}
};

} /* namespace BSP */
#endif /* BSPGLOBALARRAY_HPP_ */
