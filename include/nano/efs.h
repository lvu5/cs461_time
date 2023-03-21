//______________________________________________________________________________
// Kernel side of EFS (Ethos File System)
//______________________________________________________________________________

#ifndef _EFS_H_
#define _EFS_H_

#include <ethos/ethosTypes.h>
#include <ethos/types.h>
#include <ethos/kernel/blkfront.h>

#define EFS_INFO_NUM_SLOTS 512

#define EFS_REPLY_CALL_INFO_FREE  0
#define EFS_REPLY_CALL_INFO_READ  1
#define EFS_REPLY_CALL_INFO_WRITE 2

RdId efsRdId;
Coder *rpcEfsCoder;

struct EfsReplyInfo{
    int readyForReply; // 1 if ready to reply to EFS process
    int replyCallType; // read or write
    uint64 cid;        // RPC call ID
    Ref *buffer;       // for read request only
    Status status;     // status for request
};

struct EfsReplyInfo efsReplyInfoArray[EFS_INFO_NUM_SLOTS];

void efsRpcReplyReadSectors(unsigned long id, Status status);
void efsRpcReplyWriteSectors(unsigned long id, Status status);
#endif // _EFS_H_
