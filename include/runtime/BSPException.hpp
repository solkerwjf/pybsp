/*
 * BSPException.hpp
 *
 *  Created on: 2014-7-11
 *      Author: junfeng
 */

#ifndef BSPEXCEPTION_HPP_
#define BSPEXCEPTION_HPP_

#include <exception>
#include <string>
#include <cstring>
#include <cstdio>
#include <stdint.h>

namespace BSP {
    class NamedObject;

    // ----------------------------------------- runtime exceptions ------------------------------------------------

    class EAccessIDExhausted : public std::exception {
    public:
        EAccessIDExhausted();
        virtual const char *what() const throw ();
    };

    class EDeleteSharedArray : public std::exception {
    private:
        static char _errorString[1024];
    public:
        EDeleteSharedArray(const char *arrayName);
        virtual const char *what() const throw ();
    };

    class EShareShared : public std::exception {
    private:
        static char _errorString[1024];
    public:
        EShareShared(const char *arrayName);
        virtual const char *what() const throw ();
    };

    class EPrivatizePrivate : public std::exception {
    private:
        static char _errorString[1024];
    public:
        EPrivatizePrivate(const char *arrayName);
        virtual const char *what() const throw ();
    };

    class ENotAbleToShare : public std::exception {
    private:
        static char _errorString[1024];
    public:
        ENotAbleToShare(const char *arrayName);
        virtual const char *what() const throw ();
    };

    class EUnableToGlobalize : public::std::exception {
    private:
        static char _errorString[1024];
    public:
        EUnableToGlobalize(const char *arrayName);
        virtual const char *what() const throw ();
    };

    class EDifferentNameList : public std::exception {
    public:
        EDifferentNameList();
        virtual const char *what() const throw ();
    };

    class EInvalidType : public std::exception {
    public:
        EInvalidType();
        virtual const char *what() const throw ();
    };

    class EInvalidFileFormat : public std::exception {
    private:
        static char _errorString[1024];
    public:
        EInvalidFileFormat(const char *fileName);
        virtual const char *what() const throw ();
    };

    class EInvalidIDim : public std::exception {
    public:
        EInvalidIDim();
        virtual const char *what() const throw ();
    };

    class EInvalidNDims : public std::exception {
    public:
        EInvalidNDims();
        virtual const char *what() const throw ();
    };

    class EInvalidGrid : public std::exception {
    public:
        EInvalidGrid();
        virtual const char *what() const throw ();
    };

    class EIOError : public std::exception {
    public:
        EIOError();
        virtual const char *what() const throw ();
    };

    class ENotEnoughMemory : public std::exception {
    public:
        ENotEnoughMemory();
        virtual const char *what() const throw ();
    };

    class EDeadLockWait : public std::exception {
    private:
        static char _errorString[1024];
    public:
        EDeadLockWait(const char * programID);
        virtual const char *what() const throw ();
    };

    class EOutOfBound : public std::exception {
    private:
        static char _errorString[1024];
    public:
        EOutOfBound(const char *arrayName);

        virtual const char *what() const throw ();
    };

    class EInvalidProcID : public std::exception {
    private:
        static char _errorString[1024];
    public:
        EInvalidProcID(unsigned long procID);

        virtual const char *what() const throw ();
    };
    
    class ETypeNotMatched : public std::exception {
    private:
        static char _errorString[1024];
    public:
        ETypeNotMatched(const char *name, const char *expectdType);
        virtual const char *what() const throw ();
    };

    class EVariableNotFound : public std::exception {
    private:
        static char _errorString[1024];
    public:
        EVariableNotFound(const char *variableName);
        virtual const char *what() const throw ();
    };

    class EInvalidArgument : public std::exception {
    public:
        EInvalidArgument();
        virtual const char *what() const throw ();
    };

    class EInvalidElementPosition : public std::exception {
    public:
        EInvalidElementPosition(std::string requestID, unsigned iDim,
                unsigned nVars, int64_t indexAlongDim, const uint64_t valueOfVar[],
		const uint64_t ubound);
        virtual const char *what() const throw ();
        ~EInvalidElementPosition() throw() {}
        std::string getRequestID() const;
        unsigned getIDim() const;
        unsigned getNVars() const;
        int64_t getIndexAlongDim() const;
        uint64_t getValueOfVar(unsigned iDim) const;
    private:
        static char _errorString[1024];
        std::string _requestID;
        unsigned _iDim;
        unsigned _nVars;
        int64_t _indexAlongDim;
        uint64_t _valueOfVar[7];
    };

    class EInvalidRegionDescriptor : public std::exception {
    public:
        EInvalidRegionDescriptor(unsigned iDim, uint64_t iRegion,
                int64_t begin, int64_t end);
        virtual const char *what() const throw ();
        unsigned getIDim() const;
        int64_t getBegin() const;
        int64_t getEnd() const;
        uint64_t getIRegion() const;
    private:
        unsigned _iDim;
        uint64_t _iRegion;
        int64_t _begin;
        int64_t _end;
    };

    class EClientArrayTooSmall : public std::exception {
    public:
        EClientArrayTooSmall(std::string requestID,
                uint64_t clientArraySize, uint64_t requestSize);
        virtual const char *what() const throw ();
        ~EClientArrayTooSmall() throw () {}
        std::string getRequestID() const;
        uint64_t getClientArraySize() const;
        uint64_t getRequestSize() const;
    private:
        std::string _requestID;
        uint64_t _clientArraySize;
        uint64_t _requestSize;
    };

    class EInvalidCustomizedSync : public std::exception {
    private:
        static char _errorString[1024];
    public:
        EInvalidCustomizedSync(unsigned long senderID,
                unsigned long receiverID, unsigned long nReqUpd);
        virtual const char *what() const throw ();
    };

    class ENotAvailable : public std::exception {
    public:
        ENotAvailable();
        virtual const char *what() const throw ();
    };

    class ECorruptedRuntime : public std::exception {
    public:
        ECorruptedRuntime();
        virtual const char *what() const throw ();
    };

    class ESocketFailure : public std::exception {
    public:
        ESocketFailure();
        virtual const char *what() const throw ();
    };

    class EInvalidAccess : public std::exception {
    public:
        EInvalidAccess();
        virtual const char *what() const throw ();
    };

    class ELocalToGlobalAssignment : public std::exception {
    public:
        ELocalToGlobalAssignment();
        virtual const char *what() const throw ();
    };

    class EUnmatchedSync : public std::exception {
    private:
        static char _errorString[1024];
    public:
        EUnmatchedSync(const char *myTag, uint64_t myHash, uint64_t partnerHash);
        virtual const char *what() const throw ();
    };

} // namespace BSP

#endif /* BSPEXCEPTION_HPP_ */
