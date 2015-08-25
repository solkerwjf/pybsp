/*
 * BSPNet.hpp
 *
 *  Created on: 2014-7-9
 *      Author: hongkai & junfeng
 */

#ifndef BSPNET_HPP_
#define BSPNET_HPP_
#include <stdint.h>
#include <string>
#include <mpi.h>
// ...
namespace BSP {

    class Net {
    public:
        //! A constructor.
        /*!
         \param pArgc an integer pointer, points to argc.
         \param pArgv a char** pointer, points to argv.
         \param segmentThreshold a size type, indicates the max length available in a transmission.
         \param maxNumberOfDataBlocks a size type, indicates the upper bound of _numberOfDataBlocksToSend and _numberOfDataBlocksToReceive.
         */
        Net(int *pArgc, char ***pArgv, uint64_t segmentThreshold,
                uint64_t maxNumberOfDataBlocks);

        //! A destructor.
        /*!
         Release the memory allocated before.
         */
        ~Net();

        //! A normal member taking two arguments and returning a boolean value.
        /*!
         Add a "send" request.
         \param dataBlock a void type pointer, points to the source message to be sent.
         \param lengthOfDataBlock a size type, indicates the length of the message, counting by byte.
         \return Whether the function run successfully
         */
        bool addDataBlockToSend(void *dataBlock, uint64_t lengthOfDataBlock);

        //! A normal member taking two arguments and returning a boolean value.
        /*!
         Add a "receive" request.
         \param dataBlock a void type pointer, points to the destination of the message.
         \param lengthOfDataBlock a size type, indicates the length of the message, counting by byte.
         \return Whether the function run successfully
         */
        bool addDataBlockToReceive(void *dataBlock, uint64_t lengthOfDataBlock);

        //! A normal member taking one argument and returning nothing.
        /*!
         Implement a pair transmission
         \param procRank a integer, indicates the identity of a process needed to exchange with.
         */
        void exchangeDataBlocksWith(uint64_t procRank);

        //! broadcast
        void broadcast(void *data, uint64_t length);

        //! send and receive
        void sendReceive(void *dataOut, void *dataIn, uint64_t length, uint64_t procID);

        //! all gather
        void allGather(void *dataOut, void *dataIn, uint64_t lengthPerProc);

        //! joint gather
        void jointGather(void *dataOut, void *dataIn, uint64_t lengthPerProc,
                uint64_t startProcID, uint64_t nProcsInGrid);

        /// @brief get the processor name
        /// @return processor name
        std::string getProcessorName();

        /// @brief get the number of processes
        /// @return the number of processes
        uint64_t getNumberOfProcesses();

        /// @brief get the rank of current process
        /// @return the rank of current process
        uint64_t getProcessRank();

        /// @brief abort all processes
        void abort();

        void finalize();

    protected:
        /// initializer
        /*!
         \param pArgc an integer pointer, points to argc.
         \param pArgv a char** pointer, points to argv.
         \param segmentThreshold a size type, indicates the max length available in a transmission.
         \param maxNumberOfDataBlocks a size type, indicates the upper bound of _numberOfDataBlocksToSend and _numberOfDataBlocksToReceive.
         */
        void initialize(MPI_Comm comm, uint64_t segmentThreshold,
                uint64_t maxNumberOfDataBlocks);
        
    private:


        //! A private variable.
        /*!
         A size type, indicates the max length available in a transmission.
         \sa NetworkAbstractionLayer()
         */
        uint64_t _segmentThreshold;

        //! A private variable.
        /*!
         A void* pointer, points to a list of source messages to be sent.
         \sa addDataBlockToSend()
         */
        void **_dataBlockToSend;
        //! A private variable.
        /*!
         A size type pointer, points to a list of length of the specified block to be sent.
         \sa addDataBlockToSend()
         */
        uint64_t *_lengthOfDataBlockToSend;
        //! A private variable.
        /*!
         A size type, indicates the number of blocks to be sent in the list.
         */
        uint64_t _numberOfDataBlocksToSend;

        //! A private variable.
        /*!
         A void* pointer, points to a list of destinations of the messages.
         \sa addDataBlockToReceive()
         */
        void **_dataBlockToReceive;
        //! A private variable.
        /*!
         A size type pointer, points to a list of length of the specified block to receive.
         \sa addDataBlockToReceive()
         */
        uint64_t *_lengthOfDataBlockToReceive;
        //! A private variable.
        /*!
         A size type, indicates the number of blocks to receive in the list.
         */
        uint64_t _numberOfDataBlocksToReceive;

        //! A private variable.
        /*!
         A size type, indicates the upper bound of _numberOfDataBlocksToSend and _numberOfDataBlocksToReceive.
         */
        uint64_t _maxNumberOfDataBlocks;
    };

} // namespace BSP

#endif /* BSPNET_HPP_ */
