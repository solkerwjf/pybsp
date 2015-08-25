/*
 * BSPRuntime.cpp
 *
 *  Created on: 2014-7-11
 *      Author: junfeng
 */

#include <cstring>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include "BSPNamedObject.hpp"
#include "BSPMessage.hpp"
#include "BSPGlobalArray.hpp"
#include "BSPLocalArray.hpp"
#include "BSPIndexSetPointSequence.hpp"
#include "BSPIndexSetPointTensor.hpp"
#include "BSPIndexSetRegionSequence.hpp"
#include "BSPIndexSetRegionTensor.hpp"
#include "BSPGlobalRequestPointSequence.hpp"
#include "BSPGlobalRequestPointTensor.hpp"
#include "BSPGlobalRequestRegionSequence.hpp"
#include "BSPGlobalRequestRegionTensor.hpp"
#include "BSPGlobalRequestLinearMapping.hpp"
#include "BSPLocalRequestPointSequence.hpp"
#include "BSPLocalRequestPointTensor.hpp"
#include "BSPLocalRequestRegionSequence.hpp"
#include "BSPLocalRequestRegionTensor.hpp"
#include "BSPIndexSet.hpp"
#include "BSPRuntime.hpp"

using namespace BSP;
Runtime *Runtime::_activeRuntimeObject = NULL;


/// @brief constructor
/// @param pArgc the pointer to argc of main()
/// @param pArgv the pointer to argv of main()

Runtime::Runtime(int *pArgc, char ***pArgv) :
_nal(pArgc, pArgv, 1 << 29, 65536), _grid(
_nal.getNumberOfProcesses()) {
    _finalizing = false;
    assert(_activeRuntimeObject == NULL);
    _activeRuntimeObject = this;
    _verbose = false;
    _nProcs = _nal.getNumberOfProcesses();
    _myProcID = _nal.getProcessRank();

    _registration = NULL;
    _nOutgoingRequestsAndUpdates = new uint64_t[_nProcs];
    _nIncomingReplies = new uint64_t[_nProcs];
    if (_nOutgoingRequestsAndUpdates == NULL || _nIncomingReplies == NULL) {
        throw ENotEnoughMemory();
    }
    for (uint64_t procID = 0; procID < _nProcs; procID++) {
        _nOutgoingRequestsAndUpdates[procID] = 0;
        _nIncomingReplies[procID] = 0;
    }
    _incomingUserDefinedMessages = new std::vector<Message*>[_nProcs];
    _outgoingUserDefinedMessages = new std::vector<Message*>[_nProcs];
    if (_incomingUserDefinedMessages == NULL
            || _outgoingUserDefinedMessages == NULL)
        throw ENotEnoughMemory();

    _incomingUserArrayNames = new std::stringstream[_nProcs];
    _outgoingUserArrayNames = new std::stringstream[_nProcs];
    if (_incomingUserArrayNames == NULL || _outgoingUserArrayNames == NULL)
        throw ENotEnoughMemory();
}

/// @brief destructor

Runtime::~Runtime() {
    _finalizing = true;
    _nal.finalize();
    _this.clear();
    _activeRuntimeObject = NULL;
    delete[] _nOutgoingRequestsAndUpdates;
    delete[] _nIncomingReplies;
    delete[] _outgoingUserDefinedMessages;
    delete[] _incomingUserDefinedMessages;
    delete[] _incomingUserArrayNames;
    delete[] _outgoingUserArrayNames;
}

void Runtime::abort() {
    _finalizing = true;
    _this.clear();
    _nal.abort();
}

/// @brief request data from/to global array

void Runtime::requestFrom(GlobalArray& server, IndexSet& indexSet,
        LocalArray & client, const std::string requestID) {
    if (server.getRegistration() == NULL
            || client.getNumberOfBytesPerElement()
            != server.getNumberOfBytesPerElement()
            )
        throw EInvalidArgument();
    if (client.getElementCount(LocalArray::ALL_DIMS)
            < indexSet.getNumberOfIndices())
        throw EClientArrayTooSmall(requestID,
            client.getElementCount(LocalArray::ALL_DIMS),
            indexSet.getNumberOfIndices());

    // create the request
    uint16_t messageType = Message::MESSAGE_TYPE_MAX + 1;
    ArrayRegistration * registration = server.getRegistration();
    IndexSet * pIndexSet = &indexSet;
    GlobalRequest * request = NULL;
    if (dynamic_cast<IndexSetPointSequence *> (pIndexSet) != NULL) {
        IndexSetPointSequence *myIndexSet =
                dynamic_cast<IndexSetPointSequence *> (pIndexSet);
        request = new GlobalRequestPointSequence(server, *myIndexSet, requestID);
        messageType = Message::MESSAGE_GET_POINT_SEQUENCE;
    } else if (dynamic_cast<IndexSetPointTensor *> (pIndexSet) != NULL) {
        IndexSetPointTensor *myIndexSet =
                dynamic_cast<IndexSetPointTensor *> (pIndexSet);
        request = new GlobalRequestPointTensor(server, *myIndexSet, requestID);
        messageType = Message::MESSAGE_GET_POINT_TENSOR;
    } else if (dynamic_cast<IndexSetRegionSequence *> (pIndexSet) != NULL) {
        IndexSetRegionSequence *myIndexSet =
                dynamic_cast<IndexSetRegionSequence *> (pIndexSet);
        request = new GlobalRequestRegionSequence(server, *myIndexSet, requestID);
        messageType = Message::MESSAGE_GET_REGION_SEQUENCE;
    } else if (dynamic_cast<IndexSetRegionTensor *> (pIndexSet) != NULL) {
        IndexSetRegionTensor *myIndexSet =
                dynamic_cast<IndexSetRegionTensor *> (pIndexSet);
        request = new GlobalRequestRegionTensor(server, *myIndexSet, requestID);
        messageType = Message::MESSAGE_GET_REGION_TENSOR;
    }
    _replyReceivers[request] = &client;
    uint16_t clientID = ArrayRegistration::MAX_ACCESS_ID + 1;
    if (client.getRegistration() != NULL)
        clientID = client.getRegistration()->getAccessID();

    registration->addGlobalRequest(request);
    // request->setRequestID(requestID);
    // create the request messages and the reply messages
    uint64_t startProcID = request->getStartProcID();
    uint64_t nProcsInGrid = request->getProcCount(Grid::ALL_DIMS);
    for (uint64_t procID = startProcID; procID < startProcID + nProcsInGrid;
            procID++) {
        uint64_t indexLength = request->getIndexLength(procID);
        if (indexLength == 0)

            continue;

        Message * requestMessage = new Message(_myProcID, procID, messageType,
                registration->getAccessID(), clientID,
                LocalArray::OPID_ASSIGN, indexLength * sizeof (uint64_t),
                (char*) request->getIndexList(procID), false);
        registration->addOutgoingMessage(requestMessage);
        Message * replyMessage = new Message(procID, _myProcID,
                Message::MESSAGE_DATA_REPLY, registration->getAccessID(),
                clientID, LocalArray::OPID_ASSIGN,
                request->getDataCount(procID)
                * request->getNumberOfBytesPerElement(),
                request->getDataList(procID), false);
        registration->addIncomingMessage(replyMessage);
    }
}

void Runtime::requestTo(GlobalArray& server, IndexSet& indexSet,
        LocalArray& client, uint16_t opID, const std::string requestID) {
    if (server.getRegistration() == NULL
            || client.getNumberOfBytesPerElement()
            != server.getNumberOfBytesPerElement()
            )
        throw EInvalidArgument();
    if (client.getElementCount(LocalArray::ALL_DIMS)
            < indexSet.getNumberOfIndices())
        throw EClientArrayTooSmall(requestID,
            client.getElementCount(LocalArray::ALL_DIMS),
            indexSet.getNumberOfIndices());

    // create the request
    uint16_t messageType = Message::MESSAGE_TYPE_MAX + 1;
    ArrayRegistration * registration = server.getRegistration();
    IndexSet * pIndexSet = &indexSet;
    GlobalRequest * request = NULL;
    if (dynamic_cast<IndexSetPointSequence *> (pIndexSet) != NULL) {
        IndexSetPointSequence *myIndexSet =
                dynamic_cast<IndexSetPointSequence *> (pIndexSet);
        request = new GlobalRequestPointSequence(server, *myIndexSet, requestID);
        messageType = Message::MESSAGE_SET_POINT_SEQUENCE;
    } else if (dynamic_cast<IndexSetPointTensor *> (pIndexSet) != NULL) {
        IndexSetPointTensor *myIndexSet =
                dynamic_cast<IndexSetPointTensor *> (pIndexSet);
        request = new GlobalRequestPointTensor(server, *myIndexSet, requestID);
        messageType = Message::MESSAGE_SET_POINT_TENSOR;
    } else if (dynamic_cast<IndexSetRegionSequence *> (pIndexSet) != NULL) {
        IndexSetRegionSequence *myIndexSet =
                dynamic_cast<IndexSetRegionSequence *> (pIndexSet);
        request = new GlobalRequestRegionSequence(server, *myIndexSet, requestID);
        messageType = Message::MESSAGE_SET_REGION_SEQUENCE;
    } else if (dynamic_cast<IndexSetRegionTensor *> (pIndexSet) != NULL) {
        IndexSetRegionTensor *myIndexSet =
                dynamic_cast<IndexSetRegionTensor *> (pIndexSet);
        request = new GlobalRequestRegionTensor(server, *myIndexSet, requestID);
        messageType = Message::MESSAGE_SET_REGION_TENSOR;
    }
    uint16_t clientID = ArrayRegistration::MAX_ACCESS_ID + 1;
    if (client.getRegistration() != NULL)
        clientID = client.getRegistration()->getAccessID();

    registration->addGlobalRequest(request);
    //request->setRequestID(requestID);
    // fill the data list
    request->setData(client.getNumberOfBytesPerElement(),
            client.getElementCount(LocalArray::ALL_DIMS), client.getData());
    // create the request messages and the update messages
    uint64_t startProcID = request->getStartProcID();
    uint64_t nProcsInGrid = request->getProcCount(Grid::ALL_DIMS);
    for (uint64_t procID = startProcID; procID < startProcID + nProcsInGrid;
            procID++) {
        uint64_t indexLength = request->getIndexLength(procID);
        if (indexLength == 0)

            continue;

        Message * requestMessage = new Message(_myProcID, procID, messageType,
                registration->getAccessID(), clientID, opID,
                indexLength * sizeof (uint64_t),
                (char*) request->getIndexList(procID), false);
        registration->addOutgoingMessage(requestMessage);
        Message * updateMessage = new Message(_myProcID, procID,
                Message::MESSAGE_DATA_UPDATE, registration->getAccessID(),
                clientID, opID,
                request->getDataCount(procID)
                * request->getNumberOfBytesPerElement(),
                request->getDataList(procID), false);
        registration->addOutgoingMessage(updateMessage);
    }
}

/// @brief request data from/to local array

void Runtime::requestFrom(LocalArray& server, uint64_t serverProcID,
        IndexSet& indexSet, LocalArray & client, const std::string requestID) {
    if (server.getRegistration() == NULL
            || client.getNumberOfBytesPerElement()
            != server.getNumberOfBytesPerElement()
            )
        throw EInvalidArgument();
    if (client.getElementCount(LocalArray::ALL_DIMS)
            < indexSet.getNumberOfIndices())
        throw EClientArrayTooSmall(requestID,
            client.getElementCount(LocalArray::ALL_DIMS),
            indexSet.getNumberOfIndices());

    // create the request
    uint16_t messageType = Message::MESSAGE_TYPE_MAX + 1;
    ArrayRegistration * registration = server.getRegistration();
    ArrayShape * shape = registration->getArrayShape(serverProcID);
    IndexSet * pIndexSet = &indexSet;
    LocalRequest * request = NULL;
    if (dynamic_cast<IndexSetPointSequence *> (pIndexSet) != NULL) {
        IndexSetPointSequence *myIndexSet =
                dynamic_cast<IndexSetPointSequence *> (pIndexSet);
        request = new LocalRequestPointSequence(*shape, *myIndexSet);
        messageType = Message::MESSAGE_GET_POINT_SEQUENCE;
    } else if (dynamic_cast<IndexSetPointTensor *> (pIndexSet) != NULL) {
        IndexSetPointTensor *myIndexSet =
                dynamic_cast<IndexSetPointTensor *> (pIndexSet);
        request = new LocalRequestPointTensor(*shape, *myIndexSet);
        messageType = Message::MESSAGE_GET_POINT_TENSOR;
    } else if (dynamic_cast<IndexSetRegionSequence *> (pIndexSet) != NULL) {
        IndexSetRegionSequence *myIndexSet =
                dynamic_cast<IndexSetRegionSequence *> (pIndexSet);
        request = new LocalRequestRegionSequence(*shape, *myIndexSet);
        messageType = Message::MESSAGE_GET_REGION_SEQUENCE;
    } else if (dynamic_cast<IndexSetRegionTensor *> (pIndexSet) != NULL) {
        IndexSetRegionTensor *myIndexSet =
                dynamic_cast<IndexSetRegionTensor *> (pIndexSet);
        request = new LocalRequestRegionTensor(*shape, *myIndexSet);
        messageType = Message::MESSAGE_GET_REGION_TENSOR;
    }
    uint16_t clientID = ArrayRegistration::MAX_ACCESS_ID + 1;
    if (client.getRegistration() != NULL)
        clientID = client.getRegistration()->getAccessID();

    registration->addLocalRequest(request);
    request->setRequestID(requestID);

    // create the request messages and the reply messages
    uint64_t indexLength = request->getIndexLength();
    if (indexLength == 0)
        return;
    if (client.getElementCount(LocalArray::ALL_DIMS) < request->getDataCount())
        throw EClientArrayTooSmall(requestID,
            client.getElementCount(LocalArray::ALL_DIMS),
            request->getDataCount());

    Message * requestMessage = new Message(_myProcID, serverProcID, messageType,
            registration->getAccessID(),
            clientID, LocalArray::OPID_ASSIGN,
            indexLength * sizeof (uint64_t),
            (char*) request->getIndexList(), false);
    registration->addOutgoingMessage(requestMessage);
    uint64_t dataLength = request->getDataCount()
            * request->getNumberOfBytesPerElement();
    char* replyData = client.getData();

    Message * replyMessage = new Message(serverProcID, _myProcID,
            Message::MESSAGE_DATA_REPLY, registration->getAccessID(),
            clientID, LocalArray::OPID_ASSIGN,
            dataLength, replyData, false);
    registration->addIncomingMessage(replyMessage);
}

void Runtime::requestTo(LocalArray& server, uint64_t serverProcID,
        IndexSet& indexSet, LocalArray& client, uint16_t opID,
        const std::string requestID) {
    if (server.getRegistration() == NULL
            || client.getNumberOfBytesPerElement()
            != server.getNumberOfBytesPerElement()
            )
        throw EInvalidArgument();
    if (client.getElementCount(LocalArray::ALL_DIMS)
            < indexSet.getNumberOfIndices())
        throw EClientArrayTooSmall(requestID,
            client.getElementCount(LocalArray::ALL_DIMS),
            indexSet.getNumberOfIndices());

    // create the request
    uint16_t messageType = Message::MESSAGE_TYPE_MAX + 1;
    ArrayRegistration * registration = server.getRegistration();
    ArrayShape * shape = registration->getArrayShape(serverProcID);
    IndexSet * pIndexSet = &indexSet;
    LocalRequest * request = NULL;
    if (dynamic_cast<IndexSetPointSequence *> (pIndexSet) != NULL) {
        IndexSetPointSequence *myIndexSet =
                dynamic_cast<IndexSetPointSequence *> (pIndexSet);
        request = new LocalRequestPointSequence(*shape, *myIndexSet);
        messageType = Message::MESSAGE_SET_POINT_SEQUENCE;
    } else if (dynamic_cast<IndexSetPointTensor *> (pIndexSet) != NULL) {
        IndexSetPointTensor *myIndexSet =
                dynamic_cast<IndexSetPointTensor *> (pIndexSet);
        request = new LocalRequestPointTensor(*shape, *myIndexSet);
        messageType = Message::MESSAGE_SET_POINT_TENSOR;
    } else if (dynamic_cast<IndexSetRegionSequence *> (pIndexSet) != NULL) {
        IndexSetRegionSequence *myIndexSet =
                dynamic_cast<IndexSetRegionSequence *> (pIndexSet);
        request = new LocalRequestRegionSequence(*shape, *myIndexSet);
        messageType = Message::MESSAGE_SET_REGION_SEQUENCE;
    } else if (dynamic_cast<IndexSetRegionTensor *> (pIndexSet) != NULL) {
        IndexSetRegionTensor *myIndexSet =
                dynamic_cast<IndexSetRegionTensor *> (pIndexSet);
        request = new LocalRequestRegionTensor(*shape, *myIndexSet);
        messageType = Message::MESSAGE_SET_REGION_TENSOR;
    }
    uint16_t clientID = ArrayRegistration::MAX_ACCESS_ID + 1;
    if (client.getRegistration() != NULL)
        clientID = client.getRegistration()->getAccessID();

    registration->addLocalRequest(request);
    request->setRequestID(requestID);
    // create the request messages and the update messages
    uint64_t indexLength = request->getIndexLength();
    if (indexLength == 0)
        return;

    Message * requestMessage = new Message(_myProcID, serverProcID, messageType,
            registration->getAccessID(),
            clientID, opID,
            indexLength * sizeof (uint64_t),
            (char*) request->getIndexList(), false);
    registration->addOutgoingMessage(requestMessage);

    if (request->getDataCount() > client.getElementCount(LocalArray::ALL_DIMS))
        throw EClientArrayTooSmall(requestID,
            client.getElementCount(LocalArray::ALL_DIMS),
            request->getDataCount());

    uint64_t dataLength = request->getDataCount()
            * request->getNumberOfBytesPerElement();
    Message * updateMessage = new Message(serverProcID, _myProcID,
            Message::MESSAGE_DATA_REPLY, registration->getAccessID(),
            clientID, opID, dataLength,
            client.getData(), false);
    registration->addOutgoingMessage(updateMessage);
}

void Runtime::requestFrom(GlobalArray& server, const unsigned numberOfVariables,
        const int64_t* matrix, const uint64_t* variableStart,
        const uint64_t* variableEnd, LocalArray & client,
        const std::string requestID) {
    uint64_t dataCount = 1;
    for (unsigned iVar = 0; iVar < numberOfVariables; iVar++) {
        if (variableEnd[iVar] < variableStart[iVar])
            throw EInvalidArgument();
        dataCount *= variableEnd[iVar] - variableStart[iVar] + 1;
    }
    if (server.getRegistration() == NULL
            || client.getNumberOfBytesPerElement()
            != server.getNumberOfBytesPerElement()
            )
        throw EInvalidArgument();
    if (client.getElementCount(LocalArray::ALL_DIMS) < dataCount)
        throw EClientArrayTooSmall(requestID,
            client.getElementCount(LocalArray::ALL_DIMS),
            dataCount);
    uint16_t clientID = ArrayRegistration::MAX_ACCESS_ID + 1;
    if (client.getRegistration() != NULL)
        clientID = client.getRegistration()->getAccessID();

    // create the request
    GlobalRequestLinearMapping * request = new GlobalRequestLinearMapping(server,
            numberOfVariables, matrix, variableStart, variableEnd, requestID);
    ArrayRegistration * registration = server.getRegistration();
    registration->addGlobalRequest(request);
    //request->setRequestID(requestID);
    _replyReceivers[request] = &client;

    // create the request messages and the reply messages
    uint64_t startProcID = request->getStartProcID();
    uint64_t nProcsInGrid = request->getProcCount(Grid::ALL_DIMS);
    for (uint64_t procID = startProcID; procID < startProcID + nProcsInGrid;
            procID++) {
        uint64_t indexLength = request->getIndexLength(procID);
        if (indexLength == 0)

            continue;

        Message * requestMessage = new Message(_myProcID, procID,
                Message::MESSAGE_GET_LINEAR_MAPPING,
                registration->getAccessID(), clientID,
                LocalArray::OPID_ASSIGN, indexLength * sizeof (uint64_t),
                (char*) request->getIndexList(procID), false);
        registration->addOutgoingMessage(requestMessage);
        Message * replyMessage = new Message(procID, _myProcID,
                Message::MESSAGE_DATA_REPLY, registration->getAccessID(),
                clientID, LocalArray::OPID_ASSIGN,
                request->getDataCount(procID)
                * request->getNumberOfBytesPerElement(),
                request->getDataList(procID), false);
        registration->addIncomingMessage(replyMessage);
    }
}

void Runtime::requestTo(GlobalArray& server, const unsigned numberOfVariables,
        const int64_t* matrix, const uint64_t* variableStart,
        const uint64_t* variableEnd, LocalArray& client, uint16_t opID,
        const std::string requestID) {
    uint64_t dataCount = 1;
    for (unsigned iVar = 0; iVar < numberOfVariables; iVar++) {
        if (variableEnd[iVar] < variableStart[iVar])
            throw EInvalidArgument();
        dataCount *= variableEnd[iVar] - variableStart[iVar] + 1;
    }
    if (server.getRegistration() == NULL
            || client.getNumberOfBytesPerElement()
            != server.getNumberOfBytesPerElement()
            )
        throw EInvalidArgument();
    if (client.getElementCount(LocalArray::ALL_DIMS) < dataCount)
        throw EClientArrayTooSmall(requestID,
            client.getElementCount(LocalArray::ALL_DIMS),
            dataCount);
    uint16_t clientID = ArrayRegistration::MAX_ACCESS_ID + 1;
    if (client.getRegistration() != NULL)
        clientID = client.getRegistration()->getAccessID();

    // create the request
    GlobalRequestLinearMapping * request = new GlobalRequestLinearMapping(server,
            numberOfVariables, matrix, variableStart, variableEnd, requestID);
    ArrayRegistration * registration = server.getRegistration();
    registration->addGlobalRequest(request);
    // request->setRequestID(requestID);
    // fill the data list
    request->setData(client.getNumberOfBytesPerElement(),
            client.getElementCount(LocalArray::ALL_DIMS), client.getData());
    // create the request messages and the update messages
    uint64_t startProcID = request->getStartProcID();
    uint64_t nProcsInGrid = request->getProcCount(Grid::ALL_DIMS);
    for (uint64_t procID = startProcID; procID < startProcID + nProcsInGrid;
            procID++) {
        uint64_t indexLength = request->getIndexLength(procID);
        if (indexLength == 0)

            continue;

        Message * requestMessage = new Message(_myProcID, procID,
                Message::MESSAGE_SET_LINEAR_MAPPING,
                registration->getAccessID(), clientID, opID,
                indexLength * sizeof (uint64_t),
                (char*) request->getIndexList(procID), false);
        registration->addOutgoingMessage(requestMessage);
        Message * updateMessage = new Message(_myProcID, procID,
                Message::MESSAGE_DATA_UPDATE, registration->getAccessID(),
                clientID, opID, request->getDataCount(procID)
                * request->getNumberOfBytesPerElement(),
                request->getDataList(procID), false);
        registration->addOutgoingMessage(updateMessage);
    }
}

void Runtime::exportUserDefined(uint64_t procID, std::string path) {
    if (procID >= _nProcs)
        throw EInvalidProcID(procID);

    std::string objectPath;
    if ((int) path.find("this", 0) < 0 && (int) path.find_first_of("\r\n\t ") < 0)
        objectPath = path;
    else
        objectPath = simplifyPath(path);
    NamedObject* nobject = getObject(objectPath);
    if (NULL == nobject)
        throw EInvalidArgument();

    if (nobject->getType() != ARRAY || nobject->isGlobal())
        throw EInvalidArgument();

    _outgoingUserArrayNames[procID] << objectPath << ";";
}

bool* Runtime::computeConnectivity(bool* MatrixOfSendTo) {
    // compute the connectivity matrix
    bool* connectivity = new bool[_nProcs * _nProcs];
    for (uint64_t senderID = 0; senderID < _nProcs; senderID++) {
        for (uint64_t receiverID = 0; receiverID < _nProcs; receiverID++) {
            bool sendConnect = MatrixOfSendTo[senderID * _nProcs + receiverID];
            if (senderID == _myProcID
                    && _nOutgoingRequestsAndUpdates[receiverID] > 0
                    && !sendConnect) {
                throw EInvalidCustomizedSync(
                        (unsigned long) senderID,
                        (unsigned long) receiverID,
                        (unsigned long) _nOutgoingRequestsAndUpdates[receiverID]
                        );
            }


            bool receiveConnect =
                    MatrixOfSendTo[receiverID * _nProcs + senderID];
            connectivity[senderID * _nProcs + receiverID] = sendConnect
                    || receiveConnect;
        }
    }

    return connectivity;
}

void Runtime::fillMyMessageHeader(uint64_t& partnerID,
        uint64_t*& myMessageHeader) {
    // iterate through the array registrations
    uint64_t offset = 0;
    ArrayRegistration* registration = _registration;
    while (registration != NULL) {
        // get the headers of the outgoing messages targeting this partner
        registration->getHeaders(partnerID, myMessageHeader, offset);
        registration = registration->getNext();
    }
    // iterate through the outgoing user-defined messages
    size_t nOutgoingUserDefined =
            _outgoingUserDefinedMessages[partnerID].size();
    for (size_t i = 0; i < nOutgoingUserDefined; i++) {

        myMessageHeader[offset++] =
                _outgoingUserDefinedMessages[partnerID][i]->getTSCO();
        myMessageHeader[offset++] =
                _outgoingUserDefinedMessages[partnerID][i]->getByteCount();
    }
}

void Runtime::exchangeMessageHeaders(uint64_t& myReqUpdCount,
        uint64_t*& myMessageHeader, uint64_t& partnerID,
        uint64_t& partnerReqUpdCount, uint64_t*& partnerMessageHeader) {
    if (myReqUpdCount > 0) {
        myMessageHeader = new uint64_t[2 * myReqUpdCount];
        if (myMessageHeader == NULL)
            throw ENotEnoughMemory();

        fillMyMessageHeader(partnerID, myMessageHeader);
        send(2 * myReqUpdCount * sizeof (uint64_t), myMessageHeader);
    }
    if (partnerReqUpdCount > 0) {
        partnerMessageHeader = new uint64_t[2 * partnerReqUpdCount];

        if (partnerMessageHeader == NULL)
            throw ENotEnoughMemory();
        receive(2 * partnerReqUpdCount * sizeof (uint64_t),
                partnerMessageHeader);
    }
    exchangeWith(partnerID);
}

std::vector<Message*> Runtime::exchangeRequestsAndUpdates(
        uint64_t myReqUpdCount, uint64_t& partnerID,
        uint64_t& partnerReqUpdCount, uint64_t * partnerMessageHeader) {
    // exchange requests and updates
    if (myReqUpdCount > 0) {
        // iterate through the array registrations
        ArrayRegistration *registration = _registration;
        while (registration != NULL) {
            registration->sendRequestsAndUpdates(partnerID);
            registration = registration->getNext();
        }

        // iterate through the outgoing user-defined messages
        size_t nOutgoingUserDefined =
                _outgoingUserDefinedMessages[partnerID].size();
        for (size_t i = 0; i < nOutgoingUserDefined; i++) {
            send(_outgoingUserDefinedMessages[partnerID][i]->getByteCount(),
                    _outgoingUserDefinedMessages[partnerID][i]->getData());
        }
    }
    std::vector<Message*> partnerMessages;
    for (uint64_t i = 0; i < partnerReqUpdCount; i++) {
        uint64_t messageLength = partnerMessageHeader[(i << 1) + 1];
        char *messageData = new char[messageLength];
        if (messageData == NULL)
            throw ENotEnoughMemory();
        Message * message = new Message(partnerID, _myProcID,
                partnerMessageHeader[i << 1], messageLength, messageData, true);
        if (message == NULL)
            throw ENotEnoughMemory();
        partnerMessages.push_back(message);
        receive(messageLength, messageData);
    }
    exchangeWith(partnerID);

    return partnerMessages;
}

uint64_t Runtime::receiveMyReplies(uint64_t & partnerID) {
    // generate and exchange replies
    uint64_t myReplyCount = _nIncomingReplies[partnerID];
    if (myReplyCount > 0) {
        ArrayRegistration *registration = _registration;
        while (registration != NULL) {

            registration->receiveReplies(partnerID);
            registration = registration->getNext();
        }
    }
    return myReplyCount;
}

void Runtime::sendRepliesToPartner(uint64_t& partnerReqUpdCount,
        std::vector<Message*>& partnerMessages, uint64_t& partnerID,
        std::vector<Message*>& repliesToPartner) {
    for (uint64_t i = 0; i < partnerReqUpdCount; i++) {
        // skip the updates and the requests without replies
        LocalArray::IndexType indexType = LocalArray::UNKNOWN_INDEX_TYPE;
        switch (partnerMessages[i]->getType()) {
            case Message::MESSAGE_GET_POINT_SEQUENCE:
            {
                indexType = LocalArray::POINT_SEQUENCE;
                break;
            }
            case Message::MESSAGE_GET_POINT_TENSOR:
            {
                indexType = LocalArray::POINT_TENSOR;
                break;
            }
            case Message::MESSAGE_GET_REGION_SEQUENCE:
            {
                indexType = LocalArray::REGION_SEQUENCE;
                break;
            }
            case Message::MESSAGE_GET_REGION_TENSOR:
            {
                indexType = LocalArray::REGION_TENSOR;
                break;
            }
            case Message::MESSAGE_GET_LINEAR_MAPPING:
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
            continue;

        // create the reply message
        uint64_t replyLength = ((uint64_t *) partnerMessages[i]->getData())[0];
        char *reply = new char[replyLength];
        if (reply == NULL)
            throw ENotEnoughMemory();
        Message * message = new Message(_myProcID, partnerID,
                Message::MESSAGE_DATA_REPLY, partnerMessages[i]->getServer(),
                partnerMessages[i]->getClient(), LocalArray::OPID_ASSIGN,
                replyLength, reply, true);
        repliesToPartner.push_back(message);

        // fill in the reply
        ArrayRegistration * registration = _registration;
        while (registration != NULL) {
            if (registration->getAccessID()
                    == partnerMessages[i]->getServer()) {
                LocalArray *localArray =
                        dynamic_cast<LocalArray *> (registration->getArrayShape(
                        _myProcID));
                if (localArray == NULL)
                    throw ECorruptedRuntime();

                localArray->getElements(indexType,
                        (uint64_t *) partnerMessages[i]->getData(), reply);

                break;
            }
            registration = registration->getNext();
        }

        // send the reply
        send(replyLength, reply);
    }
}

void Runtime::copyUserDefinedArrays(uint64_t& partnerReqUpdCount,
        std::vector<Message*>& partnerMessages, uint64_t partnerID) {
    // copy the user defined messages
    for (uint64_t i = 0; i < partnerReqUpdCount; i++) {
        if (partnerMessages[i]->getType() == Message::MESSAGE_USER_ARRAY_NAME) {
            Message* arrayNameMessage = partnerMessages[i];
            if (i + 1 >= partnerMessages.size())
                throw ECorruptedRuntime();

            Message * serializationMessage = partnerMessages[i + 1];
            if (serializationMessage->getType()
                    != Message::MESSAGE_USER_ARRAY_SHAPE)
                throw ECorruptedRuntime();

            std::string names(arrayNameMessage->getData());
            uint64_t * serialization =
                    (uint64_t*) serializationMessage->getData();
            size_t pos = 0, semiPos = 0, iUserArray = 0;
            while ((semiPos = names.find(';', pos)) != (size_t) (-1)) {
                std::string arrayName = names.substr(pos, semiPos - pos);
                LocalArray* localArray = createImportedArray(partnerID,
                        arrayName,
                        serialization
                        + iUserArray * ArrayShape::SerializationSize);
                if (i + 2 + iUserArray >= partnerMessages.size())
                    throw ECorruptedRuntime();

                Message * dataMessage = partnerMessages[i + 2 + iUserArray];
                if (dataMessage->getType() != Message::MESSAGE_USER_DEFINED)
                    throw ECorruptedRuntime();
                iUserArray++;

                if (localArray->getByteCount() != dataMessage->getByteCount())
                    throw ECorruptedRuntime();

                memcpy(localArray->getData(), dataMessage->getData(),
                        localArray->getByteCount());
                pos = semiPos + 1;
            }
            break;
        }

    }

}

void Runtime::applyUpdates(uint64_t& partnerReqUpdCount,
        std::vector<Message*>& partnerMessages) {
    // apply the updates
    for (uint64_t i = 0; i < partnerReqUpdCount; i++) {
        // skip null messages
        if (partnerMessages[i] == NULL)
            continue;

        // deal with only the updates
        LocalArray::IndexType indexType = LocalArray::UNKNOWN_INDEX_TYPE;
        switch (partnerMessages[i]->getType()) {
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
            continue;

        // get the data message
        if (i == partnerReqUpdCount - 1)
            throw ECorruptedRuntime();
        Message * updateMessage = partnerMessages[i + 1];
        if (updateMessage->getType() != Message::MESSAGE_DATA_UPDATE)
            throw ECorruptedRuntime();

        // pass the data update to the server
        ArrayRegistration * registration = _registration;
        while (registration != NULL) {
            if (registration->getAccessID()
                    == partnerMessages[i]->getServer()) {
                registration->addIncomingMessage(partnerMessages[i]);
                partnerMessages[i] = NULL;
                registration->addIncomingMessage(updateMessage);
                partnerMessages[i + 1] = NULL;

                break;
            }
            registration = registration->getNext();
        }
    }
}

// BKDR Hash Function
uint32_t BKDRHash(const char *str)
{
    uint32_t seed = 131; // 31 131 1313 13131 131313 etc..
    uint32_t hash = 0;
 
    while (*str)
    {
        hash = hash * seed + (*str++);
    }
 
    return (hash & 0x7FFFFFFF);
}

// AP Hash Function
uint32_t APHash(const char *str)
{
    uint32_t hash = 0;
    int i;
 
    for (i=0; *str; i++)
    {
        if ((i & 1) == 0)
        {
            hash ^= ((hash << 7) ^ (*str++) ^ (hash >> 3));
        }
        else
        {
            hash ^= (~((hash << 11) ^ (*str++) ^ (hash >> 5)));
        }
    }
 
    return (hash & 0x7FFFFFFF);
}

void Runtime::exchange(bool* MatrixOfSendTo, const char *tag) {
    uint32_t hashTag1 = BKDRHash(tag);
    uint32_t hashTag2 = APHash(tag);

    if (_verbose) {
        std::cout << "exchange begin with hashTag 1 = " << hashTag1 << ", hashTag2 = " << hashTag2 << std::endl;
    }
    if (MatrixOfSendTo == NULL)
    {
	if (_verbose) {
	    std::cout << "[ERROR] NULL MatrixOfSendTo" << std::endl;
	}
        throw EInvalidArgument();
    }

    // clear imported
    clearImported();
    if (_verbose) {
        std::cout << "imported cleared" << std::endl;
    }

    // serialize export
    serializeExport();
    // compute the connectivity matrix
    bool* connectivity = computeConnectivity(MatrixOfSendTo);
    // exchange the messages
    bool* myConnect = connectivity + _myProcID * _nProcs;
    uint64_t UBoundNProcs = 1;
    while (UBoundNProcs < _nProcs)
        UBoundNProcs <<= 1;
    for (uint64_t mask = 0; mask < UBoundNProcs; mask++) {
        // compute partner procID
        uint64_t partnerID = _myProcID ^ mask;

        // skip unconnected partners
        if (partnerID >= _nProcs)
            continue;
        if (!myConnect[partnerID])
            continue;

        // exchange message count
        if (_verbose) {
            std::cout << _myProcID << "," << partnerID
                    << ":exchange message count and passwd...";
            std::cout.flush();
        }
        uint64_t myReqUpdCount = _nOutgoingRequestsAndUpdates[partnerID];
        uint64_t partnerReqUpdCount = 0;
        uint64_t myCountAndPass[2];
        uint64_t partnerCountAndPass[2];
        myCountAndPass[0] = myReqUpdCount;
        myCountAndPass[1] = hashTag1;
        myCountAndPass[1] <<= 32;
        myCountAndPass[1] |= hashTag2;
        partnerCountAndPass[0] = 0;
        partnerCountAndPass[1] = 0;
        _nal.sendReceive(myCountAndPass, partnerCountAndPass, 2 * sizeof (uint64_t),
                partnerID);
        partnerReqUpdCount = partnerCountAndPass[0];
        if (_verbose) {
            std::cout << "done" << std::endl;
        }

        // report error if the passwords dont match
        if (partnerCountAndPass[1] != myCountAndPass[1]) {
            throw EUnmatchedSync(tag,myCountAndPass[1],partnerCountAndPass[1]);
        }

        // skip empty request partners
        if (myReqUpdCount == 0 && partnerReqUpdCount == 0)
            continue;

        // exchange message headers (including size and type) for requests and updates
        uint64_t * myMessageHeader = NULL;
        uint64_t * partnerMessageHeader = NULL;
        if (_verbose) {
            std::cout << _myProcID << "," << partnerID
                    << ":exchange message headers...";
            std::cout.flush();
        }
        exchangeMessageHeaders(myReqUpdCount, myMessageHeader, partnerID,
                partnerReqUpdCount, partnerMessageHeader);
        if (_verbose) {
            std::cout << "done" << std::endl;
        }

        // exchange requests and updates
        if (_verbose) {
            std::cout << _myProcID << "," << partnerID
                    << ":exchange requests and updates...";
            std::cout.flush();
        }
        std::vector<Message *> partnerMessages = exchangeRequestsAndUpdates(
                myReqUpdCount, partnerID, partnerReqUpdCount,
                partnerMessageHeader);
        if (myMessageHeader != NULL) {
            delete[] myMessageHeader;
        }
        if (partnerMessageHeader != NULL) {
            delete[] partnerMessageHeader;
        }
        if (_verbose) {
            std::cout << "done" << std::endl;
        }

        // generate and exchange replies
        if (_verbose) {
            std::cout << _myProcID << "," << partnerID
                    << ":generate and exchange replies...";
            std::cout.flush();
        }
        uint64_t myReplyCount = receiveMyReplies(partnerID);
        std::vector<Message *> repliesToPartner;
        sendRepliesToPartner(partnerReqUpdCount, partnerMessages, partnerID,
                repliesToPartner);
        uint64_t partnerReplyCount = (uint64_t) repliesToPartner.size();
        if (myReplyCount > 0 || partnerReplyCount > 0) {
            exchangeWith(partnerID);
        }
        if (_verbose) {
            std::cout << "done" << std::endl;
        }

        // copy the user defined messages
        if (_verbose) {
            std::cout << _myProcID << "," << partnerID
                    << ":copy the user defined messages...";
            std::cout.flush();
        }
        copyUserDefinedArrays(partnerReqUpdCount, partnerMessages, partnerID);
        if (_verbose) {
            std::cout << "done" << std::endl;
        }

        // apply the updates
        if (_verbose) {
            std::cout << _myProcID << "," << partnerID
                    << ":apply the updates...";
            std::cout.flush();
        }
        applyUpdates(partnerReqUpdCount, partnerMessages);
        if (_verbose) {
            std::cout << "done" << std::endl;
        }

        if (_verbose) {
            std::cout << _myProcID << "," << partnerID
                    << ":clean up...";
            std::cout.flush();
        }
        for (uint64_t i = 0; i < partnerReqUpdCount; i++) {
            if (partnerMessages[i] == NULL)
                continue;
            delete partnerMessages[i];
        }
        for (uint64_t i = 0; i < partnerReplyCount; i++) {
            delete repliesToPartner[i];
        }
        partnerMessages.clear();
        repliesToPartner.clear();
        if (_verbose) {
            std::cout << "done" << std::endl;
        }
        this->_nIncomingReplies[partnerID] = 0;
        this->_nOutgoingRequestsAndUpdates[partnerID] = 0;
    }
    // release the temporary arrays
    delete[] connectivity;
    // clear registration messages
    ArrayRegistration* registration = _registration;
    while (registration != NULL) {
        registration->clearMessagesAndRequests();
        registration = registration->getNext();
    }
    _replyReceivers.clear();

    if (_verbose) {
        std::cout << "exiting exchange" << std::endl;
    }
}
/// @brief get object by path

NamedObject * Runtime::getObjectByPath(std::string path) {
    NameSpace* nspace = getObjectParent(path);
    std::string lastToken = getLastTokenOfPath(path);
    if (nspace == NULL)
        return NULL;

    NamedObject* nobject = nspace->getObject(lastToken);
    return nobject;
}

/// @brief create object

void Runtime::setObject(NameSpace * scope) {
    std::string path = scope->getPath();
    std::string objectPath;
    if ((int) path.find("this", 0) < 0 
            && (int) path.find_first_of("\r\n\t ") < 0
            && (int) path.find(":", 0) < 0)
        objectPath = path;
    else
        objectPath = simplifyPath(path);

    NamedObject *orig = getObjectByPath(objectPath);
    if (NULL != orig) {
        if (orig->getType() != NAMESPACE) {
            throw EInvalidArgument();
        }
        return;
    }

    NameSpace *nspace = getObjectParent(objectPath, true);
    std::string objectName = getLastTokenOfPath(objectPath);
    NamedObject *nobject = new NamedObject(objectName, scope);
    nspace->setObject(objectName, nobject);
}

/// @brief create object

void Runtime::setObject(std::string path, LocalArray * localArray) {
    std::string objectPath;
    if ((int) path.find("this", 0) < 0 
            && (int) path.find_first_of("\r\n\t ") < 0
            && (int) path.find(":", 0) < 0)
        objectPath = path;
    else
        objectPath = simplifyPath(path);

    NamedObject *orig = getObjectByPath(objectPath);
    if (NULL != orig) {
        if (orig->getType() != ARRAY || orig->isGlobal()) {
            throw EInvalidArgument();
        }
        if (NULL != orig->_localArray()->getRegistration()) {
            throw EDeleteSharedArray(objectPath.c_str());
        }
    }

    NameSpace *nspace = getObjectParent(objectPath, true);
    std::string objectName = getLastTokenOfPath(objectPath);
    NamedObject *nobject = new NamedObject(objectName, localArray);
    nspace->setObject(objectName, nobject);
}

/// @brief create object

void Runtime::setObject(std::string path, GlobalArray * globalArray) {
    std::string objectPath;
    if ((int) path.find("this", 0) < 0 
            && (int) path.find_first_of("\r\n\t ") < 0
            && (int) path.find(":", 0) < 0)
        objectPath = path;
    else
        objectPath = simplifyPath(path);

    NamedObject *orig = getObjectByPath(objectPath);
    if (NULL != orig) {
        if (orig->getType() != ARRAY || !orig->isGlobal()) {
            throw EInvalidArgument();
        }
        return;
    }

    NameSpace *nspace = getObjectParent(objectPath, true);
    std::string objectName = getLastTokenOfPath(objectPath);
    NamedObject *nobject = new NamedObject(objectName, globalArray);
    nspace->setObject(objectName, nobject);
}

/// @brief get object

NamedObject *Runtime::getObject(std::string path) {
    std::string objectPath;
    if ((int) path.find("this", 0) < 0 
            && (int) path.find_first_of("\r\n\t ") < 0
            && (int) path.find(":", 0) < 0)
        objectPath = path;
    else
        objectPath = simplifyPath(path);
    NamedObject *object = getObjectByPath(objectPath);
    if (NULL == object)
        throw EInvalidArgument();
    return object;
}

/// @brief delete object

void Runtime::deleteObject(std::string path, bool collective) {
    std::string objectPath;
    if ((int) path.find("this", 0) < 0 
            && (int) path.find_first_of("\r\n\t ") < 0
            && (int) path.find(":", 0) < 0)
        objectPath = path;
    else
        objectPath = simplifyPath(path);

    NameSpace *nspace = getObjectParent(objectPath, false);
    if (NULL == nspace)
        return;
    std::string objectName = getLastTokenOfPath(objectPath);
    nspace->deleteObject(objectName, collective);
}

/// @brief clear path

void Runtime::clearPath(std::string path) {
    std::string objectPath;
    if ((int) path.find("this", 0) < 0 
            && (int) path.find_first_of("\r\n\t ") < 0
            && (int) path.find(":", 0) < 0)
        objectPath = path;
    else
        objectPath = simplifyPath(path);
    NamedObject *nobj = getObjectByPath(objectPath);
    if (nobj == NULL)
        return;
    NameSpace *nspace = nobj->_namespace();
    nspace->clear();
}

/// @brief clear imported objects

void Runtime::clearImported() {
    for (uint64_t procID = 0; procID < _nProcs; procID++) {

        std::stringstream procSS;
        procSS << "_import.procID" << procID;
        clearPath(procSS.str());
    }
}

bool Runtime::hasObject(std::string path) {
    std::string objectPath;
    if ((int) path.find("this", 0) < 0 
            && (int) path.find_first_of("\r\n\t ") < 0
            && (int) path.find(":", 0) < 0)
        objectPath = path;
    else
        objectPath = simplifyPath(path);

    NameSpace* nspace = &_this;
    ssize_t pos = 0, dot = 0;
    for (;;) {
        dot = objectPath.find('.', pos);
        int length = dot - pos;
        if (dot < 0)
            length = -1;
        std::string objectName = objectPath.substr(pos, length);
        NamedObject * nObject = nspace->getObject(objectName);
        if (nObject == NULL) {
            return false;
        }
        if (dot >= 0) {
            if (nObject->getType() != NAMESPACE)
                return false;
            else {
                nspace = nObject->_namespace();
                if (nspace == NULL) {
                    throw ECorruptedRuntime();
                }
            }
        } else {

            return true;
        }
        pos = dot + 1;
    }
}

void Runtime::setVerbose(bool _verbose) {

    this->_verbose = _verbose;
}

bool Runtime::isVerbose() const {

    return _verbose;
}

/// @brief get object path

NameSpace * Runtime::getObjectParent(std::string path,
        bool createItIfNotExists) {
    NameSpace* nspace = &_this;
    std::string objectPath;
    if ((int) path.find("this", 0) < 0 && (int) path.find_first_of("\r\n\t ") < 0)
        objectPath = path;
    else
        objectPath = simplifyPath(path);
    ssize_t pos = 0, dot = 0;
    for (;;) {
        dot = objectPath.find('.', pos);
        if (dot < 0)
            break;
        std::string objectName = objectPath.substr(pos, dot - pos);
        NamedObject * nObject = nspace->getObject(objectName);
        if (nObject == NULL) {
            if (!createItIfNotExists)
                return NULL;
            NameSpace * nspaceNext = new NameSpace(nspace->getPath() + "." + objectName);
            nObject = new NamedObject(objectName, nspaceNext);
            if (nObject == NULL)
                throw ENotEnoughMemory();
            nspace->setObject(objectName, nObject);
        }
        nspace = nObject->_namespace();
        if (nspace == NULL) {

            throw ECorruptedRuntime();
        }
        pos = dot + 1;
    }
    return nspace;
}

NameSpace * Runtime::createImportedObjectParent(uint64_t procID,
        std::string path) {
    if (procID >= _nProcs)
        throw EInvalidArgument();

    std::stringstream procSS;
    procSS << "_import.procID" << procID;
    procSS << "." << path;

    return getObjectParent(procSS.str(), true);
}

LocalArray * Runtime::createImportedArray(uint64_t procID, std::string path,
        uint64_t * arrayShapeSerialization) {
    std::string objectPath;
    if ((int) path.find("this", 0) < 0 && (int) path.find_first_of("\r\n\t ") < 0)
        objectPath = path;
    else
        objectPath = simplifyPath(path);
    NameSpace* nspace = createImportedObjectParent(procID, objectPath);
    std::string objectName = getLastTokenOfPath(objectPath);
    std::stringstream procSS;
    procSS << "_import.procID" << procID;
    procSS << "." << objectPath;
    LocalArray* localArray = new LocalArray(procSS.str(),
            arrayShapeSerialization);
    if (localArray == NULL)
        throw ENotEnoughMemory();

    NamedObject * nobject = new NamedObject(objectName, localArray);
    if (nobject == NULL)
        throw ENotEnoughMemory();

    nspace->setObject(objectName, nobject);

    return localArray;
}

/// @brief serialize array shapes for export

void Runtime::serializeExport() {
    for (uint64_t procID = 0; procID < _nProcs; procID++) {
        _outgoingUserDefinedMessages[procID].clear();

        // get the names of the arrays to export
        std::string names = _outgoingUserArrayNames[procID].str();
        if (names.size() == 0)
            continue;
        _outgoingUserArrayNames[procID].clear();
        _outgoingUserArrayNames[procID].str("");

        // generate the message of array names
        char *arrayNameBuffer = new char[names.size() + 1];
        if (arrayNameBuffer == NULL)
            throw ENotEnoughMemory();
        memcpy(arrayNameBuffer, names.c_str(), names.size() + 1);
        Message * arrayNameMessage = new Message(_myProcID, procID,
                Message::MESSAGE_USER_ARRAY_NAME, 0, 0, 0, names.size() + 1,
                arrayNameBuffer, true);
        if (arrayNameMessage == NULL)
            throw ENotEnoughMemory();
        _outgoingUserDefinedMessages[procID].push_back(arrayNameMessage);

        // get the count of arrays to export
        uint64_t nExport = 0;
        size_t pos = 0, semiPos = 0;
        while ((semiPos = names.find(';', pos)) != (size_t) - 1) {
            pos = semiPos + 1;
            nExport++;
        }
        if (nExport == 0)
            throw ECorruptedRuntime();
        _nOutgoingRequestsAndUpdates[procID] += 2 + nExport;

        // generate the serialization of the array shapes
        uint64_t * serialization = new uint64_t[nExport
                * ArrayShape::SerializationSize];
        if (serialization == NULL)
            throw ENotEnoughMemory();
        uint64_t * currentSerialization = serialization;
        pos = 0, semiPos = 0;
        while ((semiPos = names.find(';', pos)) != (size_t) - 1) {
            std::string arrayName = names.substr(pos, semiPos - pos);
            NamedObject *nobject = getObject(arrayName);
            LocalArray *localArray = nobject->_localArray();
            localArray->serialize(currentSerialization);
            currentSerialization += ArrayShape::SerializationSize;
            pos = semiPos + 1;
        }
        Message *serializationMessage = new Message(_myProcID, procID,
                Message::MESSAGE_USER_ARRAY_SHAPE, 0, 0, 0,
                nExport * ArrayShape::SerializationSize * sizeof (uint64_t),
                (char *) serialization,
                true);
        if (serializationMessage == NULL)
            throw ENotEnoughMemory();
        _outgoingUserDefinedMessages[procID].push_back(serializationMessage);

        // generate the messages for array data
        pos = 0, semiPos = 0;
        while ((semiPos = names.find(';', pos)) != (size_t) - 1) {

            std::string arrayName = names.substr(pos, semiPos - pos);
            NamedObject *nobject = getObject(arrayName);
            LocalArray *localArray = nobject->_localArray();
            Message *message = new Message(_myProcID, procID,
                    Message::MESSAGE_USER_DEFINED, 0, 0, 0,
                    localArray->getByteCount(), localArray->getData(), false);
            _outgoingUserDefinedMessages[procID].push_back(message);
            pos = semiPos + 1;
        }
    }
}

std::string Runtime::simplifyPath(std::string path) {
    // expand short form of import path to full form
    ssize_t posColon = path.find(":", 0);
    if (posColon >= 0) {
        if (posColon == 0)
            throw EInvalidArgument();
        unsigned long procID = 0;
        if (1 != sscanf(path.substr(0, posColon).c_str(), "%lu", &procID))
            throw EInvalidArgument();
        std::stringstream procSS;
        procSS << "_import.procID" << procID;
        path = procSS.str() + "." + path.substr(posColon + 1);
    }
    
    // seperate the tokens with '.'
    bool firstDot = true;
    std::stringstream ss;
    ssize_t pos = 0, dot = 0;
    while (dot >= 0) {
        dot = path.find('.', pos);
        std::string part = path.substr(pos, dot >= 0 ? dot - pos : -1);

        // remove '\r','\n','\t' and ' ' at tail
        part.erase(part.find_last_not_of("\r\n\t ") + 1);

        // remove '\r','\n','\t' and ' ' at front
        part.erase(0, part.find_first_not_of("\r\n\t "));

        // check this part to see whether it is redundant
        if (part != "this") {
            if (!firstDot) {
                ss << ".";
            } else {

                firstDot = false;
            }
            ss << part;
        }

        pos = dot + 1;
    }

    return ss.str();
}

std::string Runtime::getLastTokenOfPath(std::string path) {
    ssize_t lastDot = path.find_last_of('.');
    std::string part = path.substr(lastDot + 1, -1);

    // remove '\r','\n','\t' and ' ' at tail
    part.erase(part.find_last_not_of("\r\n\t ") + 1);

    // remove '\r','\n','\t' and ' ' at front
    part.erase(0, part.find_first_not_of("\r\n\t "));

    return part;
}

std::string getParentPath(std::string path) {
    ssize_t pos = path.find_last_of(".");
    if (pos < 0)
        return "";
    else
        return path.substr(0, pos);
}

void Runtime::checkNameList(std::vector<std::string> nameList, bool checkShared) {
    // check the arrays in the name list locally, to make sure that they are not shared
    uint64_t nameHash[10] = {0,0,0,0,0,0,0,0,0,0};
    size_t n = nameList.size();
    assert(n <= 10);
    for (size_t i = 0; i < n; ++i) {
        NamedObject *nobj = getObject(nameList[i]);
        if (nobj->getType() != ARRAY)
            throw EInvalidArgument();
        if (nobj->isGlobal())
            throw EInvalidArgument();
        if (checkShared) {
            if (nobj->_localArray()->getRegistration())
                throw EShareShared(nameList[i].c_str());
        } else {
            if (NULL == nobj->_localArray()->getRegistration())
                throw EPrivatizePrivate(nameList[i].c_str());
        }
        nameHash[i] = BKDRHash(nameList[i].c_str());
        nameHash[i] <<= 32;
        nameHash[i] |= APHash(nameList[i].c_str());
    }

    // exchange n and length of nameList globally
    uint64_t *globalHash = new uint64_t[10*_nProcs];
    if (NULL == globalHash)
        throw ENotEnoughMemory();
    _nal.allGather(nameHash,globalHash,10*sizeof(size_t));

    // check that all names are equal
    for (uint64_t iProc = 0; iProc < _nProcs; ++iProc) {
        uint64_t *procHash = globalHash + 10 * iProc;
        for (unsigned i = 0; i < 10; ++i) {
            if (procHash[i] != nameHash[i]) {
                throw EDifferentNameList();
            }
        }
    }
    delete[] globalHash;
}

void Runtime::share(std::vector<std::string> nameList) {
    // check name list
    checkNameList(nameList, true);

    // compute a local buffer for exchanging local-array sizes and examinating name list
    size_t n = nameList.size();
    size_t bufferSize = n * 8; 
    size_t *buffer = new size_t[bufferSize];
    size_t *globalBuffer = new size_t[bufferSize * _nProcs];
    if (NULL == buffer || NULL == globalBuffer)
        throw ENotEnoughMemory();
    for (size_t i = 0; i < n; ++i) {
        NamedObject *nobj = getObject(nameList[i]);
        LocalArray *array = nobj->_localArray();
        buffer[(i << 3)] = array->getNumberOfDimensions();
        buffer[(i << 3)] <<= 8;
        buffer[(i << 3)] |= array->getElementType();
        buffer[(i << 3)] <<= 16;
        buffer[(i << 3)] |= array->getNumberOfBytesPerElement();
        for (unsigned iDim = 0; iDim < 7; ++iDim) {
            buffer[(i << 3) + 1 + iDim] = (size_t)array->getElementCount(iDim);
        }
    }
    _nal.allGather(buffer, globalBuffer, bufferSize * sizeof(size_t));

    // examing the consistency of nDims and elementSize
    for (size_t i = 0; i < n; ++i) {
        for (uint64_t iProc = 0; iProc < _nProcs; ++iProc) {
            if (buffer[0] != globalBuffer[iProc * bufferSize])
                throw ENotAbleToShare(nameList[i].c_str());
        }
    }

    // create array registrations
    ArrayShape **shapes = new ArrayShape *[_nProcs];
    for (size_t i = 0; i < n; ++i) {
        NamedObject *nobj = getObject(nameList[i]);
        LocalArray *array = nobj->_localArray();
        unsigned nDims = array->getNumberOfDimensions();
        unsigned elementSize = array->getNumberOfBytesPerElement();
        ArrayShape::ElementType elementType = array->getElementType();
        for (uint64_t iProc = 0; iProc < _nProcs; ++iProc) {
            if (iProc == _myProcID) {
                shapes[iProc] = array;
            } else {
                uint64_t dimSize[7];
                for (unsigned iDim = 0; iDim < 7; ++iDim) {
                    dimSize[iDim] = globalBuffer[iProc * bufferSize + 1 + iDim];
                }
                shapes[iProc] = new ArrayShape(elementType, elementSize, nDims, dimSize);
            }
        }
        ArrayRegistration *registration = new ArrayRegistration(_grid,shapes);
        if (NULL == registration) {
            throw ENotAbleToShare(nameList[i].c_str());
        }
    }

    // release temporary buffers
    delete[] shapes;
    delete[] buffer;
    delete[] globalBuffer;
}

void Runtime::globalize(std::vector<std::string> nameList, const unsigned nGridDims, 
        const uint64_t gridDimSize[7], uint64_t procStart) {
    Grid grid(nGridDims, gridDimSize, _nProcs, procStart);

    // check name list
    checkNameList(nameList, true);

    // compute a local buffer for exchanging local-array sizes and examinating name list
    size_t n = nameList.size();
    size_t bufferSize = n * 8 + 9;
    size_t *buffer = new size_t[bufferSize];
    size_t *globalBuffer = new size_t[bufferSize * _nProcs];
    if (NULL == buffer || NULL == globalBuffer)
        throw ENotEnoughMemory();
    for (size_t i = 0; i < n; ++i) {
        NamedObject *nobj = getObject(nameList[i]);
        LocalArray *array = nobj->_localArray();
        buffer[(i << 3)] = array->getNumberOfDimensions();
        buffer[(i << 3)] <<= 8;
        buffer[(i << 3)] |= array->getElementType();
        buffer[(i << 3)] <<= 16;
        buffer[(i << 3)] |= array->getNumberOfBytesPerElement();
        for (unsigned iDim = 0; iDim < 7; ++iDim) {
            buffer[(i << 3) + 1 + iDim] = (size_t)array->getElementCount(iDim);
        }
    }
    buffer[n * 8] = nGridDims;
    for (unsigned iDim = 0; iDim < nGridDims; ++iDim) {
        buffer[n * 8 + 1 + iDim] = (size_t)gridDimSize[iDim];
    }
    buffer[n * 8 + 8] = (size_t)procStart;
    _nal.allGather(buffer, globalBuffer, bufferSize * sizeof(size_t));

    // examine the consistency of nDims and elementSize
    for (size_t i = 0; i < n; ++i) {
        for (uint64_t iProc = 0; iProc < _nProcs; ++iProc) {
            if (!grid.containsProc(iProc))
                continue;
            if (buffer[0] != globalBuffer[iProc * bufferSize])
                throw ENotAbleToShare(nameList[i].c_str());
        }
    }

    // examine the consistency of grid
    for (uint64_t iProc = 0; iProc < _nProcs; ++iProc) {
        if (grid.containsProc(iProc)) {
            size_t offset = n * 8;
            size_t *gridOfProc = globalBuffer + iProc * bufferSize + offset;
            size_t *gridOfMine = buffer + offset;
            for (unsigned i = 0; i <= nGridDims; ++i) {
                if (gridOfProc[i] != gridOfMine[i])
                    throw EInvalidGrid();
            }
        }
    }

    // create array registrations and global arrays
    ArrayShape **shapes = new ArrayShape *[_nProcs];
    for (size_t i = 0; i < n; ++i) {
        NamedObject *nobj = getObject(nameList[i]);
        LocalArray *array = nobj->_localArray();
        unsigned nDims = array->getNumberOfDimensions();
        unsigned elementSize = array->getNumberOfBytesPerElement();
        ArrayShape::ElementType elementType = array->getElementType();
        for (uint64_t iProc = 0; iProc < _nProcs; ++iProc) {
            if (iProc == _myProcID) {
                shapes[iProc] = array;
            } else {
                uint64_t dimSize[7];
                for (unsigned iDim = 0; iDim < 7; ++iDim) {
                    dimSize[iDim] = globalBuffer[iProc * bufferSize + (i << 3) + 1 + iDim];
                }
                shapes[iProc] = new ArrayShape(elementType, elementSize, nDims, dimSize);
            }
        }
        try {
            ArrayRegistration *registration = new ArrayRegistration(grid,shapes);
            GlobalArray *globalArray = new GlobalArray(*registration);
            setObject(nameList[i]+"@global",globalArray);
        } catch (const EInvalidArgument& e) {
            throw EUnableToGlobalize(nameList[i].c_str());
        } 
    }

    // release temporary buffers
    delete[] shapes;
    delete[] buffer;
    delete[] globalBuffer;
}

void Runtime::privatize(std::vector<std::string> nameList) {
    // check name list
    checkNameList(nameList, false);

    // iterate through the name list
    size_t n = nameList.size();
    for (size_t i = 0; i < n; ++i) {
        // delete global arrays
        if (hasObject(nameList[i] + "@global")) {
            deleteObject(nameList[i] + "@global", true);
        }
        // delete registration
        NamedObject *nobj = getObject(nameList[i]);
        LocalArray *array = nobj->_localArray();
        delete array->getRegistration();
    }
}

/// @brief create 1d local array from a string
void Runtime::fromString(std::string string, std::string path) {
    std::string objPath = simplifyPath(path);
    if (hasObject(objPath)) {
        NamedObject *nobj = getObjectByPath(objPath);
        if (nobj->getType() != ARRAY || nobj->isGlobal())
            throw EInvalidArgument();
    }
    uint64_t len = string.size() + 1;
    LocalArray *localArray = new LocalArray(objPath,ArrayShape::INT8,1,1,&len);
    if (localArray == NULL)
        throw ENotEnoughMemory();
    setObject(objPath, localArray);
    strcpy(localArray->getData(), string.c_str());
}

/// @brief create nd local array from a buffer
void Runtime::fromBuffer(const char *buffer, const char kind, const int nDims, const ssize_t *dimSize, const ssize_t *strides, std::string path) {
    std::string objPath = simplifyPath(path);
    if (hasObject(objPath)) {
        NamedObject *nobj = getObjectByPath(objPath);
        if (nobj->getType() != ARRAY || nobj->isGlobal())
            throw EInvalidArgument();
    }
    ArrayShape::ElementType elemType = ArrayShape::BINARY;
    unsigned elemSize = 1;
    bool isFortranArray = false;
    if (strides[nDims - 1] > strides[0]) {
        isFortranArray = true;
        elemSize = strides[0];
    } else {
        elemSize = strides[nDims - 1];
    }
    if (kind == 'i') {
        switch (elemSize) {
            case 1:
                elemType = ArrayShape::INT8;
                break;
            case 2:
                elemType = ArrayShape::INT16;
                break;
            case 4:
                elemType = ArrayShape::INT32;
                break;
            case 8:
                elemType = ArrayShape::INT64;
                break;
            default:
                throw EInvalidArgument();
        }
    } else if (kind == 'u') {
        switch (elemSize) {
            case 1:
                elemType = ArrayShape::UINT8;
                break;
            case 2:
                elemType = ArrayShape::UINT16;
                break;
            case 4:
                elemType = ArrayShape::UINT32;
                break;
            case 8:
                elemType = ArrayShape::UINT64;
                break;
            default:
                throw EInvalidArgument();
        }
    } else if (kind == 'f') {
        switch (elemSize) {
            case 4:
                elemType = ArrayShape::FLOAT;
                break;
            case 8:
                elemType = ArrayShape::DOUBLE;
                break;
            default:
                throw EInvalidArgument();
        }
    } else if (kind == 'c') {
        switch (elemSize) {
            case 8:
                elemType = ArrayShape::CFLOAT;
                break;
            case 16:
                elemType = ArrayShape::CDOUBLE;
                break;
            default:
                throw EInvalidArgument();
        }
    } else {
        throw EInvalidArgument();
    }

    uint64_t myDimSize[7];
    size_t myStride[7];
    if (isFortranArray) {
        for (int iDim = 0; iDim < nDims; ++iDim) {
            myDimSize[nDims - 1 - iDim] = dimSize[iDim];
            myStride[nDims - 1 - iDim] = strides[iDim];
        }
    } else {
        for (int iDim = 0; iDim < nDims; ++iDim) {
            myDimSize[iDim] = dimSize[iDim];
            myStride[iDim] = strides[iDim];
        }
    }
    for (int iDim = nDims; iDim < 7; ++iDim) {
        myDimSize[iDim] = 1;
        myStride[iDim] = myStride[iDim - 1];
    }

    LocalArray *localArray = new LocalArray(objPath, elemType, elemSize, nDims, myDimSize);
    if (localArray == NULL)
        throw ENotEnoughMemory();
    setObject(objPath, localArray);

    bool contiguous = true;
    if (isFortranArray) {
        for (int iDim = 1; iDim < nDims; ++iDim) {
            if (strides[iDim] != strides[iDim - 1] * dimSize[iDim - 1]) {
                contiguous = false;
                break;
            }
        }
    } else {
        for (int iDim = 0; iDim < nDims - 1; ++iDim) {
            if (strides[iDim] != strides[iDim + 1] * dimSize[iDim + 1]) {
                contiguous = false;
                break;
            }
        }
    }

    if (contiguous) {
        memcpy(localArray->getData(), buffer, localArray->getByteCount());
    } else {
        char *dst = localArray->getData();
        const char *src = buffer;
        if (nDims <= 2) {
            for (uint64_t i0 = 0; i0 < myDimSize[0]; ++i0) {
                const char *src1 = src;
                for (uint64_t i1 = 0; i1 < myDimSize[1]; ++i1) {
                    for (unsigned i = 0; i < elemSize; ++i) {
                        *dst = src1[i];
                        ++dst;
                    }
                    src1 += myStride[1];
                }
                src += myStride[0];
            }
        } else if (nDims <= 4) {
            for (uint64_t i0 = 0; i0 < myDimSize[0]; ++i0) {
                const char *src1 = src;
                for (uint64_t i1 = 0; i1 < myDimSize[1]; ++i1) {
                    const char *src2 = src1;
                    for (uint64_t i2 = 0; i2 < myDimSize[2]; ++i2) {
                        const char *src3 = src2;
                        for (uint64_t i3 = 0; i3 < myDimSize[3]; ++i3) {
                            for (unsigned i = 0; i < elemSize; ++i) {
                                *dst = src3[i];
                                ++dst;
                            }
                            src3 += myStride[3];
                        }
                        src2 += myStride[2];
                    }
                    src1 += myStride[1];
                }
                src += myStride[0];
            }
        } else {
            for (uint64_t i0 = 0; i0 < myDimSize[0]; ++i0) {
                const char *src1 = src;
                for (uint64_t i1 = 0; i1 < myDimSize[1]; ++i1) {
                    const char *src2 = src1;
                    for (uint64_t i2 = 0; i2 < myDimSize[2]; ++i2) {
                        const char *src3 = src2;
                        for (uint64_t i3 = 0; i3 < myDimSize[3]; ++i3) {
                            const char *src4 = src3;
                            for (uint64_t i4 = 0; i4 < myDimSize[4]; ++i4) {
                                const char *src5 = src4;
                                for (uint64_t i5 = 0; i5 < myDimSize[5]; ++i5) {
                                    const char *src6 = src5;
                                    for (uint64_t i6 = 0; i6 < myDimSize[6]; ++i6) {
                                        for (unsigned i = 0; i < elemSize; ++i) {
                                            *dst = src6[i];
                                            ++dst;
                                        }
                                        src6 += myStride[6];
                                    }
                                    src5 += myStride[5];
                                }
                                src4 += myStride[4];
                            }
                            src3 += myStride[3];
                        }
                        src2 += myStride[2];
                    }
                    src1 += myStride[1];
                }
                src += myStride[0];
            }
        }
    }
}

/// @brief create string from 1d local array
std::string Runtime::toString(std::string path) {
    std::string retval(getObject(path)->_localArray()->getData());
    return retval;
}

