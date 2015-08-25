/*
 * BSPGlobalRequestLinearMapping.hpp
 *
 *  Created on: 2014-8-12
 *      Author: junfeng
 */

#ifndef BSPGLOBALREQUESTLINEARMAPPING_HPP_
#define BSPGLOBALREQUESTLINEARMAPPING_HPP_
#include "BSPGlobalRequest.hpp"

namespace BSP {

    class GlobalRequestLinearMapping : public GlobalRequest {
    public:
        GlobalRequestLinearMapping(ArrayPartition &partition,
                const unsigned numberOfVariables, const int64_t *matrix,
                const uint64_t *variableStart, const uint64_t *variableEnd,
                const std::string requestID);
        virtual ~GlobalRequestLinearMapping();
        virtual void getData(const uint64_t numberOfBytesPerElement,
                const uint64_t nData, char *data);
        virtual void setData(const uint64_t numberOfBytesPerElement,
                const uint64_t nData, const char *data);
    private:
        unsigned _numberOfVariables;
        int64_t _coefMatrix[7][8];
        uint64_t _variableStart[7];
        uint64_t _variableEnd[7];
    };

} /* namespace BSP */
#endif /* BSPGLOBALREQUESTLINEARMAPPING_HPP_ */
