/*
 * BSPLocalRequestPointSequence.hpp
 *
 *  Created on: 2014-8-21
 *      Author: junfeng
 */

#ifndef BSPLOCALREQUESTPOINTSEQUENCE_HPP_
#define BSPLOCALREQUESTPOINTSEQUENCE_HPP_

#include "BSPLocalRequest.hpp"
#include "BSPIndexSetPointSequence.hpp"

namespace BSP {

class LocalRequestPointSequence: public BSP::LocalRequest {
public:
	LocalRequestPointSequence(ArrayShape &shape, IndexSetPointSequence &indexSet);
	virtual ~LocalRequestPointSequence();
};

} /* namespace BSP */
#endif /* BSPLOCALREQUESTPOINTSEQUENCE_HPP_ */
