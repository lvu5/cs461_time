//______________________________________________________________________________
// Automatically generated file from source status.def.   Do not modify!!!
// We cannot defined these constants as an enum because we must be able
// to use them from assembly.
//______________________________________________________________________________
#ifndef __STATUS_H__
#define __STATUS_H__

#ifndef __ASSEMBLER__
#include <stdint.h>
#endif

// everything is OK, value=0
#define StatusOk		0 
// already been closed
#define StatusAlreadyClosed		1 
// Object to be created already exists
#define StatusExists		2 
// generic failure, when no more specific reason appropriate
#define StatusFail		3 
// authentication failed
#define StatusInvalidAuthentication		4 
// Elf file (executable) is invalid
#define StatusInvalidElfFile		5 
// event is invalid 
#define StatusInvalidEvent		6 
// eventWaitTree is invalid
#define StatusInvalidEventWaitTree		7 
// fd is invalid
#define StatusInvalidFd		8 
// FdId is invalid
#define StatusInvalidFdId		9 
// Filename is invalid
#define StatusInvalidFileName		10 
// FileType is invalid in FileInformation
#define StatusInvalidFileType		11 
// Handle is invalid
#define StatusInvalidHandle		12 
// Hostname is invalid
#define StatusInvalidHostname		13 
// Length is invalid
#define StatusInvalidLength		14 
// Per user executable is invalid
#define StatusInvalidPerUserExecutable		15 
// Process is invalid
#define StatusInvalidProcess		16 
// String is invalid
#define StatusInvalidString		17 
// User is invalid
#define StatusInvalidUser		18 
// Status result returned on exit of killed process
#define StatusKilled		19 
// Ran out of memory
#define StatusNoMemory		20 
// Ran out of space in fixed sized data structure
#define StatusNoSpace		21 
// requested operation not authorized
#define StatusNotAuthorized		22 
// diretory expected
#define StatusNotADirectory		23 
// file expected
#define StatusNotAFile		24 
// Directory, or other structure, expected to be empty
#define StatusNotEmpty		25 
// Requested item not found
#define StatusNotFound		26 
// Unimplemented functionality
#define StatusNotImplemented		27 
// Page fault
#define StatusPageFault		28 
// directory has been removed
#define StatusRemovedDirectory		29 
// insufficient return space, used on event retires
#define StatusReturnSpace		30 
// Data ends (i.e. EOF)
#define StatusDataEnds		31 
// Xen status returns
#define StatusXenAgain		32 
#define StatusXenBusy		33 
#define StatusXenComplete		34 
#define StatusXenInputOutput		35 
#define StatusXenInvalidValue		36 
#define StatusXenIsDirectory		37 
#define StatusXenReadOnly		38 
// write failed
#define StatusWriteFail		39 
#define StatusTunnelNotCreated		40 
// certificate signature field not valid for certificate
#define StatusCertBadSignature		41 
// certificate type stamp does not match directory in which certificate is stored
#define StatusCertBadTypeStamp		42 
// certificate is expired
#define StatusCertExpired		43 
// certificate not yet effective
#define StatusCertNotYetEffective		44 
// certificate credentials revoked
#define StatusCertCredentialsRevoked		45 
// event is not complete
#define StatusEventNotComplete		46 
// bad type verify
#define StatusInvalidType		47 
// this is the last line

// Can't use typedef with assembler.
#ifndef __ASSEMBLER__
typedef uint32_t Status;
#endif

#endif
