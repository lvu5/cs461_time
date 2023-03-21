//______________________________________________________________________________
/// Basic File Operations
//
// Jul-2008: Andrew Trumbo
// Jul-2009: Jon A. Solworth
//______________________________________________________________________________

#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__

#include <nano/kernel/common.h>
#include <nano/net/auth.h>
#include <nano/event.h>
// #include <nano/net/connection.h>
// #include <nano/net/rpc.h>
#include <nano/ethosTypes.h>
#include <nano/constant.h>
#include <nano/latch.h>

struct  ResourceDescriptorS;
typedef struct ResourceDescriptorS ResourceDescriptor;

struct ResourceDescriptorS
{
    RdId                     rdId;         ///< file resource descriptor

    Latch                    latch;        ///< serializes access to descriptor
    String                  *fileContents; ///< the contents of the file if fileContents.size nonzero

    FileInformation     fileInformation;
                                           ///< file information

    bool                      cached;       ///< true if both directory and cached
    msize_t                 refCount;     ///< number of pointers to object
    RdId                      parentRdId;   ///< parent of file, NULL if root
    ListHead               children;     ///< list of children cached in memory
};

typedef struct
{
    String             *fileName;     ///< file name    
    RdId                rdId;	      ///< file resource descriptor
    ListHead		sibling;      ///< sibling linked thtough this field
} ResourceDescriptorList;

extern HandleTable        resourceDescriptors;

#define DirectoryEmptyLinkCount 1

String *dot;
String *dotDot;

void resourceDescriptorInit(void);

Status resourceDescriptorFileInformation(RdId fdId, FileInformation fileInformation);

Status resourceDescriptorCreate(ResourceDescriptor *directoryResourceDescriptor,
				String *fileName,
				ResourceDescriptor **returnResourceDescriptor);

Status resourceDescriptorDestroy(RdId fdId);

Status resourceDescriptorFind(RdId fdId, ResourceDescriptor **resourceDescriptor);

void resourceDescriptorInitialize(void *resourceDescriptor);

void resourceDescriptorReleaseContents(ResourceDescriptor *resourceDescriptor);
				   
void resourceDescriptorPrint(ResourceDescriptor *resourceDescriptor);

Status resourceDescriptorUnlink(ResourceDescriptor *resourceDescriptor);

Status resourceDescriptorReferenceDecrement(RdId rdId);

Status resourceDescriptorReferenceIncrement(RdId rdId);

FdType resourceDescriptorToFdType(ResourceDescriptor *resourceDescriptor);

void resourceDescriptorListInitialize(void *resourceDescriptor);

void resourceDescriptorListPrint(ResourceDescriptorList *resourceDescriptorList);

Status resourceDescriptorListNext(ResourceDescriptor *directoryDescriptor, 
				  String *name, 
				  ResourceDescriptorList **successorDescriptorList);

Status resourceDescriptorListFindChild(ResourceDescriptor *resourceDescriptor,
				       String *name,
				       ResourceDescriptor **returnResourceDescriptor);

Status resourceDescriptorListInsert(ResourceDescriptor *rdParent,
				    String *name,
				    RdId rdId);

Status resourceDescriptorListFindNextChild(ResourceDescriptor *resourceDescriptor,
					   String *name,
					   ResourceDescriptorList **returnResourceDescriptorList);

Status resourceDescriptorListRemoveChild(ResourceDescriptor *resourceDescriptor,
					 String *name);

Status resourceDescriptorListLinkParent(ResourceDescriptor *rdParent,
					ResourceDescriptor *rdChild,
					ResourceDescriptorList *rdChildList);

Status resourceDescriptorListGetNext(ResourceDescriptor *directoryDescriptor,
				     String *nameIterator,
				     ResourceDescriptor **returnResourceDescriptor,
				     String **nameReturn);

Status resourceDescriptorListGetNextFile(ResourceDescriptor *directoryDescriptor,
					 String *nameIterator,
					 ResourceDescriptor **returnRescourceDescriptor,
					 String **nameReturn);
// Initialize data structures needed for file operations.
void fileSystemInit(void);

// Opens file/directory with permissions, setting resourceDescriptor to the value of the
// open file descriptor.  Returns status as appropriate.
Status directoryOpenDirectory(RdId dirId,
			      String* filename, 
			      Event *event);

// Creates directory with permissions, setting resourceDescriptor to the value of the
// open file descriptor.  Returns status as appropriate.
Status directoryCreateDirectory(
				RdId        dirId,
				String     *filename, 
				String     *label,
				HashRef    *typeHash,
				Event      *event);

struct advertise_s;

Status directoryCreateService(RdId                directoryId,
			      String             *service,
			      Event              *event);

Status directoryRemove(Syscall syscall,
		       RdId    dirId,
		       String *filename, 
		       Event  *event);

Status directoryGetNextName(RdId    dirId,
			    String *name,
			    Event  *event);

Status directoryMake(ResourceDescriptor *rdParent,
		     ResourceDescriptor *rdNew);

Status directoryClean(ResourceDescriptor *rdParent);

// Closes the passed file descriptor
// Returns status as appropriate.
Status resourceClose(RdId fdId);

static
inline
Status
fileNameValid(String *ref)
{
    if (!ref)
	{
	    return StatusInvalidFileName;
	}

    if (!refSize(ref))
	{
	    return StatusInvalidFileName;
	}

    if (refSize(ref) > MaxFileNameSize)
	{
	    return StatusInvalidFileName;
	}
    return StatusOk;
}


// Read the whole file
Status fileInformation(RdId fdId, String *filename, Event *event);

// Read the whole file
Status fileRandom(uint32 size, Event *event);

Status fileRead(RdId dirId, ProcessId processId, Fd fd, Event *event);

Status fileReadVar(RdId dirId, String *filename, Event *event);


// write the whole file
Status fileWriteVar(RdId dirRdId, String *fileName, String *contents, Event *event);

// Sign the file
Status signSign(RdId dirId, String *filename, Event *event);


Status metaFindEntry(
		     Event *event,
		     ResourceDescriptor *dirDescriptor,
		     String  *filename,
		     ResourceDescriptor **returnFileDescriptor
		     );

Status metaGetDirectory( 
			Event *event,
			ResourceDescriptor *directoryDescriptor
			 );

Status metaNext(ResourceDescriptor *directoryDescriptor, 
		Ref *name, 
		ResourceDescriptorList **successorDescriptorList);

Status  fileSystemDirectoryClean(String *string);
Status  fileSystemOpenDirectory(RdId rdId, String *str, RdId *outRdId);

Status  fileSystemOpenDirectoryPath (String *str, RdId *outRdId);

Status  fileSystemReadVarPath(String *path, String **contents);

Status  fileSystemOpenDirectoryVector(String **path, RdId *outRdId);

Status  fileSystemGetRandomBytes(uchar *x, unsigned long long xlen);

Status  fileSystemReadHostEphemeralKeyPair(EncryptionKeyPair *pair);
Status  fileSystemReadUserEncryptionKeyPair(AuthUser *user, EncryptionKeyPair *pair);
Status  fileSystemReadUserSignatureKeyPair(AuthUser *user, SignatureKeyPair *pair);
Status  fileSystemReadTyped(String *path[], EtnValue v);
Status  fileSystemReadAuthUser(void);

Status  fileSystemWriteHostEphemeralKeyPair(EncryptionKeyPair *pair);
Status  fileSystemWriteVarPath(String *path, String *contents);
Status  fileSystemWriteTyped(String *path[], EtnValue v);

Status  fileSystemReadFileInformation(String *path[], FileInformation *fileInfo);

String **fileSystemSub(String *path[], FileType typeOfFile);
String **fileSystemSubFiles(String *path[]);
String **fileSystemSubDirectories(String *path[]);

Status  pathNameConcat(String *directoryPathName, String *fileName, String **pathName);
Status  pathNameCreate(ResourceDescriptor *dir, ResourceDescriptor *fileResourceDescriptor, String *fileName);
Status  pathNameExplode(String *pathName, String ***returnPathComponent);
void    pathNameExplodeDestroy(String  **pathComponent);

#endif /* __FILE_SYSTEM_H__ */
