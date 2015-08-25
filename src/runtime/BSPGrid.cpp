/*
 * BSPGrid.cpp
 *
 *  Created on: 2014-7-11
 *      Author: junfeng
 */
#include "BSPGrid.hpp"
#include "BSPRuntime.hpp"

using namespace BSP;

Grid::Grid(const unsigned nDims, const uint64_t size[], const uint64_t nProcs,
        const uint64_t startProcID) {
    // set grid dim
    if (nDims == 0)
        throw EInvalidNDims();
    else {
        uint64_t gridSize = 1;
        for (unsigned i = 0; i < nDims; i++) {
            gridSize *= size[i];
        }
        if (gridSize == 0 || startProcID + gridSize > nProcs)
            throw EInvalidGrid();
        else {
            _nDims = nDims;
            _nProcs = nProcs;
            _startProcID = startProcID;
            for (unsigned i = 0; i < nDims; ++i)
                _nProcsAlongDim[i] = size[i];
            for (unsigned i = nDims; i < 7; ++i)
                _nProcsAlongDim[i] = 1;
            _nProcsInGrid = gridSize;
        }
    }
}

Grid::Grid(const Grid &grid) {
    _nDims = grid._nDims;
    _nProcs = grid._nProcs;
    for (unsigned i = 0; i < _nDims; i++)
        _nProcsAlongDim[i] = grid._nProcsAlongDim[i];
    _nProcsInGrid = grid._nProcsInGrid;
    _startProcID = grid._startProcID;
}

Grid::Grid(uint64_t nProcs) {
    _nDims = 1;
    _nProcs = nProcs;
    _startProcID = 0;
    _nProcsAlongDim[0] = nProcs;
    _nProcsInGrid = nProcs;
}

uint64_t Grid::getSize(const unsigned iDim) const {
    if (iDim == ALL_DIMS)
        return _nProcsInGrid;
    if (iDim >= _nDims)
        return 1;
    return _nProcsAlongDim[iDim];
}

void Grid::getIndex(const uint64_t procID, uint64_t indexAlongDim[]) const {
    indexAlongDim[_nDims - 1] = (procID + _nProcs - _startProcID) % _nProcs;
    for (unsigned i = _nDims - 1; i > 0; i--) {
        indexAlongDim[i - 1] = indexAlongDim[i] / _nProcsAlongDim[i];
        indexAlongDim[i] %= _nProcsAlongDim[i];
    }
}

uint64_t Grid::getProcID(const uint64_t indexAlongDim[]) const {
    uint64_t retval = 0;
    for (unsigned i = 0; i < _nDims; i++) {
        retval *= _nProcsAlongDim[i];
        retval += indexAlongDim[i];
    }
    retval += _startProcID;
    return retval;
}

bool Grid::containsProc(const uint64_t procID) const {
    return (procID >= _startProcID && procID < _startProcID + _nProcsInGrid);
}

void Grid::operator=(const Grid& grid) {
    _nDims = grid._nDims;
    _nProcs = grid._nProcs;
    _startProcID = grid._startProcID;
    for (unsigned i = 0; i < _nDims; i++)
        _nProcsAlongDim[i] = grid._nProcsAlongDim[i];
    _nProcsInGrid = grid._nProcsInGrid;
}

bool Grid::operator ==(const Grid& grid) const {
    std::cout << "comparing grids" << std::endl;
    if (_nDims != grid._nDims)
        return false;
    if (_startProcID != grid._startProcID)
        return false;
    for (unsigned i = 0; i < _nDims; ++i) {
        if (_nProcsAlongDim[i] != grid._nProcsAlongDim[i])
            return false;
    }
    if (_nProcsInGrid != grid._nProcsInGrid)
        return false;
    return true;
}

void Grid::getPositionFromIProc(const uint64_t iProc,
        uint64_t position[]) const {
    if (iProc > _nProcsInGrid)
        throw EInvalidArgument();
    uint64_t tempIProc = iProc;
    for (int iDim = _nDims - 1; iDim > 0; iDim--) {
        position[iDim] = tempIProc % _nProcsAlongDim[iDim];
        tempIProc /= _nProcsAlongDim[iDim];
    }
    position[0] = tempIProc;
}

void Grid::getPositionFromProcID(const uint64_t procID,
        uint64_t position[]) const {
    //if (procID < 0)
    //    throw EInvalidArgument();
    getPositionFromIProc(procID - _startProcID, position);
}

bool Grid::overlapWith(const Grid &grid) const {
    if (_startProcID < grid._startProcID) {
        if (_startProcID + _nProcsInGrid - 1 >= grid._startProcID)
            return true;
        else
            return false;
    } else {
        if (grid._startProcID + grid._nProcsInGrid - 1 >= _startProcID)
            return true;
        else
            return false;
    }
}

bool Grid::include(const Grid &grid) const {
    if (_startProcID <= grid._startProcID
            && _startProcID + _nProcsInGrid >= 
            grid._startProcID + grid._nProcsInGrid)
        return true;
    else
        return false;
}

bool Grid::includeAll() const {
    if (_startProcID == 0 && _nProcsInGrid == _nProcs)
        return true;
    else
        return false;
}
