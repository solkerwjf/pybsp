/*
 * BSPGrid.hpp
 *
 *  Created on: 2014-7-10
 *      Author: junfeng
 */

#ifndef BSPGRID_HPP_
#define BSPGRID_HPP_
#include <stdint.h>
#include "BSPException.hpp"

namespace BSP {

    class Grid {
    protected:
        unsigned _nDims;
        uint64_t _nProcsAlongDim[7];
        uint64_t _nProcs;
        uint64_t _startProcID;
        uint64_t _nProcsInGrid;
    public:
        const static unsigned ALL_DIMS = 7;
        Grid(const unsigned nDims, const uint64_t size[], const uint64_t nProcs,
                const uint64_t startProcID);
        Grid(const Grid &grid);
        Grid(uint64_t nProcs = 1);

        unsigned getNumberOfDimensions() const {
            return _nDims;
        }

        uint64_t getNumberOfProcesses() const {
            return _nProcs;
        }

        uint64_t getStartProcID() const {
            return _startProcID;
        }
        uint64_t getSize(const unsigned iDim) const;
        void getIndex(const uint64_t procID, uint64_t indexAlongDim[]) const;
        uint64_t getProcID(const uint64_t indexAlongDim[]) const;
        bool containsProc(const uint64_t procID) const;
        bool overlapWith(const Grid &grid) const;
        bool include(const Grid &grid) const;
        bool includeAll() const;
        void getPositionFromIProc(const uint64_t iProc, uint64_t position[]) const;
        void getPositionFromProcID(const uint64_t procID, uint64_t position[]) const;
        void operator=(const Grid& grid);
        bool operator==(const Grid& grid) const;
    };

}

#endif /* BSPGRID_HPP_ */
