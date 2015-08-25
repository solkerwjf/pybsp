/*
 * BSPRuntime.hpp
 *
 *  Created on: 2014-7-9
 *      Author: junfeng
 */

#ifndef BSPRUNTIME_HPP_
#define BSPRUNTIME_HPP_

#include <stdint.h>
#include <string>
#include <mpi.h>
#include <map>
#include <vector>
#include <queue>
#include <sstream>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "BSPNet.hpp"
#include "BSPGrid.hpp"
#include "BSPException.hpp"
#include "BSPNameSpace.hpp"
#include "BSPIndexSet.hpp"
namespace BSP {
    class ArrayRegistration;
    class GlobalArray;
    class LocalArray;
    class GlobalRequest;
    class LocalRequest;
    class Message;
    class Program;
    enum Type;
    /// @brief the main class of C$ runtime library

    class Runtime {
        friend class ArrayRegistration;
    private:
        Net _nal;
        Grid _grid;
        uint64_t _nProcs;
        uint64_t _myProcID;
        uint64_t _indexOfGridDim[7];
        std::string _locationString;
	int _currentSubRank;
        ArrayRegistration *_registration;
        NameSpace _this;
        uint64_t *_nOutgoingRequestsAndUpdates;
        uint64_t *_nIncomingReplies;
        std::vector<Message *> *_incomingUserDefinedMessages; // reserved for later use
        std::vector<Message *> *_outgoingUserDefinedMessages;
        std::queue<std::string> _eventQueue;
        std::vector<std::string> _syncList;
        std::map<GlobalRequest *, LocalArray *> _replyReceivers;
        std::stringstream *_incomingUserArrayNames; // reserved for later use
        std::stringstream *_outgoingUserArrayNames;
        static Runtime *_activeRuntimeObject;
        bool _verbose;
        bool _finalizing;


        void fillMyMessageHeader(uint64_t& partnerID, uint64_t*& myMessageHeader);
        void exchangeMessageHeaders(uint64_t& myReqUpdCount,
                uint64_t*& myMessageHeader, uint64_t& partnerID,
                uint64_t& partnerReqUpdCount, uint64_t*& partnerMessageHeader);
        std::vector<Message*> exchangeRequestsAndUpdates(uint64_t myReqUpdCount,
                uint64_t& partnerID, uint64_t& partnerReqUpdCount,
                uint64_t* partnerMessageHeader);
        uint64_t receiveMyReplies(uint64_t& partnerID);
        void sendRepliesToPartner(uint64_t& partnerReqUpdCount,
                std::vector<Message*>& partnerMessages, uint64_t& partnerID,
                std::vector<Message*>& repliesToPartner);
        void copyUserDefinedArrays(uint64_t& partnerReqUpdCount,
                std::vector<Message*>& partnerMessages, uint64_t partnerID);
        void applyUpdates(uint64_t& partnerReqUpdCount,
                std::vector<Message*>& partnerMessages);
        std::string getLastTokenOfPath(std::string path);
        void checkNameList(std::vector<std::string> nameList, bool checkShared);

    public:

        /// @brief constructor
        /// @param pArgc the pointer to argc of main()
        /// @param pArgv the pointer to argv of main()
        Runtime(int *pArgc, char ***pArgv);

        /// @brief destructor
        ~Runtime();

        inline bool inGrid() {
            return _grid.containsProc(_myProcID);
        }

        Grid &getGrid() {
            return _grid;
        }

        /// @brief get number of processes
        /// @return the number of processes

        uint64_t getNumberOfProcesses() {
            return _nProcs;
        }

        /// @brief get my process ID
        /// @return the ID of my process

        uint64_t getMyProcessID() {
            return _myProcID;
        }

        std::string getLocation() {
            return _locationString;
        }

        /// @brief get the active runtime object

        static Runtime *getActiveRuntime() {
            return _activeRuntimeObject;
        }

        /// @brief get "this" namespace

        NameSpace &getThis() {
            return _this;
        }

        /// @brief get nal

        Net *getNAL() {
            return &_nal;
        }

        /// @brief abort
        void abort();

        /// @brief resume a program
        ///void resumeProgram(std::string programID);

        /// @brief request data from/to global array
        void requestFrom(GlobalArray &server, IndexSet &indexSet,
                LocalArray &client, const std::string requestID);
        void requestTo(GlobalArray &server, IndexSet &indexSet, LocalArray &client,
                uint16_t opID, const std::string requestID);
        void requestFrom(GlobalArray &server, const unsigned numberOfVariables,
                const int64_t *matrix, const uint64_t *variableStart,
                const uint64_t *variableEnd, LocalArray &client,
                const std::string requestID);
        void requestTo(GlobalArray &server, const unsigned numberOfVariables,
                const int64_t *matrix, const uint64_t *variableStart,
                const uint64_t *variableEnd, LocalArray &client, uint16_t opID,
                const std::string requestID);

        /// @brief request data from/to local array
        void requestFrom(LocalArray &server, uint64_t serverProcID,
                IndexSet &indexSet, LocalArray &client,
                const std::string requestID);
        void requestTo(LocalArray &server, uint64_t serverProcID,
                IndexSet &indexSet, LocalArray &client, uint16_t opID,
                const std::string requestID);

        /// @brief send user-defined message to a given proc
        void exportUserDefined(uint64_t procID, std::string path);

        /// @brief perform the data exchange
        void exchange(bool *MatrixOfSendTo, const char *tag);

        /// @brief remove "this." from path string
        std::string simplifyPath(std::string path);

        /// @brief create object
        void setObject(NameSpace *scope);

        /// @brief create object
        void setObject(std::string path, LocalArray *localArray);

        /// @brief create object
        void setObject(std::string path, GlobalArray *globalArray);

        /// @brief get object
        NamedObject *getObject(std::string path);

        /// @brief delete object
        void deleteObject(std::string path, bool collective = false);

        /// @brief clear path
        void clearPath(std::string path);

        /// @brief clear imported objects
        void clearImported();

        /// @brief check whether object exists
        bool hasObject(std::string path);

        void setVerbose(bool _verbose);

        bool isVerbose() const;

        bool isFinalizing() const {return _finalizing;};

        void share(std::vector<std::string> nameList);

        void globalize(std::vector<std::string> nameList, const unsigned nGridDims, 
            const uint64_t gridDimSize[7], uint64_t procStart);

        void privatize(std::vector<std::string> nameList);

        /// @brief create 1d local array from a string
        void fromString(std::string string, std::string path);

        /// @brief create nd local array from a buffer
        void fromBuffer(const char *buffer, const char kind, const int nDims, const ssize_t *dimSize, const ssize_t *strides, std::string path);

        /// @brief create string from 1d local array
        std::string toString(std::string path);

    protected:
        /// @brief send data
        /// @param numberOfBytes the number of bytes of the data size
        /// @param dataBuffer the pointer to the data

        void send(size_t numberOfBytes, void *dataBuffer) {
            _nal.addDataBlockToSend(dataBuffer, numberOfBytes);
        }

        /// @brief receive data
        /// @param numberOfBytes the number of bytes of the data size
        /// @param dataBuffer the pointer to the data

        void receive(size_t numberOfBytes, void *dataBuffer) {
            _nal.addDataBlockToReceive(dataBuffer, numberOfBytes);
        }

        /// @brief exchange data
        /// @param partnerRank the rank of the partner to exchange data with

        void exchangeWith(uint64_t partnerRank) {
            _nal.exchangeDataBlocksWith(partnerRank);
        }

        /// @brief get object by path
        NamedObject *getObjectByPath(std::string path);

        /// @brief get object path
        NameSpace *getObjectParent(std::string path, bool createItIfNotExists =
                false);

        /// @brief create imported object path
        NameSpace *createImportedObjectParent(uint64_t procID, std::string path);

        /// @brief create imported array
        LocalArray *createImportedArray(uint64_t procID, std::string path,
                uint64_t *arrayShapeSerialization);

        /// @brief serialize array shapes for export
        void serializeExport();

        /// @brief compute connectivity matrix
        bool* computeConnectivity(bool* MatrixOfSendTo);
    };
}

#endif /* BSPRUNTIME_HPP_ */
