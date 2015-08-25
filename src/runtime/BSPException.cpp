#include "BSPException.hpp"
#include "BSPRuntime.hpp"

using namespace BSP;

EAccessIDExhausted::EAccessIDExhausted() {
}

const char *EAccessIDExhausted::what() const throw () {
    return "Access IDs have been exhausted";
}

char EDeleteSharedArray::_errorString[1024];
EDeleteSharedArray::EDeleteSharedArray(const char *arrayName) {
    sprintf(_errorString, "Failed to delete shared array \"%s\" in private\n",
            arrayName);
}

const char *EDeleteSharedArray::what() const throw () {
    return _errorString;
}

char EShareShared::_errorString[1024];
EShareShared::EShareShared(const char *arrayName) {
    sprintf(_errorString, "Failed to share or globalize an already-shared-or-globalized array \"%s\"\n",
            arrayName);
}

const char *EShareShared::what() const throw () {
    return _errorString;
}

char EPrivatizePrivate::_errorString[1024];
EPrivatizePrivate::EPrivatizePrivate(const char *arrayName) {
    sprintf(_errorString, "Failed to privatize a private array \"%s\"\n",
            arrayName);
}

const char *EPrivatizePrivate::what() const throw () {
    return _errorString;
}

char ENotAbleToShare::_errorString[1024];
ENotAbleToShare::ENotAbleToShare(const char *arrayName) {
    sprintf(_errorString, "array \"%s\" is of different nDims/elementType in different procs and therefore not able to be shared\n",
            arrayName);
}

const char *ENotAbleToShare::what() const throw () {
    return _errorString;
}

char EUnableToGlobalize::_errorString[1024];
EUnableToGlobalize::EUnableToGlobalize(const char *arrayName) {
    sprintf(_errorString, "array \"%s\" is unable to be globalized due to its bad partition among procs\n",
            arrayName);
}

const char *EUnableToGlobalize::what() const throw () {
    return _errorString;
}

EDifferentNameList::EDifferentNameList() {
}

const char *EDifferentNameList::what() const throw () {
    return "Different name list in share/globalize/privatize\n";
}

EInvalidType::EInvalidType() {
}

const char *EInvalidType::what() const throw () {
    return "Invalid type";
}

char EInvalidFileFormat::_errorString[1024];
EInvalidFileFormat::EInvalidFileFormat(const char *fileName) {
    sprintf(_errorString, "Invalid format of file \"%s\"", fileName);
}

const char *EInvalidFileFormat::what() const throw () {
    return _errorString;
}

EInvalidIDim::EInvalidIDim() {
}

const char *EInvalidIDim::what() const throw () {
    return "Invalid index of dimension";
}

EInvalidNDims::EInvalidNDims() {
}

const char *EInvalidNDims::what() const throw () {
    return "Invalid number of dimensions";
}

EInvalidGrid::EInvalidGrid() {
}

const char *EInvalidGrid::what() const throw () {
    return "Invalid grid specification";
}

EIOError::EIOError() {
}

const char *EIOError::what() const throw () {
    return "IO error";
}

ENotEnoughMemory::ENotEnoughMemory() {
}

const char *ENotEnoughMemory::what() const throw () {
    return "Not enough memory";
}

char EDeadLockWait::_errorString[1024];

EDeadLockWait::EDeadLockWait(const char * programID) {
    sprintf(_errorString, "[ERROR] DEADLOCK: calling sync at client %s, meanwhile "
            "calling wait at server", programID);
}

const char *EDeadLockWait::what() const throw () {
    return _errorString;
}

char EOutOfBound::_errorString[1024];

EOutOfBound::EOutOfBound(const char *arrayName) {
    sprintf(_errorString,
            "Failed to access an element "
            "in %s (the location of the element is out of bound)",
            arrayName);
}

const char *EOutOfBound::what() const throw () {
    return _errorString;
}

char EInvalidProcID::_errorString[1024];

EInvalidProcID::EInvalidProcID(unsigned long procID) {
    Runtime *runtime = Runtime::getActiveRuntime();
    sprintf(_errorString,
            "procID %lu is out of bound (the valid bound is 0 to %lu)",
            procID, (unsigned long)runtime->getNumberOfProcesses() - 1);
}

const char *EInvalidProcID::what() const throw () {
    return _errorString;
}

char ETypeNotMatched::_errorString[1024];
ETypeNotMatched::ETypeNotMatched(const char *name, const char *expectdType) {
    sprintf(_errorString,
	    "variable %s is not of type %s",
	    name, expectdType);
}

const char *ETypeNotMatched::what() const throw () {
    return _errorString;
}

char EVariableNotFound::_errorString[1024];

EVariableNotFound::EVariableNotFound(const char *variableName) {
    sprintf(_errorString, "Variable %s does not exist", variableName);
}

const char *EVariableNotFound::what() const throw () {
    return _errorString;
}

EInvalidArgument::EInvalidArgument() {
}

const char *EInvalidArgument::what() const throw () {
    return "Invalid argument";
}

char EInvalidElementPosition::_errorString[1024];

EInvalidElementPosition::EInvalidElementPosition(
        std::string requestID, unsigned iDim,
        unsigned nVars, int64_t indexAlongDim, const uint64_t valueOfVar[],
	const uint64_t ubound) {
    _requestID = requestID;
    _iDim = iDim;
    _nVars = nVars;
    _indexAlongDim = indexAlongDim;
    if (valueOfVar != NULL) {
        for (unsigned iVar = 0; iVar < _nVars; ++iVar) {
            _valueOfVar[iVar] = valueOfVar[iVar];
        }
    }
    std::stringstream ss;
    ss << "[ERROR] Invalid element position " << indexAlongDim
            << " along dim " << iDim << ", which should < " << ubound << std::endl;
    ss << "\tRequest call stack = " << _requestID << std::endl;
    if (valueOfVar != NULL) {
        ss << "\tValue of variables (or components):" << std::endl;
        for (unsigned iVar = 0; iVar < _nVars; ++iVar) {
            ss << "\t\t" << valueOfVar[iVar] << std::endl;
        }
    }
    strcpy(_errorString,ss.str().c_str());
}

const char *EInvalidElementPosition::what() const throw () {
    return _errorString;
}

std::string EInvalidElementPosition::getRequestID() const {
    return _requestID;
}

unsigned EInvalidElementPosition::getIDim() const {
    return _iDim;
}

unsigned EInvalidElementPosition::getNVars() const {
    return _nVars;
}

int64_t EInvalidElementPosition::getIndexAlongDim() const {
    return _indexAlongDim;
}

uint64_t EInvalidElementPosition::getValueOfVar(unsigned iDim) const {
    return _valueOfVar[iDim];
}

EInvalidRegionDescriptor::EInvalidRegionDescriptor(
        unsigned iDim, uint64_t iRegion, int64_t begin, int64_t end) {
    _iDim = iDim;
    _iRegion = iRegion;
    _begin = begin;
    _end = end;
}

unsigned EInvalidRegionDescriptor::getIDim() const {
    return _iDim;
}

int64_t EInvalidRegionDescriptor::getBegin() const {
    return _begin;
}

int64_t EInvalidRegionDescriptor::getEnd() const {
    return _end;
}

uint64_t EInvalidRegionDescriptor::getIRegion() const {
    return _iRegion;
}

const char *EInvalidRegionDescriptor::what() const throw () {
    return "Invalid region descriptor";
}

EClientArrayTooSmall::EClientArrayTooSmall(std::string requestID,
        uint64_t clientArraySize, uint64_t requestSize) {
    _requestID = requestID;
    _clientArraySize = clientArraySize;
    _requestSize = requestSize;
}

const char *EClientArrayTooSmall::what() const throw () {
    return "size of client array is too small";
}

std::string EClientArrayTooSmall::getRequestID() const {
    return _requestID;
}

uint64_t EClientArrayTooSmall::getClientArraySize() const {
    return _clientArraySize;
}

uint64_t EClientArrayTooSmall::getRequestSize() const {
    return _requestSize;
}

char EInvalidCustomizedSync::_errorString[1024];

EInvalidCustomizedSync::EInvalidCustomizedSync(unsigned long senderID,
        unsigned long receiverID, unsigned long nReqUpd) {
    sprintf(_errorString, "Invalid customized sync, without sendto proc %lu "
            "from sender proc %lu that has %lu request/update messages "
            "to this receiver", receiverID, senderID, nReqUpd);
}

const char *EInvalidCustomizedSync::what() const throw () {
    return _errorString;
}

ENotAvailable::ENotAvailable() {
}

const char *ENotAvailable::what() const throw () {
    return "Not available";
}

ECorruptedRuntime::ECorruptedRuntime() {
}

const char *ECorruptedRuntime::what() const throw () {
    return "Corrupted Runtime";
}

ESocketFailure::ESocketFailure() {
}

const char *ESocketFailure::what() const throw () {
    return "Failed to open socket";
}

EInvalidAccess::EInvalidAccess() {
}

const char *EInvalidAccess::what() const throw () {
    return "Invalid access";
}

ELocalToGlobalAssignment::ELocalToGlobalAssignment() {
}

const char *ELocalToGlobalAssignment::what() const throw () {
    return "Assigning a local value to a global variable is not allowed";
}

char EUnmatchedSync::_errorString[1024];
EUnmatchedSync::EUnmatchedSync(const char *myTag, uint64_t myHash, uint64_t partnerHash)
{
    sprintf(_errorString, "[ERROR] Unmatched sync: myTag = %s, myHash1 = %u, myHash2 = %u, partnerHash1 = %u, partnerHash2 = %u",
	    myTag, (unsigned)(myHash & 0xffffffff), (unsigned)((myHash >> 32)&0xffffffff), (unsigned)(partnerHash & 0xffffffff), (unsigned)((partnerHash >> 32)&0xffffffff));
}

const char *EUnmatchedSync::what() const throw () {
    return _errorString;
}

