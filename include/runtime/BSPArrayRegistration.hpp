/*
 * BSPArrayRegistratoin.hpp
 *
 *  Created on: 2014-8-23
 *      Author: junfeng
 */

#ifndef BSPARRAYREGISTRATION_HPP_
#define BSPARRAYREGISTRATION_HPP_
#include <vector>
#include "BSPGrid.hpp"
#include "BSPArrayShape.hpp"
#include "BSPMessage.hpp"
namespace BSP {
    class GlobalRequest;
    class LocalRequest;

    class ArrayRegistration : public Grid {
        friend class ArrayPartition;
    public:
        static const unsigned MAX_ACCESS_ID = 10000;
        
        /// @brief constructor of array registration
        /// @param grid a grid within which the local arrays are constructed 
        /// with normal constructor, and out of which the local arrays are 
        /// constructed (possibly) with null constructor
        /// @param localArrayRef an array with nProcs elements, where nProcs
        /// is the number of BSP proccesses in total (including those in the 
        /// grid and those out of the grid)
        ArrayRegistration(Grid &grid, ArrayShape **localArrayRef);

        virtual ~ArrayRegistration();
        void addGlobalRequest(GlobalRequest *request);
        void addLocalRequest(LocalRequest *request);
        void addIncomingMessage(Message *message);
        void addOutgoingMessage(Message *message);
        void clearMessagesAndRequests();

        uint16_t getAccessID() {
            return _accessID;
        }

        ArrayShape *getArrayShape(uint64_t procID) {
            if (!containsProc(procID))
                throw EInvalidArgument();
            return _localArrayRefInGrid[procID - _startProcID];
        }

        ArrayRegistration *getNext() {
            return _next;
        }
        void getHeaders(uint64_t procID, uint64_t *headers, uint64_t& offset);
        void sendRequestsAndUpdates(uint64_t procID);
        void receiveReplies(uint64_t procID);
        ArrayShape::ElementType getElementType();
    protected:
        void registerToRuntime();
        void unregister();
    private:
        ArrayShape **_localArrayRefInGrid;
        ArrayShape **_localArrayRef;
        uint16_t _accessID;
        ArrayRegistration *_next;
        std::vector<Message *> _incomingReplies;
        std::vector<Message *> _incomingRequests;
        std::vector<Message *> _incomingUpdates;
        std::vector<Message *> _outgoingRequestsAndUpdates;
        std::vector<GlobalRequest *> _globalRequests;
        std::vector<LocalRequest *> _localRequests;
    };

} /* namespace BSP */
#endif /* BSPARRAYREGISTRATOIN_HPP_ */
