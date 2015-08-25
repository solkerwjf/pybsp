/*
 * BSPArrayRegistratoin.cpp
 *
 *  Created on: 2014-8-23
 *      Author: junfeng
 */

#include "BSPArrayRegistration.hpp"
#include "BSPRuntime.hpp"
#include "BSPLocalArray.hpp"
#include "BSPGlobalRequest.hpp"
#include "BSPLocalRequest.hpp"
#include "BSPException.hpp"
using namespace BSP;

ArrayRegistration::ArrayRegistration(Grid &grid, ArrayShape **localArrayRef) :
Grid(grid) {
    if (localArrayRef == NULL)
        throw EInvalidArgument();

    _localArrayRef = new ArrayShape *[_nProcs];
    _localArrayRefInGrid = new ArrayShape *[_nProcsInGrid];
    if (_localArrayRef == NULL || _localArrayRefInGrid == NULL)
        throw ENotEnoughMemory();
    for (uint64_t procID = 0; procID < _nProcs; procID++)
        _localArrayRef[procID] = localArrayRef[procID];
    for (uint64_t iProc = 0; iProc < _nProcsInGrid; ++iProc) {
        _localArrayRefInGrid[iProc] = localArrayRef[iProc + _startProcID];
        if (_localArrayRefInGrid[iProc]->getElementType() !=
                _localArrayRefInGrid[0]->getElementType())
            throw EInvalidArgument();
    }

    Runtime *runtime = Runtime::getActiveRuntime();
    uint64_t myProcID = runtime->getMyProcessID();
    LocalArray *localArray =
            dynamic_cast<LocalArray *> (localArrayRef[myProcID]);
    if (localArray == NULL)
        throw EInvalidArgument();
    localArray->_registration = this;
    registerToRuntime();
}

ArrayRegistration::~ArrayRegistration() {
    unregister();
    //Runtime *runtime = Runtime::getActiveRuntime();
    //uint64_t myProcID = runtime->getMyProcessID();
    for (uint64_t procID = 0; procID < _nProcs; procID++) {
        LocalArray *localArray =
                dynamic_cast<LocalArray *> (_localArrayRef[procID]);
        if (localArray == NULL)
            delete _localArrayRef[procID];
        else
            localArray->_registration = NULL;
    }
    delete[] _localArrayRef;
    delete[] _localArrayRefInGrid;
}

void ArrayRegistration::registerToRuntime() {
    Runtime *runtime = Runtime::getActiveRuntime();
    _accessID = 0;
    ArrayRegistration *registration = runtime->_registration;
    if (registration == NULL) {
        runtime->_registration = this;
        _next = NULL;
        return;
    }

    ArrayRegistration **prevPointer = &(runtime->_registration);
    while (registration != NULL) {
        if (registration->_accessID > _accessID) {
            *prevPointer = this;
            _next = registration;
            return;
        }
        if (registration->_accessID == _accessID) {
            _accessID++;
            if (_accessID > MAX_ACCESS_ID)
                throw EAccessIDExhausted();
            prevPointer = &(registration->_next);
            registration = registration->_next;
        } else {
            throw ECorruptedRuntime();
        }
    }
    *prevPointer = this;
    _next = NULL;
}

void ArrayRegistration::unregister() {
    Runtime *runtime = Runtime::getActiveRuntime();
    ArrayRegistration *registration = runtime->_registration;
    if (registration == NULL) {
        throw ECorruptedRuntime();
    }

    ArrayRegistration **prevPointer = &(runtime->_registration);
    while (registration != NULL) {
        if (registration == this) {
            *prevPointer = _next;
            return;
        }
        prevPointer = &(registration->_next);
        registration = registration->_next;
    }
    throw ECorruptedRuntime();
}

void ArrayRegistration::addGlobalRequest(GlobalRequest *request) {
    if (request == NULL)
        throw EInvalidArgument();
    _globalRequests.push_back(request);
}

void ArrayRegistration::addLocalRequest(LocalRequest *request) {
    if (request == NULL)
        throw EInvalidArgument();
    _localRequests.push_back(request);
}

void ArrayRegistration::addIncomingMessage(Message *message) {
    if (message->getServer() != _accessID)
        throw EInvalidArgument();
    Runtime *runtime = Runtime::getActiveRuntime();
    uint16_t messageType = message->getType();
    if (messageType == Message::MESSAGE_DATA_REPLY) {
        _incomingReplies.push_back(message);
        runtime->_nIncomingReplies[message->getSenderProcID()]++;
    } else if (messageType == Message::MESSAGE_SET_POINT_SEQUENCE
            || messageType == Message::MESSAGE_SET_POINT_TENSOR
            || messageType == Message::MESSAGE_SET_REGION_SEQUENCE
            || messageType == Message::MESSAGE_SET_REGION_TENSOR
            || messageType == Message::MESSAGE_SET_LINEAR_MAPPING) {
        _incomingRequests.push_back(message);
    } else {
        if (messageType != Message::MESSAGE_DATA_UPDATE ||
                _incomingRequests.size() != _incomingUpdates.size() + 1)
            throw ECorruptedRuntime();
        _incomingUpdates.push_back(message);
    }
}

void ArrayRegistration::addOutgoingMessage(Message *message) {
    if (message->getServer() != _accessID)
        throw EInvalidArgument();
    _outgoingRequestsAndUpdates.push_back(message);
    Runtime *runtime = Runtime::getActiveRuntime();
    runtime->_nOutgoingRequestsAndUpdates[message->getReceiverProcID()]++;
}

void ArrayRegistration::clearMessagesAndRequests() {
    Runtime *runtime = Runtime::getActiveRuntime();
    uint64_t myProcID = runtime->getMyProcessID();

    size_t nIncomingReplies = _incomingReplies.size();
    for (size_t i = 0; i < nIncomingReplies; i++) {
        delete _incomingReplies[i];
    }
    _incomingReplies.clear();

    size_t nIncomingUpdates = _incomingUpdates.size();
    if (nIncomingUpdates != _incomingRequests.size())
        throw ECorruptedRuntime();
    for (size_t i = 0; i < nIncomingUpdates; i++) {
        LocalArray::IndexType indexType = LocalArray::UNKNOWN_INDEX_TYPE;
        switch (_incomingRequests[i]->getType()) {
            case Message::MESSAGE_SET_POINT_SEQUENCE:
            {
                indexType = LocalArray::POINT_SEQUENCE;
                break;
            }
            case Message::MESSAGE_SET_POINT_TENSOR:
            {
                indexType = LocalArray::POINT_TENSOR;
                break;
            }
            case Message::MESSAGE_SET_REGION_SEQUENCE:
            {
                indexType = LocalArray::REGION_SEQUENCE;
                break;
            }
            case Message::MESSAGE_SET_REGION_TENSOR:
            {
                indexType = LocalArray::REGION_TENSOR;
                break;
            }
            case Message::MESSAGE_SET_LINEAR_MAPPING:
            {
                indexType = LocalArray::LINEAR_MAPPING;
                break;
            }
            default:
            {
                break;
            }
        }
        if (indexType == LocalArray::UNKNOWN_INDEX_TYPE)
            throw ECorruptedRuntime();
        LocalArray *localArray =
                dynamic_cast<LocalArray *> (getArrayShape(myProcID));
        if (localArray == NULL)
            throw ECorruptedRuntime();

        localArray->setElements(indexType,
                (uint64_t *) _incomingRequests[i]->getData(),
                _incomingUpdates[i]->getData(),
                _incomingRequests[i]->getOperator());
    }
    _incomingRequests.clear();
    _incomingUpdates.clear();

    size_t nOutgoing = _outgoingRequestsAndUpdates.size();
    for (size_t i = 0; i < nOutgoing; i++) {
        delete _outgoingRequestsAndUpdates[i];
    }
    _outgoingRequestsAndUpdates.clear();

    size_t nGlobalRequests = _globalRequests.size();
    for (size_t i = 0; i < nGlobalRequests; ++i) {
        if (runtime->_replyReceivers.find(_globalRequests[i])
                != runtime->_replyReceivers.end()) {
            LocalArray *client = runtime->_replyReceivers[_globalRequests[i]];
            _globalRequests[i]->getData(client->getNumberOfBytesPerElement(),
                    client->getElementCount(LocalArray::ALL_DIMS),
                    client->getData());
        }
        delete _globalRequests[i];
    }
    _globalRequests.clear();

    size_t nLocalRequests = _localRequests.size();
    for (size_t i = 0; i < nLocalRequests; ++i) {
        delete _localRequests[i];
    }
    _localRequests.clear();
}

void ArrayRegistration::getHeaders(uint64_t procID, uint64_t *headers,
        uint64_t& offset) {
    Runtime *runtime = Runtime::getActiveRuntime();
    if (procID >= runtime->getNumberOfProcesses())
        throw EInvalidArgument();
    if ((offset & 1) != 0)
        throw EInvalidArgument();
    size_t n = _outgoingRequestsAndUpdates.size();
    for (size_t i = 0; i < n; i++) {
        if (_outgoingRequestsAndUpdates[i]->getReceiverProcID() != procID)
            continue;
        headers[offset++] = _outgoingRequestsAndUpdates[i]->getTSCO();
        headers[offset++] = _outgoingRequestsAndUpdates[i]->getByteCount();
    }
}

void ArrayRegistration::sendRequestsAndUpdates(uint64_t procID) {
    Runtime *runtime = Runtime::getActiveRuntime();
    if (procID >= runtime->getNumberOfProcesses())
        throw EInvalidArgument();
    size_t n = _outgoingRequestsAndUpdates.size();
    for (size_t i = 0; i < n; i++) {
        if (_outgoingRequestsAndUpdates[i]->getReceiverProcID() != procID)
            continue;
        runtime->send(_outgoingRequestsAndUpdates[i]->getByteCount(),
                _outgoingRequestsAndUpdates[i]->getData());
    }
}

void ArrayRegistration::receiveReplies(uint64_t procID) {
    Runtime *runtime = Runtime::getActiveRuntime();
    if (procID >= runtime->getNumberOfProcesses())
        throw EInvalidArgument();
    size_t n = _incomingReplies.size();
    for (size_t i = 0; i < n; i++) {
        if (_incomingReplies[i]->getSenderProcID() != procID)
            continue;
        runtime->receive(_incomingReplies[i]->getByteCount(),
                _incomingReplies[i]->getData());
    }
}

ArrayShape::ElementType ArrayRegistration::getElementType() {
    return _localArrayRefInGrid[0]->getElementType();
}
