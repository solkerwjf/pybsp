/*
 * BSPPartition.hpp
 *
 *  Created on: 2014-8-5
 *      Author: junfeng
 */

#ifndef BSPARRAYPARTITION_HPP_
#define BSPARRAYPARTITION_HPP_
#include <stdint.h>
#include "BSPException.hpp"
#include "BSPArrayRegistration.hpp"
namespace BSP {

    class ArrayPartition {
    public:
        const static unsigned ALL_DIMS = 7;
        ArrayPartition(ArrayRegistration &registration);
        ArrayPartition(const ArrayPartition& partition);
        virtual ~ArrayPartition();
        uint64_t getProcCount(const unsigned iDim);
        uint64_t getElementCount(const unsigned iDim);
        uint64_t getIProc(const uint64_t elementIndex[]);
        uint64_t getProcID(const uint64_t elementIndex[]);
        uint64_t getIProcFromPosition(const uint64_t position[]);
        uint64_t getProcIDFromPosition(const uint64_t position[]);
        uint64_t getPosition(const unsigned iDim, const uint64_t index,
                const std::string requestID, const unsigned nVars,
                const uint64_t valueOfVar[]);
        void getPosition(const uint64_t elementIndex[], uint64_t position[],
                const std::string requestID, const unsigned nVars,
                const uint64_t valueOfVar[]);
        void getPositionFromIProc(const uint64_t iProc, uint64_t position[]);
        void getPositionFromProcID(const uint64_t procID, uint64_t position[]);
        uint64_t getNode(const unsigned iDim, const uint64_t position);

        uint64_t getStartProcID() {
            return _startProcID;
        }

        unsigned getNumberOfDimensions() {
            return _numberOfDimensions;
        }

        uint64_t getNumberOfBytesPerElement() {
            return _numberOfBytesPerElement;
        }
        ArrayShape::ElementType getElementType() const;
    protected:
        bool combineLocalArrays(Grid &grid, ArrayShape **localArrayRef);

        ArrayShape::ElementType _elementType;
        unsigned _numberOfDimensions;
        uint64_t _numberOfBytesPerElement;
        uint64_t _nProcsInGrid;
        uint64_t _nProcsAlongDim[7];
        uint64_t *_nodeAlongDim[7];
        uint64_t _startProcID;
    };

} /* namespace BSP */
#endif /* BSPPARTITION_HPP_ */
